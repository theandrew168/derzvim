#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include "array.h"
#include "term.h"

int
main(int argc, char* argv[])
{
    char* orig_buf = "";
    long orig_size = 0;

    // load a file if present on CLI
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        fseek(fp, 0, SEEK_END);
        orig_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        orig_buf = malloc(orig_size);
        if (orig_buf == NULL) {
            fclose(fp);
            fprintf(stderr, "failed to allocate buffer for file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        long count = fread(orig_buf, 1, orig_size, fp);
        if (count != orig_size) {
            free(orig_buf);
            fclose(fp);
            fprintf(stderr, "failed to read file into buffer: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        fclose(fp);
    }

    // primary editor state
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    long width = 0;
    long height = 0;
    long scroll = 0;
    long cx = 0;
    long cy = 0;
    long lx = 0;
    long ly = 0;
    // TODO: DDL of lines?

    // capture the original termios config to restore later
    struct termios original_termios;
    if (tcgetattr(input_fd, &original_termios) == -1) {
        fprintf(stderr, "error capturing default termios: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // TODO: handle resizing via sigwinch and volatile sig_atomic_t?
    if (!term_size(output_fd, &width, &height)) {
        fprintf(stderr, "error getting term size: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // put the terminal into raw mode
    if (!term_mode_raw(input_fd)) {
        fprintf(stderr, "error enabling raw mode: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // initially, clear the screen and maybe draw some tildes?
    term_cursor_reset(output_fd);
    term_cursor_hide(output_fd);
    term_screen_clear(output_fd);
    term_screen_write(output_fd, width, height, "hello world", 11);
    term_cursor_show(output_fd);
    term_cursor_reset(output_fd);

    bool running = true;
    while (running) {
        // wait for input
        int c = 0;
        if (!term_key_wait(input_fd, &c)) {
            fprintf(stderr, "error waiting for input: %s\n", strerror(errno));
            tcsetattr(input_fd, TCSAFLUSH, &original_termios);
            if (orig_size > 0) free(orig_buf);
            return EXIT_FAILURE;
        }

        // process the input
        switch (c) {
            // quit derzvim
            case CTRL_KEY('q'):
                running = false;
                break;
            // move the cursor left but not past the left edge
            case KEY_ARROW_LEFT:
                if (cx <= 0) break;
                cx--;
                break;
            // move the cursor right but not past the right edge OR end of line
            case KEY_ARROW_RIGHT:
                if (cx >= width - 1) break;
                cx++;
                break;
            // move the cursor up but not past the top edge
            case KEY_ARROW_UP:
                if (cy <= 0) break;
                cy--;
                break;
            // move the cursor down but not past the bottom edge OR end of file
            case KEY_ARROW_DOWN:
                if (cy >= height - 1) break;
                cy++;
                break;
            // break the line at the current cursor pos
            case KEY_ENTER:
                if (cy >= height - 1) break;
                cy++;
                break;
            // delete the char behind the cursor if not at left edge AND collapse lines
            case KEY_BACKSPACE:
                if (cx <= 0) break;
                cx--;
                break;
            // insert a char at the current cursor pos (adjust line as necessary)
            default:
                if (c < 32 || c > 126) break;
                break;
        }

        // update the screen
        // for each line [offset:offset + height]:
        //  draw the line [0 to height] and handle wrapping (extra y++)
        term_cursor_reset(output_fd);
        term_cursor_hide(output_fd);
        term_screen_clear(output_fd);
        term_screen_write(output_fd, width, height, "hello world", 11);
        term_cursor_show(output_fd);

        // reposition the cursor
        term_cursor_set(output_fd, cx, cy);
    }

    term_screen_clear(output_fd);
    term_cursor_reset(output_fd);

    // restore the original termios config
    tcsetattr(input_fd, TCSAFLUSH, &original_termios);

    // free the original text buffer if it came from a file (and malloc)
    if (orig_size > 0) free(orig_buf);

    return EXIT_SUCCESS;
}
