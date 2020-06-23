#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "ab.h"
#include "term.h"

#define DERZVIM_VERSION "0.0.1"

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
term_raw_mode(int input_fd)
{
    struct termios raw;
    if (tcgetattr(input_fd, &raw) == -1) return false;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(input_fd, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

bool
term_cursor_pos(int input_fd, int output_fd, long* cx, long* cy)
{
    // send an escape sequence to output_fd and then
    // read an escape sequence back on input_fd

    char buf[32] = { 0 };
    if (write(output_fd, "\x1b[6n", 4) != 4) return false;

    unsigned long i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(input_fd, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    // parse the cursor pos from the resulting escape sequence
    if (buf[0] != '\x1b' || buf[1] != '[') return false;
    if (sscanf(&buf[2], "%ld;%ld", cy, cx) != 2) return false;

    // derzvim uses 0-based indexing; termios stuff uses 1-based
    *cx -= 1;
    *cy -= 1;

    return true;
}

bool
term_read_key(int input_fd, int* c)
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
    if (*c == '\x1b') {
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

bool
term_draw_rows(struct ab* ab, long width, long height)
{
    for (long y = 0; y < height; y++) {
        if (y == height / 3) {
            char welcome[80];
            long len = snprintf(welcome, sizeof(welcome), "Derzvim -- version %s", DERZVIM_VERSION);
            if (len > width) len = width;
            long padding = (width - len) / 2;
            if (padding) {
                ab_append(ab, "~", 1);
                padding--;
            }
            while (padding--) ab_append(ab, " ", 1);
            ab_append(ab, welcome, len);
        } else {
            ab_append(ab, "~", 1);
        }
        ab_append(ab, "\x1b[K", 3);

        // add CRNL on all lines except the last one
        if (y < height - 1) {
            ab_append(ab, "\r\n", 2);
        }
    }

    return true;
}

bool
term_screen_refresh(int output_fd, long width, long height, long cx, long cy)
{
    struct ab ab = { 0 };

    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);

    term_draw_rows(&ab, width, height);

    char buf[80] = { 0 };
    snprintf(buf, sizeof(buf), "\x1b[%ld;%ldH", cy + 1, cx + 1);
    ab_append(&ab, buf, strlen(buf));

    ab_append(&ab, "\x1b[?25h", 6);

    if (write(output_fd, ab.buf, ab.size) != ab.size) {
        return false;
    }

    ab_free(&ab);
    return true;
}
