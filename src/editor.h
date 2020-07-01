#ifndef DERZVIM_EDITOR_H_INCLUDED
#define DERZVIM_EDITOR_H_INCLUDED

#include <stdbool.h>

#include <termios.h>

#include "line.h"

// TODO: impl differ modes
// a struct of func ptrs, something like:
// typedef (*mode_handler)(struct editor* e, int c);
// or use more specific handler for different things?
// I think the first would scale better and be less restrictive
struct editor {
    struct termios original_termios;

    // TODO: should these be FILE* instead?
    long input_fd;
    long output_fd;

    long width;
    long height;
    // TODO: do I need these four?
    long scroll_x;
    long scroll_y;
    long cursor_x;
    long cursor_y;

    struct line* head;
    struct line* tail;

    struct line* line;
    long line_count;
    long line_index;  // TODO: do I need this?
    long line_pos;
    long line_affinity;
};

enum editor_status {
    EDITOR_OK = 0,
    EDITOR_ERROR,
};

int editor_init(struct editor* e, int input_fd, int output_fd, const char* path);
int editor_free(struct editor* e);

bool editor_run(struct editor* e);

int editor_draw(const struct editor* e);
int editor_key_wait(const struct editor* e, int* c);

int editor_rune_insert(struct editor* e, char rune);
int editor_rune_delete(struct editor* e);

int editor_line_break(struct editor* e);

int editor_cursor_left(struct editor* e);
int editor_cursor_right(struct editor* e);
int editor_cursor_up(struct editor* e);
int editor_cursor_down(struct editor* e);

int editor_cursor_home(struct editor* e);
int editor_cursor_end(struct editor* e);
int editor_cursor_page_up(struct editor* e);
int editor_cursor_page_down(struct editor* e);

#endif
