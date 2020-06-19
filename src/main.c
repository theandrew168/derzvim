#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define DERZVIM_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

struct ab {
    char* buf;
    long size;
};

static bool
ab_append(struct ab* ab, const char* s, long size)
{
    // alloc a new buffer that is big enough to hold everything
    char* buf = realloc(ab->buf, ab->size + size);
    if (buf == NULL) return false;

    // append the new contents onto the new buffer
    memmove(&buf[ab->size], s, size);

    // update the struct ab with the new buffer and size
    ab->buf = buf;
    ab->size += size;

    return true;
}

static void
ab_free(struct ab* ab)
{
    free(ab->buf);
}

static bool
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

static bool
term_read_key(int input_fd, char* c)
{
    if (c == NULL) return false;

    long n = 0;
    while ((n = read(input_fd, c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN) {
            return false;
        }
    }

    return true;
}

static bool
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

static bool
term_screen_refresh(int output_fd, long width, long height)
{
    struct ab ab = { 0 };

    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);
    term_draw_rows(&ab, width, height);
    ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);

    if (write(output_fd, ab.buf, ab.size) != ab.size) {
        return false;
    }

    ab_free(&ab);
    return true;
}

static bool
term_cursor_pos(int input_fd, int output_fd, long* x, long* y)
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
    if (sscanf(&buf[2], "%ld;%ld", y, x) != 2) return false;

    return true;
}

static bool
term_size(int input_fd, int output_fd, long* width, long* height)
{
    // attempt ioctl "easy" method first but fallback to moving the cursor
    // to the bottom-right of the screen and reading its position

    struct winsize ws;
    if (ioctl(output_fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
//        if (write(output_fd, "\x1b[999C\x1b[999B", 12) != 12) return false;
//        return term_cursor_pos(input_fd, output_fd, width, height);
        return false;
    }

    *width = ws.ws_col;
    *height = ws.ws_row;

    return true;
}

int
main(int argc, char* argv[])
{
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    long width = 0;
    long height = 0;
    long cursor_x = 0;
    long cursor_y = 0;

    if (!term_size(input_fd, output_fd, &width, &height)) {
        fprintf(stderr, "error getting term size: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct termios default_termios;
    if (tcgetattr(input_fd, &default_termios) == -1) {
        fprintf(stderr, "error capturing default termios: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_raw_mode(input_fd)) {
        fprintf(stderr, "error enabling raw mode: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_screen_refresh(output_fd, width, height)) {
        fprintf(stderr, "error refreshing screen\n");
        tcsetattr(input_fd, TCSAFLUSH, &default_termios);
        return EXIT_FAILURE;
    }

//    printf("size: %ld, %ld\r\n", width, height);
//    if (term_cursor_pos(input_fd, output_fd, &cursor_x, &cursor_y)) {
//        printf("cursor: %ld, %ld\r\n", cursor_x, cursor_y);
//    }

    term_read_key(input_fd, NULL);

    bool running = true;
    while (running) {
        char c = '\0';
        if (!term_read_key(input_fd, &c)) {
            fprintf(stderr, "error reading key: %s\n", strerror(errno));
            tcsetattr(input_fd, TCSAFLUSH, &default_termios);
            return EXIT_FAILURE;
        }

        switch (c) {
        case CTRL_KEY('q'):
            running = false;
            break;
//        default:
//            if (iscntrl(c)) {
//                printf("%d\r\n", c);
//            } else {
//                printf("%d ('%c')\r\n", c, c);
//            }
        }
    }

    if (write(output_fd, "\x1b[2J", 4) != 4) {
        // stub
    }
    if (write(output_fd, "\x1b[H", 3) != 3) {
        // stub
    }

    tcsetattr(input_fd, TCSAFLUSH, &default_termios);
    return EXIT_SUCCESS;
}
