#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term.h"

// This terminal API is compatible with a VT100 + ANSI colors.

// References:
// http://braun-home.net/michael/info/misc/VT100_commands.htm
// http://www.termsys.demon.co.uk/vtansi.htm
// https://en.wikipedia.org/wiki/ANSI_escape_code

// Scrolling Functions
#define TERM_SCROLL_REGION_SET "\033[%ld;%ldr"
#define TERM_SCROLL_REGION_ON  "\033[?6h"
#define TERM_SCROLL_REGION_OFF "\033[?6l"

// Cursor Functions
#define TERM_CURSOR_UP          "\033[%ldA"
#define TERM_CURSOR_DOWN        "\033[%ldB"
#define TERM_CURSOR_RIGHT       "\033[%ldC"
#define TERM_CURSOR_LEFT        "\033[%ldD"
#define TERM_CURSOR_POS_SET     "\033[%ld;%ldH"
#define TERM_CURSOR_POS_HOME    "\033[H"
#define TERM_CURSOR_DOWN_SCROLL "\033D"
#define TERM_CURSOR_UP_SCROLL   "\033M"
#define TERM_CURSOR_NEXT_LINE   "\033E"
#define TERM_CURSOR_SAVE        "\0337"
#define TERM_CURSOR_RESTORE     "\0338"

// Erasing
#define TERM_ERASE_LINE_AFTER    "\033[0K"
#define TERM_ERASE_LINE_BEFORE   "\033[1K"
#define TERM_ERASE_LINE          "\033[2K"
#define TERM_ERASE_SCREEN_AFTER  "\033[0J"
#define TERM_ERASE_SCREEN_BEFORE "\033[1J"
#define TERM_ERASE_SCREEN        "\033[2J"

// Setup Functions
#define TERM_SCROLL_SMOOTH "\033[?4h"
#define TERM_SCROLL_JUMP   "\033[?4l"
#define TERM_LINE_WRAP_ON  "\033[?7h"
#define TERM_LINE_WRAP_OFF "\033[?7l"
#define TERM_MODE_NEWLINE  "\033[20h"
#define TERM_MODE_LINEFEED "\033[20l"

// ANSI Colors
#define TERM_COLOR_RESET "\033[0m"

#define TERM_COLOR_BLACK   "\033[30m"
#define TERM_COLOR_RED     "\033[31m"
#define TERM_COLOR_GREEN   "\033[32m"
#define TERM_COLOR_YELLOW  "\033[33m"
#define TERM_COLOR_BLUE    "\033[34m"
#define TERM_COLOR_MAGENTA "\033[35m"
#define TERM_COLOR_CYAN    "\033[36m"
#define TERM_COLOR_WHITE   "\033[37m"

#define TERM_COLOR_BG_BLACK   "\033[40m"
#define TERM_COLOR_BG_RED     "\033[41m"
#define TERM_COLOR_BG_GREEN   "\033[42m"
#define TERM_COLOR_BG_YELLOW  "\033[43m"
#define TERM_COLOR_BG_BLUE    "\033[44m"
#define TERM_COLOR_BG_MAGENTA "\033[45m"
#define TERM_COLOR_BG_CYAN    "\033[46m"
#define TERM_COLOR_BG_WHITE   "\033[47m"

bool
term_mode_raw(int input_fd)
{
    struct termios raw;
    if (tcgetattr(input_fd, &raw) == -1) return false;

    // Section: Raw mode
    // https://man7.org/linux/man-pages/man3/termios.3.html
    raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |=  (CS8);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(input_fd, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

bool
term_mode_newline(int output_fd)
{
    long size = strlen(TERM_MODE_NEWLINE);
    if (write(output_fd, TERM_MODE_NEWLINE, size) != size) return false;
    return true;
}

bool
term_mode_linefeed(int output_fd)
{
    long size = strlen(TERM_MODE_LINEFEED);
    if (write(output_fd, TERM_MODE_LINEFEED, size) != size) return false;
    return true;
}

bool
term_line_wrap_on(int output_fd)
{
    long size = strlen(TERM_LINE_WRAP_ON);
    if (write(output_fd, TERM_LINE_WRAP_ON, size) != size) return false;
    return true;
}

bool
term_line_wrap_off(int output_fd)
{
    long size = strlen(TERM_LINE_WRAP_OFF);
    if (write(output_fd, TERM_LINE_WRAP_OFF, size) != size) return false;
    return true;
}

bool
term_scroll_smooth(int output_fd)
{
    long size = strlen(TERM_SCROLL_SMOOTH);
    if (write(output_fd, TERM_SCROLL_SMOOTH, size) != size) return false;
    return true;
}

bool
term_scroll_jump(int output_fd)
{
    long size = strlen(TERM_SCROLL_JUMP);
    if (write(output_fd, TERM_SCROLL_JUMP, size) != size) return false;
    return true;
}

bool
term_scroll_region_set(int output_fd, long top, long bottom)
{
    char buf[80] = { 0 };
    long size = snprintf(buf, sizeof(buf), TERM_SCROLL_REGION_SET, top, bottom);
    if (write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_scroll_region_on(int output_fd)
{
    long size = strlen(TERM_SCROLL_REGION_ON);
    if (write(output_fd, TERM_SCROLL_REGION_ON, size) != size) return false;
    return true;
}

bool
term_scroll_region_off(int output_fd)
{
    long size = strlen(TERM_SCROLL_REGION_OFF);
    if (write(output_fd, TERM_SCROLL_REGION_OFF, size) != size) return false;
    return true;
}

bool
term_cursor_up(int output_fd, long n, bool scroll)
{
    if (scroll) {
        // TODO: do the move + scroll ops support a count?
        for (long i = 0; i < n; i++) {
            long size = strlen(TERM_CURSOR_UP_SCROLL);
            if (write(output_fd, TERM_CURSOR_UP_SCROLL, size) != size) return false;
        }
    } else {
        char buf[80] = { 0 };
        long size = snprintf(buf, sizeof(buf), TERM_CURSOR_UP, n);
        if (write(output_fd, buf, size) != size) return false;
    }
    return true;
}

bool
term_cursor_down(int output_fd, long n, bool scroll)
{
    if (scroll) {
        // TODO: do the move + scroll ops support a count?
        for (long i = 0; i < n; i++) {
            long size = strlen(TERM_CURSOR_DOWN_SCROLL);
            if (write(output_fd, TERM_CURSOR_DOWN_SCROLL, size) != size) return false;
        }
    } else {
        char buf[80] = { 0 };
        long size = snprintf(buf, sizeof(buf), TERM_CURSOR_DOWN, n);
        if (write(output_fd, buf, size) != size) return false;
    }
    return true;
}

bool
term_cursor_right(int output_fd, long n)
{
    char buf[80] = { 0 };
    long size = snprintf(buf, sizeof(buf), TERM_CURSOR_RIGHT, n);
    if (write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_cursor_left(int output_fd, long n)
{
    char buf[80] = { 0 };
    long size = snprintf(buf, sizeof(buf), TERM_CURSOR_LEFT, n);
    if (write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_cursor_pos_set(int output_fd, long cx, long cy)
{
    char buf[80] = { 0 };
    long size = snprintf(buf, sizeof(buf), TERM_CURSOR_POS_SET, cy + 1, cx + 1);
    if(write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_cursor_pos_home(int output_fd)
{
    long size = strlen(TERM_CURSOR_POS_HOME);
    if (write(output_fd, TERM_CURSOR_POS_HOME, size) != size) return false;
    return true;
}

bool
term_cursor_next_line(int output_fd)
{
    long size = strlen(TERM_CURSOR_NEXT_LINE);
    if (write(output_fd, TERM_CURSOR_NEXT_LINE, size) != size) return false;
    return true;
}

bool
term_cursor_save(int output_fd)
{
    long size = strlen(TERM_CURSOR_SAVE);
    if (write(output_fd, TERM_CURSOR_SAVE, size) != size) return false;
    return true;
}

bool
term_cursor_restore(int output_fd)
{
    long size = strlen(TERM_CURSOR_RESTORE);
    if (write(output_fd, TERM_CURSOR_RESTORE, size) != size) return false;
    return true;
}

bool
term_erase_line_after(int output_fd)
{
    long size = strlen(TERM_ERASE_LINE_AFTER);
    if (write(output_fd, TERM_ERASE_LINE_AFTER, size) != size) return false;
    return true;
}

bool
term_erase_line_before(int output_fd)
{
    long size = strlen(TERM_ERASE_LINE_BEFORE);
    if (write(output_fd, TERM_ERASE_LINE_BEFORE, size) != size) return false;
    return true;
}

bool
term_erase_line(int output_fd)
{
    long size = strlen(TERM_ERASE_LINE);
    if (write(output_fd, TERM_ERASE_LINE, size) != size) return false;
    return true;
}

bool
term_erase_screen_after(int output_fd)
{
    long size = strlen(TERM_ERASE_SCREEN_AFTER);
    if (write(output_fd, TERM_ERASE_SCREEN_AFTER, size) != size) return false;
    return true;
}

bool
term_erase_screen_before(int output_fd)
{
    long size = strlen(TERM_ERASE_SCREEN_BEFORE);
    if (write(output_fd, TERM_ERASE_SCREEN_BEFORE, size) != size) return false;
    return true;
}

bool
term_erase_screen(int output_fd)
{
    long size = strlen(TERM_ERASE_SCREEN);
    if (write(output_fd, TERM_ERASE_SCREEN, size) != size) return false;
    return true;
}

bool
term_screen_write(int output_fd, char* buf, long size)
{
    if (write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_size(int output_fd, long* width, long* height)
{
    struct winsize ws;
    if (ioctl(output_fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return false;
    }

    *width = ws.ws_col;
    *height = ws.ws_row;

    return true;
}

bool
term_key_wait(int input_fd, int* c)
{
    if (c == NULL) return false;
    *c = 0;

    long n = 0;
    while ((n = read(input_fd, c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN) {
            return false;
        }
    }

    // check for extra data beyond the escape, else just
    // return the lone escape char. TODO: clean this up?
    if (*c == KEY_ESCAPE) {
        char seq[3] = { 0 };
        if (read(input_fd, &seq[0], 1) != 1) return true;
        if (read(input_fd, &seq[1], 1) != 1) return true;
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                // clean page up / down keys
                if (read(input_fd, &seq[2], 1) != 1) return true;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': *c = KEY_HOME; return true;
                        case '3': *c = KEY_DEL; return true;
                        case '4': *c = KEY_END; return true;
                        case '5': *c = KEY_PAGE_UP; return true;
                        case '6': *c = KEY_PAGE_DOWN; return true;
                        case '7': *c = KEY_HOME; return true;
                        case '8': *c = KEY_END; return true;
                    }
                }
            } else {
                // clean arrow keys
                switch (seq[1]) {
                    case 'A': *c = KEY_ARROW_UP; return true;
                    case 'B': *c = KEY_ARROW_DOWN; return true;
                    case 'C': *c = KEY_ARROW_RIGHT; return true;
                    case 'D': *c = KEY_ARROW_LEFT; return true;
                    case 'H': *c = KEY_HOME; return true;
                    case 'F': *c = KEY_END; return true;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': *c = KEY_HOME; return true;
                case 'F': *c = KEY_END; return true;
            }
        }
    }

    return true;
}
