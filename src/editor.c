#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>

#include "editor.h"
#include "line.h"
#include "term.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int
editor_init(struct editor* e, int input_fd, int output_fd, const char* path)
{
    assert(e != NULL);

    e->input_fd = input_fd;
    e->output_fd = output_fd;

    e->scroll_x = 0;
    e->scroll_y = 0;
    e->cursor_x = 0;
    e->cursor_y = 0;

    e->line = calloc(1, sizeof(struct line));
    line_init(e->line);

    e->head = e->line;
    e->tail = e->line;

    e->line_count = 1;
    e->line_affinity = 0;
    e->line_index = 0;
    e->line_pos = 0;

    if (path != NULL) {
        FILE* fp = fopen(path, "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file: %s\n", path);
            return EDITOR_ERROR;
        }

        // read through each character in the file
        int c = 0;
        bool newline = false;
        while ((c = fgetc(fp)) != EOF) {
            if (newline) {
                struct line* line = calloc(1, sizeof(struct line));
                line_init(line);
                line->prev = e->tail;
                e->tail->next = line;
                e->tail = line;

                newline = false;
                e->line_count++;
            }
            // add a new line to the DLL when found
            if (c == '\n') {
                newline = true;
            } else {
                line_append(e->tail, c);
            }
        }

        if (ferror(fp)) {
            fprintf(stderr, "IO error while reading file: %s\n", path);
            return EDITOR_ERROR;
        }

        fclose(fp);
    }

    term_cursor_save(e->output_fd);
    term_screen_save(e->output_fd);

    // capture original termios config to restore upon exit
    if (tcgetattr(input_fd, &e->original_termios) == -1) {
        fprintf(stderr, "error capturing original termios: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    if (!term_mode_raw(input_fd)) {
        fprintf(stderr, "error setting terminal to raw mode: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    // TODO: allow for updating size via sigwinch handler
    if (!term_size(output_fd, &e->width, &e->height)) {
        fprintf(stderr, "error getting terminal size: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    return EDITOR_OK;
}

int
editor_free(struct editor* e)
{
    assert(e != NULL);

    // restore original termios config
    tcsetattr(e->input_fd, TCSAFLUSH, &e->original_termios);

    // restore the terminal before exiting
    term_screen_restore(e->output_fd);
    term_cursor_restore(e->output_fd);

    // write the output.txt file
    FILE* fp = fopen("output.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open output file: output.txt\n");
        fprintf(stderr, "skipping writing output file\n");
    } else {
        for (struct line* line = e->head; line != NULL; line = line->next) {
            fwrite(line->buf, line->size, 1, fp);
            fputc('\n', fp);
        }
        fclose(fp);
    }

    // free the DLL of lines
    struct line* line = e->head;
    while (line != NULL) {
        struct line* current = line;
        line = line->next;
        line_free(current);
        free(current);
    }

    return EDITOR_OK;
}

int
editor_draw(const struct editor* e)
{
    assert(e != NULL);

    // TODO optimize this to not redraw everything upon every key input
    // thats probs too slow. its def inefficient
    term_screen_clear(e->output_fd);
    term_cursor_reset(e->output_fd);
    term_cursor_hide(e->output_fd);

    // this is our iterator
    struct line* line = e->head;

    // skip lines based on scroll value
    for (long s = e->scroll_y; s > 0; s--) line = line->next;

    // draw the text lines
    for (long i = 0; i < e->height - 1; i++) {
        if (line == NULL) break;
        term_cursor_set(e->output_fd, 0, i);
        term_screen_write(e->output_fd,
            line->buf + e->scroll_x,
            MIN(line->size - e->scroll_x, e->width));
        line = line->next;
    }

    // draw the status message
    char status[128] = { 0 };
    snprintf(status, sizeof(status),
        "-- cx: %3ld | cy: %3ld | lp: %3ld | ls: %3ld | la: %3ld | sx: %3ld | sy %3ld --",
        e->cursor_x,
        e->cursor_y,
        e->line_pos,
        e->line->size,
        e->line_affinity,
        e->scroll_x,
        e->scroll_y);
    term_cursor_set(e->output_fd, 1, e->height - 1);
    term_screen_write(e->output_fd, status, strlen(status));

    // draw the cursor pos indicator
    char curpos[64] = { 0 };
    long curpos_size = snprintf(curpos, sizeof(curpos),
        "%8ld,%-8ld", e->line_index + 1, e->line_pos + 1);
    term_cursor_set(e->output_fd, e->width - curpos_size - 1, e->height - 1);
    term_screen_write(e->output_fd, curpos, strlen(curpos));

    term_cursor_set(e->output_fd, e->cursor_x, e->cursor_y);
    term_cursor_show(e->output_fd);

    return EDITOR_OK;
}

int
editor_key_wait(const struct editor* e, int* c)
{
    assert(e != NULL);
    assert(c != NULL);

    if (!term_key_wait(e->input_fd, c)) {
        fprintf(stderr, "error waiting for input: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    return EDITOR_OK;
}

int
editor_rune_insert(struct editor* e, char rune)
{
    assert(e != NULL);

    line_insert(e->line, e->line_pos, rune);
    editor_cursor_right(e);

    return EDITOR_OK;
}

int
editor_rune_delete(struct editor* e)
{
    assert(e != NULL);

    if (e->line_pos == 0 && e->line->prev != NULL) {
        struct line* prev = e->line->prev;

        // move cursor and line values to prev line
        e->line_pos = prev->size;
        e->line_affinity = prev->size;

        // horizontal scrolling?
        if (e->line_pos >= e->width) {
            e->scroll_x = e->line_pos - (e->width / 2);
        }
        e->cursor_x = prev->size - e->scroll_x;

        // append current line to prev line
        for (long i = 0; i < e->line->size; i++) {
            line_append(prev, line_get(e->line, i));
        }

        // unlink and delete the current line
        struct line* old = e->line;
        if (e->line->next != NULL) e->line->next->prev = e->line->prev;
        e->line->prev->next = e->line->next;
        e->line = e->line->prev;
        line_free(old);
        free(old);

        e->line_index--;
        e->line_count--;

        // vertical scrolling
        if (e->cursor_y <= 0) {
            e->scroll_y--;
        } else {
            e->cursor_y--;
        }
    } else if (e->cursor_x > 0) {
        editor_cursor_left(e);
        line_delete(e->line, e->line_pos);
    }

    return EDITOR_OK;
}

int
editor_line_break(struct editor* e)
{
    assert(e != NULL);

    struct line* line = calloc(1, sizeof(struct line));
    line_init(line);

    // copy rest of existing line to the new one
    for (long i = e->line_pos; i < e->line->size; i++) {
        line_append(line, line_get(e->line, i));
    }

    // delete rest of existing line and replace the NL
    for (long i = e->line->size - 1; i >= e->line_pos; i--) {
        line_delete(e->line, i);
    }

    // link the new line in
    line->prev = e->line;
    line->next = e->line->next;
    if (e->line->next != NULL) e->line->next->prev = line;
    e->line->next = line;

    e->line = line;
    e->line_count++;
    e->line_index++;

    e->line_affinity = 0;
    e->line_pos = 0;

    // horizontal scrolling
    e->scroll_x = 0;
    e->cursor_x = 0;

    // vertical scrolling
    if (e->cursor_y >= e->height - 2) {
        e->scroll_y++;
    } else {
        e->cursor_y++;
    }

    return EDITOR_OK;
}

int
editor_cursor_left(struct editor* e)
{
    assert(e != NULL);

    // if at start of line, done
    if (e->line_pos <= 0) return EDITOR_OK;

    if (e->cursor_x <= 0) {
        e->scroll_x--;
    } else {
        e->cursor_x--;
    }

    e->line_pos--;
    e->line_affinity = e->line_pos;

    return EDITOR_OK;
}

int
editor_cursor_right(struct editor* e)
{
    assert(e != NULL);

    // if at end of line, done
    if (e->line_pos >= e->line->size) return EDITOR_OK;

    if (e->cursor_x >= e->width - 1) {
        e->scroll_x++;
    } else {
        e->cursor_x++;
    }

    e->line_pos++;
    e->line_affinity = e->line_pos;

    return EDITOR_OK;
}

int
editor_cursor_up(struct editor* e)
{
    assert(e != NULL);

    // if at top of lines, done
    if (e->line->prev == NULL) return EDITOR_OK;

    // vertical scrolling
    if (e->cursor_y <= 0) {
        e->scroll_y--;
    } else {
        e->cursor_y--;
    }

    // move to the prev line
    e->line = e->line->prev;
    e->line_index--;

    // handle affinity
    if (e->line_affinity >= e->line->size) {
        if (e->line->size >= e->width + e->scroll_x) e->scroll_x = e->line_affinity - e->width + 1;
        e->cursor_x = e->line->size - e->scroll_x;
        e->line_pos = e->line->size;
    } else {
        e->cursor_x = e->line_affinity - e->scroll_x;
        e->line_pos = e->line_affinity;
    }

    // horizontal scrolling
    if (e->line_pos < e->scroll_x) {
        e->scroll_x = MAX(e->line_pos - e->width, 0);
        e->cursor_x = e->line_pos;
    }

    return EDITOR_OK;
}

int
editor_cursor_down(struct editor* e)
{
    assert(e != NULL);

    // if at bottom of lines, done
    if (e->line->next == NULL) return EDITOR_OK;  

    // vertical scrolling
    if (e->cursor_y >= e->height - 2) {
        e->scroll_y++;
    } else {
        e->cursor_y++;
    }

    // move to the next line
    e->line = e->line->next;
    e->line_index++;

    // handle affinity
    if (e->line_affinity >= e->line->size) {
        if (e->line->size >= e->width + e->scroll_x) e->scroll_x = e->line_affinity - e->width + 1;
        e->cursor_x = e->line->size - e->scroll_x;
        e->line_pos = e->line->size;
    } else {
        e->cursor_x = e->line_affinity - e->scroll_x;
        e->line_pos = e->line_affinity;
    }

    // horizontal scrolling
    if (e->line_pos < e->scroll_x) {
        e->scroll_x = MAX(e->line_pos - e->width, 0);
        e->cursor_x = e->line_pos;
    }

    return EDITOR_OK;
}

int
editor_cursor_home(struct editor* e)
{
    assert(e != NULL);

    e->scroll_x = 0;
    e->cursor_x = 0;
    e->line_pos = 0;
    e->line_affinity = 0;

    return EDITOR_OK;
}

int
editor_cursor_end(struct editor* e)
{
    assert(e != NULL);

    long size = e->line->size;
    if (size >= e->width) {
        e->scroll_x = size - e->width + 1;
    }
    e->cursor_x = MIN(size, e->width - 1);
    e->line_pos = size;
    e->line_affinity = size;

    return EDITOR_OK;
}

int
editor_cursor_page_up(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_page_down(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}
