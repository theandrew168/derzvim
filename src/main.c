#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct derzvim {
    int input_fd;
    int output_fd;
    long width;
    long height;
};

static bool
term_raw_mode(const struct derzvim* dv)
{
    struct termios raw;
    if (tcgetattr(dv->input_fd, &raw) == -1) return false;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(dv->input_fd, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

static bool
term_read_key(const struct derzvim* dv, char* c)
{
    long n = 0;
    while ((n = read(dv->input_fd, c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN) {
            return false;
        }
    }

    return true;
}

static bool
term_draw_rows(const struct derzvim* dv)
{
    for (long y = 0; y < dv->height; y++) {
        if (write(dv->output_fd, "~\r\n", 3) != 3) return false;
    }
    if (write(dv->output_fd, "~", 1) != 1) return false;

    return true;
}

static bool
term_screen_refresh(const struct derzvim* dv)
{
    if (write(dv->output_fd, "\x1b[2J", 4) != 4) return false;
    if (write(dv->output_fd, "\x1b[H", 3) != 3) return false;

    if (!term_draw_rows(dv)) return false;

    if (write(dv->output_fd, "\x1b[H", 3) != 3) return false;

    return true;
}

static bool
term_size(struct derzvim* dv)
{
    struct winsize ws;
    if (ioctl(dv->output_fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return false;
    }

    dv->width = ws.ws_col;
    dv->height = ws.ws_row;

    return true;
}

int
main(int argc, char* argv[])
{
    struct derzvim dv = {
        .input_fd = STDIN_FILENO,
        .output_fd = STDOUT_FILENO,
    };
    if (!term_size(&dv)) {
        fprintf(stderr, "error getting term size: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct termios default_termios;
    if(tcgetattr(dv.input_fd, &default_termios) == -1) {
        fprintf(stderr, "error capturing default termios: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_raw_mode(&dv)) {
        fprintf(stderr, "error enabling raw mode: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_screen_refresh(&dv)) {
        fprintf(stderr, "error refreshing screen\n");
        tcsetattr(dv.input_fd, TCSAFLUSH, &default_termios);
        return EXIT_FAILURE;
    }

    bool running = true;
    while (running) {
        char c = '\0';
        if (!term_read_key(&dv, &c)) {
            fprintf(stderr, "error reading key: %s\n", strerror(errno));
            tcsetattr(dv.input_fd, TCSAFLUSH, &default_termios);
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

    assert(write(dv.output_fd, "\x1b[2J", 4) == 4);
    assert(write(dv.output_fd, "\x1b[H", 3) == 3);
    tcsetattr(dv.input_fd, TCSAFLUSH, &default_termios);
    return EXIT_SUCCESS;
}
