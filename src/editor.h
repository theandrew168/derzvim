#ifndef DERZVIM_EDITOR_H_INCLUDED
#define DERZVIM_EDITOR_H_INCLUDED

#include <termios.h>

#include "line.h"

// TODO: impl differ modes
// a struct of func ptrs, something like:
// typedef (*mode_handler)(struct editor* e, int c);
// or use more specific handler for different things?
// I think the first would scale better and be less restrictive
// TODO: impl a simple status bar (just use the last row)
// use it for messages and cursor pos thing like vim
struct editor {
    struct termios original_termios;

    long input_fd;
    long output_fd;

    long width;
    long height;
    long scroll;
    long cursor_x;
    long cursor_y;

    struct line* current_line;
    struct line* head;
    struct line* tail;
    long line_count;
    long line_pos;
    long line_affinity;
};

enum editor_status {
    EDITOR_OK = 0,
    EDITOR_ERROR,
};

int editor_init(struct editor* e, int input_fd, int output_fd, const char* path);
int editor_free(struct editor* e);

int editor_draw(const struct editor* e);

int editor_cursor_left(struct editor* e);
int editor_cursor_right(struct editor* e);
int editor_cursor_up(struct editor* e);
int editor_cursor_down(struct editor* e);

int editor_cursor_home(struct editor* e);
int editor_cursor_end(struct editor* e);
int editor_cursor_page_up(struct editor* e);
int editor_cursor_page_down(struct editor* e);

#endif
