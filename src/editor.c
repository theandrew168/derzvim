#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>

#include "array.h"
#include "editor.h"
#include "line.h"
#include "term.h"

int
editor_init(struct editor* e, int input_fd, int output_fd, const char* path)
{
    assert(e != NULL);

    e->input_fd = input_fd;
    e->output_fd = output_fd;

    e->scroll = 0;
    e->cursor_x = 0;
    e->cursor_y = 0;

    e->current_line = calloc(1, sizeof(struct line));
    line_init(e->current_line);

    e->head = e->current_line;
    e->tail = e->current_line;
    e->line_count = 0;
    e->line_pos = 0;
    e->line_affinity = 0;

    if (path != NULL) {
        FILE* fp = fopen(path, "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file: %s\n", path);
            return EDITOR_ERROR;
        }

        // read through each character in the file
        int c = 0;
        while ((c = fgetc(fp)) != EOF) {
            array_append(&e->tail->array, c);

            // add a new line to the DLL when found
            if (c == '\n') {
                struct line* line = calloc(1, sizeof(struct line));
                line_init(line);
                line->prev = e->tail;
                e->tail->next = line;
                e->tail = line;

                e->line_count++;
            }
        }

        if (ferror(fp)) {
            fprintf(stderr, "IO error while reading file: %s\n", path);
            return EDITOR_ERROR;
        }

        fclose(fp);
    } else {
        array_append(&e->tail->array, '\n');
        e->line_count++;
    }

    // TODO: allow for updating size via sigwinch handler
    if (!term_size(output_fd, &e->width, &e->height)) {
        fprintf(stderr, "error getting terminal size: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    // capture original termios config to restore upon exit
    if (tcgetattr(input_fd, &e->original_termios) == -1) {
        fprintf(stderr, "error capturing original termios: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    if (!term_mode_raw(input_fd)) {
        fprintf(stderr, "error setting terminal to raw mode: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    return EDITOR_OK;
}

int
editor_free(struct editor* e)
{
    assert(e != NULL);

    // clear and reset the terminal before exiting
    term_screen_clear(e->output_fd);
    term_cursor_reset(e->output_fd);

    // restore original termios config
    tcsetattr(e->input_fd, TCSAFLUSH, &e->original_termios);

    // write the output.txt file
    FILE* fp = fopen("output.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open output file: output.txt\n");
        fprintf(stderr, "skipping writing output file\n");
    } else {
        for (struct line* line = e->head; line != NULL; line = line->next) {
            fwrite(line->array.buf, line->array.size, 1, fp);
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

    // draw the text lines
    struct line* line = e->head;
    for (long i = 0; i < e->height - 1; i++) {
        if (line == NULL) break;
        term_screen_write(e->output_fd, e->width, e->height, line->array.buf, line->array.size);
        line = line->next;
    }

    // draw the status message
    char status[128] = { 0 };
    snprintf(status, sizeof(status), "-- cx: %ld | cy: %ld | lp: %ld | la: %ld | lc: %ld --",
        e->cursor_x, e->cursor_y, e->line_pos, e->line_affinity, e->line_count);
    term_cursor_set(e->output_fd, 1, e->height - 1);
    term_screen_write(e->output_fd, e->width, e->height, status, strlen(status));

    // draw the cursor pos indicator
    char curpos[64] = { 0 };
    long curpos_size = snprintf(curpos, sizeof(curpos), "%8ld,%-8ld", e->cursor_y + 1, e->cursor_x + 1);
    term_cursor_set(e->output_fd, e->width - curpos_size - 1, e->height - 1);
    term_screen_write(e->output_fd, e->width, e->height, curpos, strlen(curpos));

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

    line_insert(e->current_line, e->line_pos, rune);
    e->cursor_x++;
    e->line_pos++;
    e->line_affinity++;

    return EDITOR_OK;
}

int
editor_rune_delete(struct editor* e)
{
    // TODO: need to handle scrolling in here
    assert(e != NULL);

    if (e->cursor_x == 0 && e->current_line->prev != NULL) {
        struct line* prev = e->current_line->prev;

        // delete NL from prev line
        e->cursor_x = line_size(prev) - 1;
        e->line_pos = line_size(prev) - 1;
        e->line_affinity = line_size(prev) - 1;
        line_delete(prev, line_size(prev) - 1);

        // append current line to prev line
        for (long i = 0; i < line_size(e->current_line); i++) {
            line_append(prev, line_get(e->current_line, i));
        }

        // unlink and delete the current line
        struct line* old = e->current_line;
        if (e->current_line->next != NULL) e->current_line->next->prev = e->current_line->prev;
        e->current_line->prev->next = e->current_line->next;
        e->current_line = e->current_line->prev;
        line_free(old);
        free(old);

        e->cursor_y--;
        e->line_count--;
    } else if (e->cursor_x > 0) {
        editor_cursor_left(e);
        line_delete(e->current_line, e->line_pos);
    }

    return EDITOR_OK;
}

int
editor_line_break(struct editor* e)
{
    // TODO: need to handle scrolling in here
    assert(e != NULL);

    struct line* line = calloc(1, sizeof(struct line));
    line_init(line);

    // copy rest of existing line to the new one (including the NL)
    for (long i = e->line_pos; i < line_size(e->current_line); i++) {
        line_append(line, line_get(e->current_line, i));
    }

    // delete rest of existing line and replace the NL
    for (long i = line_size(e->current_line) - 1; i >= e->line_pos; i--) {
        line_delete(e->current_line, i);
    }
    line_append(e->current_line, '\n');

    // link the new line in
    line->prev = e->current_line;
    line->next = e->current_line->next;
    if (e->current_line->next != NULL) e->current_line->next->prev = line;
    e->current_line->next = line;

    e->current_line = line;
    e->line_count++;

    // move cursor to the start of the new line
    e->cursor_y++;
    e->cursor_x = 0;
    e->line_pos = 0;
    e->line_affinity = 0;

    return EDITOR_OK;
}

int
editor_cursor_left(struct editor* e)
{
    assert(e != NULL);

    if (e->cursor_x <= 0) return EDITOR_OK;
    e->cursor_x--;
    e->line_pos--;
    e->line_affinity = e->line_pos;

    return EDITOR_OK;
}

int
editor_cursor_right(struct editor* e)
{
    assert(e != NULL);

    if (e->cursor_x >= e->width - 1) return EDITOR_OK;
    if (e->cursor_x >= line_size(e->current_line) - 1) return EDITOR_OK;  // will be "- 2" in normal mode
    e->cursor_x++;
    e->line_pos++;
    e->line_affinity = e->line_pos;

    return EDITOR_OK;
}

int
editor_cursor_up(struct editor* e)
{
    assert(e != NULL);

    if (e->cursor_y <= 0) return EDITOR_OK;
    e->current_line = line_prev(e->current_line);
    if (e->line_affinity >= line_size(e->current_line)) {
        e->cursor_x = line_size(e->current_line) - 1;
        e->line_pos = line_size(e->current_line) - 1;
    } else {
        e->cursor_x = e->line_affinity;
        e->line_pos = e->line_affinity;
    }
    e->cursor_y--;

    return EDITOR_OK;
}

int
editor_cursor_down(struct editor* e)
{
    assert(e != NULL);

    // leave a line for the status bar
    if (e->cursor_y >= e->height - 2) return EDITOR_OK;
    if (e->cursor_y >= e->line_count - 1) return EDITOR_OK;
    e->current_line = line_next(e->current_line);
    if (e->line_affinity >= line_size(e->current_line)) {
        e->cursor_x = line_size(e->current_line) - 1;
        e->line_pos = line_size(e->current_line) - 1;
    } else {
        e->cursor_x = e->line_affinity;
        e->line_pos = e->line_affinity;
    }
    e->cursor_y++;

    return EDITOR_OK;
}

int
editor_cursor_home(struct editor* e)
{
    assert(e != NULL);

    e->cursor_x = 0;
    e->line_pos = 0;
    e->line_affinity = 0;

    return EDITOR_OK;
}

int
editor_cursor_end(struct editor* e)
{
    assert(e != NULL);

    e->cursor_x = line_size(e->current_line) - 1;
    e->line_pos = line_size(e->current_line) - 1;
    e->line_affinity = line_size(e->current_line) - 1;

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
