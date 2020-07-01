#ifndef DERZVIM_TERM_H_INCLUDED
#define DERZVIM_TERM_H_INCLUDED

#include <stdbool.h>

#define CTRL_KEY(k) ((k) & 0x1f)

enum key {
    KEY_TAB = 9,
    KEY_ENTER = 13,
    KEY_ESCAPE = 27,
    KEY_BACKSPACE = 127,
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
bool term_mode_raw(int input_fd);

bool term_screen_save(int output_fd);
bool term_screen_restore(int output_fd);
bool term_screen_clear(int output_fd);
bool term_screen_write(int output_fd, char* buf, long size);

bool term_screen_scroll_up(int output_fd);
bool term_screen_scroll_down(int output_fd);

bool term_row_clear(int output_fd);

bool term_cursor_save(int output_fd);
bool term_cursor_restore(int output_fd);
//bool term_cursor_get(int input_fd, int output_fd, long* cx, long* cy);
bool term_cursor_set(int output_fd, long cx, long cy);
bool term_cursor_reset(int output_fd);
bool term_cursor_hide(int output_fd);
bool term_cursor_show(int output_fd);

bool term_key_wait(int input_fd, int* c);

#endif
