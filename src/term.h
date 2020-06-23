#ifndef DERZVIM_TERM_H_INCLUDED
#define DERZVIM_TERM_H_INCLUDED

#include <stdbool.h>

#include "ab.h"

#define CTRL_KEY(k) ((k) & 0x1f)

enum key {
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_DEL,
};

bool term_size(int output_fd, long* width, long* height);
bool term_raw_mode(int input_fd);
bool term_cursor_pos(int input_fd, int output_fd, long* cx, long* cy);
bool term_read_key(int input_fd, int* c);

bool term_draw_rows(struct ab* ab, long width, long height);
bool term_screen_refresh(int output_fd, long width, long height, long cx, long cy);

#endif
