#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include "array.h"
#include "line.h"
#include "term.h"

int
main(int argc, char* argv[])
{
    // primary editor state
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    long width = 0;
    long height = 0;
    long scroll = 0;
    long cx = 0;
    long cy = 0;

    struct line* line = calloc(1, sizeof(struct line));
    array_init(&line->array);

    struct line* head = line;
    struct line* tail = line;
    long line_count = 0;
    long lx = 0;
    long ly = 0;

    // load a file if present on CLI
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "failed to open file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        // read through each character in the file
        int c = 0;
        while ((c = fgetc(fp)) != EOF) {
            array_append(&tail->array, c);

            // add a new line to the DLL when found
            if (c == '\n') {
                struct line* cur = calloc(1, sizeof(struct line));
                array_init(&cur->array);
                cur->prev = tail;
                tail->next = cur;
                tail = cur;

                line_count++;
            }
        }

        if (ferror(fp)) {
            fprintf(stderr, "IO error while reading file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }

        fclose(fp);
    } else {
        // start empty files with a single newline
        array_append(&tail->array, '\n');
        line_count++;
    }

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
    struct line* draw = head;
    for (long i = 0; i < height; i++) {
        if (draw == NULL) break;
        term_screen_write(output_fd, width, height, draw->array.buf, draw->array.size);
        draw = draw->next;        
    }
    term_cursor_show(output_fd);
    term_cursor_reset(output_fd);

    bool running = true;
    while (running) {
        // wait for input
        int c = 0;
        if (!term_key_wait(input_fd, &c)) {
            fprintf(stderr, "error waiting for input: %s\n", strerror(errno));
            tcsetattr(input_fd, TCSAFLUSH, &original_termios);
            // TODO free ALL lines
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
                if (cx >= line->array.size - 1) break;
                cx++;
                break;
            // move the cursor up but not past the top edge
            case KEY_ARROW_UP:
                if (cy <= 0) break;
                line = line->prev;
                if (cx >= line->array.size) cx = line->array.size - 1;
                cy--;
                break;
            // move the cursor down but not past the bottom edge OR end of file
            case KEY_ARROW_DOWN:
                if (cy >= height - 1) break;
                if (cy >= line_count - 1) break;
                line = line->next;
                if (cx >= line->array.size) cx = line->array.size - 1;
                cy++;
                break;
            // move the cursor back to the left edge
            case KEY_HOME:
                cx = 0;
                break;
            // move the cursor the end of the current line
            case KEY_END:
                cx = line->array.size - 1;
                break;
            // break the line at the current cursor pos
            case KEY_ENTER:
                if (cy >= height - 1) break;

                struct line* cur = calloc(1, sizeof(struct line));
                array_init(&cur->array);
                for (long i = cx; i < line->array.size; i++) {
                    array_append(&cur->array, line->array.buf[i]);
                }
                for (long i = line->array.size - 1; i >= cx; i--) {
                    array_delete(&line->array, i);
                }
                array_append(&line->array, '\n');
                cur->prev = line;
                cur->next = line->next;
                line->next = cur;

                line = line->next;
                line_count++;

                cx = 0;
                cy++;
                break;
            // delete the char behind the cursor if not at left edge AND collapse lines
            case KEY_BACKSPACE:
                if (cx == 0 && line->prev != NULL) {
                    // delete NL from prev line
                    cx = line->prev->array.size - 1;
                    array_delete(&line->prev->array, line->prev->array.size - 1);
                    for (long i = 0; i < line->array.size; i++) {
                        array_append(&line->prev->array, line->array.buf[i]);
                    }
                    struct line* old = line;
                    if (line->next != NULL) line->next->prev = line->prev;
                    line->prev->next = line->next;
                    line = line->prev;
                    line_count--;
                    free(old);
                    cy--;
                } else if (cx > 0) {
                    cx--;
                    array_delete(&line->array, cx);
                }
                break;
            // insert a char at the current cursor pos (adjust line as necessary)
            default:
                if (c < 32 || c > 126) break;
                array_insert(&line->array, cx, c);
                cx++;
                break;
        }

        // update the screen
        // for each line [offset:offset + height]:
        //  draw the line [0 to height] and handle wrapping (extra y++)
        term_cursor_reset(output_fd);
        term_cursor_hide(output_fd);
        term_screen_clear(output_fd);
        struct line* draw = head;
        for (long i = 0; i < height; i++) {
            if (draw == NULL) break;
            term_screen_write(output_fd, width, height, draw->array.buf, draw->array.size);
            draw = draw->next;        
        }
        term_cursor_show(output_fd);

        // reposition the cursor
        term_cursor_set(output_fd, cx, cy);
    }

    term_screen_clear(output_fd);
    term_cursor_reset(output_fd);

    // restore the original termios config
    tcsetattr(input_fd, TCSAFLUSH, &original_termios);

    // write the output.txt file
    FILE* fp = fopen("output.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open output file: output.txt\n");
    } else {
        for (struct line* write = head; write != NULL; write = write->next) {
            fwrite(write->array.buf, write->array.size, 1, fp);
        }
        fclose(fp);
    }

    // free the DLL of lines
    line = head;
    while (line != NULL) {
        struct line* current = line;
        line = line->next;
        array_free(&current->array);
        free(current);
    }

    return EXIT_SUCCESS;
}
