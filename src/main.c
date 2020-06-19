#include <assert.h>
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

#define CTRL_KEY(k) ((k) & 0x1f)

static bool
term_raw_mode(int fd)
{
    struct termios raw;
    if (tcgetattr(fd, &raw) == -1) return false;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

static bool
term_read_key(int fd, char* c)
{
    long n = 0;
    while ((n = read(fd, c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN) {
            return false;
        }
    }

    return true;
}

static bool
term_draw_rows(int fd, long width, long height)
{
    for (long y = 0; y < height; y++) {
        if (write(fd, "~\r\n", 3) != 3) return false;
    }
    if (write(fd, "~", 1) != 1) return false;

    return true;
}

static bool
term_screen_refresh(int fd, long width, long height)
{
    if (write(fd, "\x1b[2J", 4) != 4) return false;
    if (write(fd, "\x1b[H", 3) != 3) return false;

    if (!term_draw_rows(fd, width, height)) return false;

    if (write(fd, "\x1b[H", 3) != 3) return false;

    return true;
}

static bool
term_size(int fd, long* width, long* height)
{
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return false;
    }

    *width = ws.ws_col;
    *height = ws.ws_row;

    return true;
}

void
handle_sigwinch(int signal)
{
    printf("caught SIGWINCH!\n");
}

int
main(int argc, char* argv[])
{
//    signal(SIGWINCH, handle_sigwinch);

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    long width = 0;
    long height = 0;

    if (!term_size(output_fd, &width, &height)) {
        fprintf(stderr, "error getting term size: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct termios default_termios;
    if(tcgetattr(input_fd, &default_termios) == -1) {
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

    assert(write(output_fd, "\x1b[2J", 4) == 4);
    assert(write(output_fd, "\x1b[H", 3) == 3);
    tcsetattr(input_fd, TCSAFLUSH, &default_termios);
    return EXIT_SUCCESS;
}
