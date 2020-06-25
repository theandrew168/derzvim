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

    // capture original termios config to restore upon exit
    if (tcgetattr(input_fd, &e->original_termios) == -1) {
        fprintf(stderr, "error capturing original termios: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    // TODO: allow for updating size via sigwinch handler
    if (!term_size(output_fd, &e->width, &e->height)) {
        fprintf(stderr, "error getting terminal size: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    if (!term_mode_raw(input_fd)) {
        fprintf(stderr, "error setting terminal to raw mode: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

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
    char* status = "-- status message --";
    term_cursor_set(e->output_fd, 1, e->height - 1);
    term_screen_write(e->output_fd, e->width, e->height, status, strlen(status));

    // draw the cursor pos indicator
    char curpos[64] = { 0 };
    long curpos_size = snprintf(curpos, sizeof(curpos), "%8ld,%-8ld", e->cursor_x + 1, e->cursor_y + 1);
    term_cursor_set(e->output_fd, e->width - curpos_size - 1, e->height - 1);
    term_screen_write(e->output_fd, e->width, e->height, curpos, strlen(curpos));

    term_cursor_set(e->output_fd, 0, 0);
    term_cursor_show(e->output_fd);

    return EDITOR_OK;
}

int
editor_key_wait(const struct editor* e, int* c)
{
    if (!term_key_wait(e->input_fd, c)) {
        fprintf(stderr, "error waiting for input: %s\n", strerror(errno));
        return EDITOR_ERROR;
    }

    return EDITOR_OK;
}

int
editor_cursor_left(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_right(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_up(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_down(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_home(struct editor* e)
{
    assert(e != NULL);
    return EDITOR_OK;
}

int
editor_cursor_end(struct editor* e)
{
    assert(e != NULL);
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
