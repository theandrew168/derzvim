#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include "array.h"
#include "editor.h"
#include "line.h"
#include "term.h"

int
main(int argc, char* argv[])
{
    char* path = NULL;
    if (argc == 2) {
        path = argv[1];
    }

    struct editor e = { 0 };
    if (editor_init(&e, STDIN_FILENO, STDOUT_FILENO, path) != EDITOR_OK) {
        fprintf(stderr, "failed to init editor\n");
        return EXIT_FAILURE;
    }

    bool running = true;
    while (running) {
        // draw current editor state to the terminal
        editor_draw(&e);

        // wait for input
        int c = 0;
        if (editor_key_wait(&e, &c) != EDITOR_OK) {
            editor_free(&e);
            return EXIT_FAILURE;
        }

        // process the input
        switch (c) {
            // quit derzvim
            case CTRL_KEY('q'):
                running = false;
                break;
            // move the cursor left but not past the left edge
//            case KEY_ARROW_LEFT:
//                if (cx <= 0) break;
//                cx--;
//                line_pos--;
//                line_affinity--;
//                break;
//            // move the cursor right but not past the right edge OR end of line
//            case KEY_ARROW_RIGHT:
//                if (cx >= width - 1) break;
//                if (cx >= line->array.size - 1) break;  // will be "- 2" in normal mode
//                cx++;
//                line_pos++;
//                line_affinity++;
//                break;
//            // move the cursor up but not past the top edge
//            case KEY_ARROW_UP:
//                if (cy <= 0) break;
//                line = line->prev;
//                if (line_affinity >= line->array.size) cx = line->array.size - 1;
//                else cx = line_affinity;
//                cy--;
//                break;
//            // move the cursor down but not past the bottom edge OR end of file
//            case KEY_ARROW_DOWN:
//                if (cy >= height - 1) break;
//                if (cy >= line_count - 1) break;
//                line = line->next;
//                if (line_affinity >= line->array.size) cx = line->array.size - 1;
//                else cx = line_affinity;
//                cy++;
//                break;
//            // move the cursor back to the left edge
//            case KEY_HOME:
//                cx = 0;
//                line_pos = 0;
//                line_affinity = 0;
//                break;
//            // move the cursor the end of the current line
//            case KEY_END:
//                cx = line->array.size - 1;
//                line_pos = line->array.size - 1;
//                line_affinity = line->array.size - 1;
//                break;
//            // break the line at the current cursor pos
//            case KEY_ENTER:
//                if (cy >= height - 1) break;
//
//                struct line* cur = calloc(1, sizeof(struct line));
//                array_init(&cur->array);
//
//                // copy rest of existing line to the new one
//                for (long i = line_pos; i < line->array.size; i++) {
//                    array_append(&cur->array, line->array.buf[i]);
//                }
//
//                // delete rest of existing line but replace the NL
//                for (long i = line->array.size - 1; i >= line_pos; i--) {
//                    array_delete(&line->array, i);
//                }
//                array_append(&line->array, '\n');
//
//                cur->prev = line;
//                cur->next = line->next;
//                line->next = cur;
//
//                line = cur;
//                line_count++;
//
//                cx = 0;
//                line_pos = 0;
//                line_affinity = 0;
//                cy++;
//                break;
//            // delete the char behind the cursor if not at left edge AND collapse lines
//            case KEY_BACKSPACE:
//                if (cx == 0 && line->prev != NULL) {
//                    // delete NL from prev line
//                    cx = line->prev->array.size - 1;
//                    line_pos = line->prev->array.size - 1;
//                    line_affinity = line->prev->array.size - 1;
//                    array_delete(&line->prev->array, line->prev->array.size - 1);
//                    for (long i = 0; i < line->array.size; i++) {
//                        array_append(&line->prev->array, line->array.buf[i]);
//                    }
//                    struct line* old = line;
//                    if (line->next != NULL) line->next->prev = line->prev;
//                    line->prev->next = line->next;
//                    line = line->prev;
//                    line_count--;
//                    free(old);
//                    cy--;
//                } else if (cx > 0) {
//                    cx--;
//                    line_pos--;
//                    line_affinity--;
//                    array_delete(&line->array, cx);
//                }
//                break;
//            // insert a char at the current cursor pos (adjust line as necessary)
//            default:
//                if (c < 32 || c > 126) break;
//                array_insert(&line->array, cx, c);
//                cx++;
//                break;
        }
    }

    editor_free(&e);
    return EXIT_SUCCESS;
}
