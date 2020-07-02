#include <assert.h>
#include <errno.h>
#include <stdbool.h>
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

    e->line = calloc(1, sizeof(struct line));
    line_init(e->line);

    e->head = e->line;
    e->tail = e->line;

    e->line_count = 1;
    e->line_affinity = 0;
    e->line_index = 0;
    e->line_pos = 0;

    if (path != NULL) {
        lines_init(&e->head, &e->tail, &e->line_count, path);
    }

    term_cursor_save(e->output_fd);

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

    // initialize a blank screen
    term_erase_screen(e->output_fd);
    term_cursor_pos_home(e->output_fd);

    // set scrolling region to the main text area
    term_scroll_smooth(e->output_fd);
    term_scroll_region_set(e->output_fd, 0, e->height - 2);
    term_scroll_region_on(e->output_fd);

    return EDITOR_OK;
}

int
editor_free(struct editor* e)
{
    assert(e != NULL);

    // restore original termios config
    tcsetattr(e->input_fd, TCSAFLUSH, &e->original_termios);

    // restore screen and cursor upon exit
    term_erase_screen(e->output_fd);
    term_scroll_region_off(e->output_fd);

    // write the output.txt file
    lines_write(e->head, "output.txt");

    // free the DLL of lines
    lines_free(&e->head, &e->tail);

    return EDITOR_OK;
}

static void
draw_status_bar(struct editor* e)
{
    term_cursor_save(e->output_fd);
    term_scroll_region_off(e->output_fd);

    // draw the status message
    char status[128] = { 0 };
    snprintf(status, sizeof(status),
        "-- lp: %3ld | ls: %3ld | la: %3ld --",
        e->line_pos,
        e->line->size,
        e->line_affinity);
    term_cursor_pos_set(e->output_fd, 1, e->height - 1);
    term_screen_write(e->output_fd, status, strlen(status));

    // draw the cursor pos indicator
    char curpos[64] = { 0 };
    long curpos_size = snprintf(curpos, sizeof(curpos),
        "%8ld,%-8ld", e->line_index + 1, e->line_pos + 1);
    term_cursor_pos_set(e->output_fd, e->width - curpos_size - 1, e->height - 1);
    term_screen_write(e->output_fd, curpos, strlen(curpos));

    term_scroll_region_on(e->output_fd);
    term_cursor_restore(e->output_fd);
}

bool
editor_run(struct editor* e)
{
    draw_status_bar(e);

    int c = 0;
    if (!term_key_wait(e->input_fd, &c)) return false;

    switch (c) {
        case CTRL_KEY('q'):
            return false;
        case KEY_ARROW_UP:
            if (e->line->prev == NULL) break;
            term_cursor_up(e->output_fd, 1, true);
            e->line = e->line->prev;
            e->line_index--;

            // account for line affinity
            e->line_pos = MIN(e->line->size, e->line_affinity);
            term_cursor_left(e->output_fd, 999);
            term_cursor_right(e->output_fd, e->line_pos);
            break;
        case KEY_ARROW_DOWN:
            if (e->line->next == NULL) break;
            term_cursor_down(e->output_fd, 1, true);
            e->line = e->line->next;
            e->line_index++;

            // account for line affinity
            e->line_pos = MIN(e->line->size, e->line_affinity);
            term_cursor_left(e->output_fd, 999);
            term_cursor_right(e->output_fd, e->line_pos);
            break;
        case KEY_ARROW_LEFT:
            if (e->line_pos <= 0) break;
            term_cursor_left(e->output_fd, 1);
            e->line_pos--;
            e->line_affinity = e->line_pos;
            break;
        case KEY_ARROW_RIGHT:
            if (e->line_pos >= e->line->size) break;
            term_cursor_right(e->output_fd, 1);
            e->line_pos++;
            e->line_affinity = e->line_pos;
            break;
        case KEY_ENTER:
            // break the current line
            line_break(e->line, e->line_pos);
            e->line_count++;

            // redraw original line
            term_cursor_left(e->output_fd, e->line->size);
            term_erase_line_after(e->output_fd);
            term_screen_write(e->output_fd, e->line->buf, e->line->size);

            // move to the next line
            term_cursor_next_line(e->output_fd);
            e->line = e->line->next;
            e->line_index++;

            // scroll region down
            long cx, cy;
            term_cursor_pos_get(e->output_fd, e->input_fd, &cx, &cy);
            term_cursor_save(e->output_fd);
            term_scroll_region_set(e->output_fd, cy, e->width - 2);
            term_cursor_up(e->output_fd, 1, true);
            term_scroll_region_set(e->output_fd, 0, e->width - 2);
            term_cursor_restore(e->output_fd);

            // draw the split line
            term_screen_write(e->output_fd, e->line->buf, e->line->size);
            term_cursor_left(e->output_fd, e->line->size);

            e->line_affinity = 0;
            e->line_pos = 0;
            break;
        default:
            if (c < 32 || c > 126) break;
            line_insert(e->line, e->line_pos, c);
            e->line_pos++;
            e->line_affinity++;

            // move the cursor to the right and redraw the line
            term_cursor_right(e->output_fd, 1);
            term_cursor_save(e->output_fd);
            term_cursor_left(e->output_fd, e->line->size);
            term_erase_line_after(e->output_fd);
            term_screen_write(e->output_fd, e->line->buf, e->line->size);
            term_cursor_restore(e->output_fd);
            break;
    }

    return true;
}

//int
//editor_rune_delete(struct editor* e)
//{
//    assert(e != NULL);
//
//    if (e->line_pos == 0 && e->line->prev != NULL) {
//        struct line* prev = e->line->prev;
//
//        // move cursor and line values to prev line
//        e->line_pos = prev->size;
//        e->line_affinity = prev->size;
//
//        // horizontal scrolling?
//        if (e->line_pos >= e->width) {
//            e->scroll_x = e->line_pos - (e->width / 2);
//        }
//        e->cursor_x = prev->size - e->scroll_x;
//
//        // move back a line and merge the two
//        e->line = e->line->prev;
//        line_merge(e->line, e->line->next);
//
//        e->line_index--;
//        e->line_count--;
//
//        // vertical scrolling
//        if (e->cursor_y <= 0) {
//            e->scroll_y--;
//        } else {
//            e->cursor_y--;
//        }
//    } else if (e->cursor_x > 0) {
//        editor_cursor_left(e);
//        line_delete(e->line, e->line_pos);
//    }
//
//    return EDITOR_OK;
//}
//int
//editor_cursor_up(struct editor* e)
//{
//    assert(e != NULL);
//
//    // if at top of lines, done
//    if (e->line->prev == NULL) return EDITOR_OK;
//
//    // vertical scrolling
//    if (e->cursor_y <= 0) {
//        e->scroll_y--;
//    } else {
//        e->cursor_y--;
//    }
//
//    // move to the prev line
//    e->line = e->line->prev;
//    e->line_index--;
//
//    // handle affinity
//    if (e->line_affinity >= e->line->size) {
//        if (e->line->size >= e->width + e->scroll_x) e->scroll_x = e->line_affinity - e->width + 1;
//        e->cursor_x = e->line->size - e->scroll_x;
//        e->line_pos = e->line->size;
//    } else {
//        e->cursor_x = e->line_affinity - e->scroll_x;
//        e->line_pos = e->line_affinity;
//    }
//
//    // horizontal scrolling
//    if (e->line_pos < e->scroll_x) {
//        e->scroll_x = MAX(e->line_pos - e->width, 0);
//        e->cursor_x = e->line_pos;
//    }
//
//    return EDITOR_OK;
//}
//
//int
//editor_cursor_down(struct editor* e)
//{
//    assert(e != NULL);
//
//    // if at bottom of lines, done
//    if (e->line->next == NULL) return EDITOR_OK;  
//
//    // vertical scrolling
//    if (e->cursor_y >= e->height - 2) {
//        e->scroll_y++;
//    } else {
//        e->cursor_y++;
//    }
//
//    // move to the next line
//    e->line = e->line->next;
//    e->line_index++;
//
//    // handle affinity
//    if (e->line_affinity >= e->line->size) {
//        if (e->line->size >= e->width + e->scroll_x) e->scroll_x = e->line_affinity - e->width + 1;
//        e->cursor_x = e->line->size - e->scroll_x;
//        e->line_pos = e->line->size;
//    } else {
//        e->cursor_x = e->line_affinity - e->scroll_x;
//        e->line_pos = e->line_affinity;
//    }
//
//    // horizontal scrolling
//    if (e->line_pos < e->scroll_x) {
//        e->scroll_x = MAX(e->line_pos - e->width, 0);
//        e->cursor_x = e->line_pos;
//    }
//
//    return EDITOR_OK;
//}
//
//int
//editor_cursor_home(struct editor* e)
//{
//    assert(e != NULL);
//
//    e->scroll_x = 0;
//    e->cursor_x = 0;
//    e->line_pos = 0;
//    e->line_affinity = 0;
//
//    return EDITOR_OK;
//}
//
//int
//editor_cursor_end(struct editor* e)
//{
//    assert(e != NULL);
//
//    long size = e->line->size;
//    if (size >= e->width) {
//        e->scroll_x = size - e->width + 1;
//    }
//    e->cursor_x = MIN(size, e->width - 1);
//    e->line_pos = size;
//    e->line_affinity = size;
//
//    return EDITOR_OK;
//}
//
//int
//editor_cursor_page_up(struct editor* e)
//{
//    assert(e != NULL);
//
//    for (long i = 0; i < e->height - 1; i++) {
//        editor_cursor_up(e);
//    }
//
//    return EDITOR_OK;
//}
//
//int
//editor_cursor_page_down(struct editor* e)
//{
//    assert(e != NULL);
//
//    for (long i = 0; i < e->height - 1; i++) {
//        editor_cursor_down(e);
//    }
//
//    return EDITOR_OK;
//}
