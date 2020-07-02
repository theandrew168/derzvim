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

bool term_mode_raw(int input_fd);
bool term_mode_newline(int output_fd);
bool term_mode_linefeed(int output_fd);

bool term_line_wrap_on(int output_fd);
bool term_line_wrap_off(int output_fd);

bool term_scroll_smooth(int output_fd);
bool term_scroll_jump(int output_fd);
bool term_scroll_region_set(int output_fd, long top, long bottom);
bool term_scroll_region_on(int output_fd);
bool term_scroll_region_off(int output_fd);

bool term_cursor_up(int output_fd, long n, bool scroll);
bool term_cursor_down(int output_fd, long n, bool scroll);
bool term_cursor_right(int output_fd, long n);
bool term_cursor_left(int output_fd, long n);
bool term_cursor_pos_get(int output_fd, int input_fd, long* cx, long* cy);
bool term_cursor_pos_set(int output_fd, long cx, long cy);
bool term_cursor_pos_home(int output_fd);
bool term_cursor_next_line(int output_fd);
bool term_cursor_save(int output_fd);
bool term_cursor_restore(int output_fd);

bool term_erase_line_after(int output_fd);
bool term_erase_line_before(int output_fd);
bool term_erase_line(int output_fd);
bool term_erase_screen_after(int output_fd);
bool term_erase_screen_before(int output_fd);
bool term_erase_screen(int output_fd);

bool term_screen_write(int output_fd, char* buf, long size);

bool term_size(int output_fd, long* width, long* height);
bool term_key_wait(int input_fd, int* c);

#endif
