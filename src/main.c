#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include "ab.h"
#include "pt.h"
#include "term.h"

int
main(int argc, char* argv[])
{
    char* buf = "";
    long size = 0;

    // load a file if present on CLI
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = malloc(size);
        if (buf == NULL) {
            fclose(fp);
            fprintf(stderr, "failed to allocate buffer for file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        long count = fread(buf, 1, size, fp);
        if (count != size) {
            free(buf);
            fclose(fp);
            fprintf(stderr, "failed to read file into buffer: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        fclose(fp);
    }

    struct pt pt = { 0 };
    pt_init(&pt, buf, size);

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    long width = 0;
    long height = 0;
    long cx = 0;
    long cy = 0;

    // capture the origina termios config to restore later
    struct termios original_termios;
    if (tcgetattr(input_fd, &original_termios) == -1) {
        fprintf(stderr, "error capturing default termios: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_size(output_fd, &width, &height)) {
        fprintf(stderr, "error getting term size: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!term_raw_mode(input_fd)) {
        fprintf(stderr, "error enabling raw mode: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    bool running = true;
    while (running) {
        if (!term_screen_refresh(output_fd, width, height, cx, cy)) {
            fprintf(stderr, "error refreshing screen\n");
            tcsetattr(input_fd, TCSAFLUSH, &original_termios);
            return EXIT_FAILURE;
        }

        int c = 0;
        if (!term_read_key(input_fd, &c)) {
            fprintf(stderr, "error reading key: %s\n", strerror(errno));
            tcsetattr(input_fd, TCSAFLUSH, &original_termios);
            return EXIT_FAILURE;
        }

        // what does a given key[combo] do in a given mode?
        // typedef some sort of handler func and make a nice table
        //   for each mode?
        switch (c) {
            case CTRL_KEY('q'):
                running = false;
                break;
            case KEY_ARROW_LEFT:
                if (cx > 0) cx--;
                break;
            case KEY_ARROW_RIGHT:
                if (cx < width - 1) cx++;
                break;
            case KEY_ARROW_UP:
                if (cy > 0) cy--;
                break;
            case KEY_ARROW_DOWN:
                if (cy < height - 1) cy++;
                break;
            case KEY_PAGE_UP:
                cy = 0;
                break;
            case KEY_PAGE_DOWN:
                cy = height - 1;
                break;
            case KEY_HOME:
                cx = 0;
                break;
            case KEY_END:
                cx = width - 1;
                break;
        }
    }

    if (write(output_fd, "\x1b[2J", 4) != 4) {
        // stub
    }
    if (write(output_fd, "\x1b[H", 3) != 3) {
        // stub
    }

    // restore the original termios config
    tcsetattr(input_fd, TCSAFLUSH, &original_termios);

    pt_free(&pt);

    // free the original text buffer if it came from a file (and malloc)
    if (size > 0) {
        free(buf);
    }

    return EXIT_SUCCESS;
}
