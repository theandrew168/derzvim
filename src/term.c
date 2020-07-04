#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term.h"

// Non-standard escape codes
#define TERM_CURSOR_SHOW    "\033[?25h"
#define TERM_CURSOR_HIDE    "\033[?25l"

#define TERM_SCREEN_SAVE    "\033[?47h"
#define TERM_SCREEN_RESTORE "\033[?47l"

// VT100 escape codes
#define TERM_CURSOR_POS_GET "\033[6n"
#define TERM_CURSOR_POS_SET "\033[%ld;%ldH"
#define TERM_CURSOR_SAVE    "\0337"
#define TERM_CURSOR_RESTORE "\0338"

#define TERM_ERASE_LINE     "\033[2K"
#define TERM_ERASE_SCREEN   "\033[2J"

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
    // Linux-specific

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
term_screen_save(int output_fd)
{
    long size = strlen(TERM_SCREEN_SAVE);
    if (write(output_fd, TERM_SCREEN_SAVE, size) != size) return false;
    return true;
}

bool
term_screen_restore(int output_fd)
{
    long size = strlen(TERM_SCREEN_RESTORE);
    if (write(output_fd, TERM_SCREEN_RESTORE, size) != size) return false;
    return true;
}

bool
term_cursor_pos_get(int input_fd, int output_fd, long* cx, long* cy)
{
    // send an escape sequence to output_fd and then
    // read an escape sequence back on input_fd

    long size = strlen(TERM_CURSOR_POS_GET);
    if (write(output_fd, TERM_CURSOR_POS_GET, size) != size) return false;

    char buf[32] = { 0 };
    unsigned long i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(input_fd, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    // parse the cursor pos from the resulting escape sequence
    if (buf[0] != KEY_ESCAPE || buf[1] != '[') return false;
    if (sscanf(&buf[2], "%ld;%ld", cy, cx) != 2) return false;

    // derzvim uses 0-based indexing; termios stuff uses 1-based
    *cx -= 1;
    *cy -= 1;

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
term_cursor_show(int output_fd)
{
    long size = strlen(TERM_CURSOR_SHOW);
    if (write(output_fd, TERM_CURSOR_SHOW, size) != size) return false;
    return true;
}

bool
term_cursor_hide(int output_fd)
{
    long size = strlen(TERM_CURSOR_HIDE);
    if (write(output_fd, TERM_CURSOR_HIDE, size) != size) return false;
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
term_erase_line(int output_fd)
{
    long size = strlen(TERM_ERASE_LINE);
    if (write(output_fd, TERM_ERASE_LINE, size) != size) return false;
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
term_write(int output_fd, char* buf, long size)
{
    if (write(output_fd, buf, size) != size) return false;
    return true;
}

bool
term_size(int output_fd, long* width, long* height)
{
    // Linux-specific

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
