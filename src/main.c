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

// TODO line wrap
// TODO tabs
// TODO write back real file
// TODO lazy loading (how does dirty text work here? mmap or something?) or nah?

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
            case CTRL_KEY('q'):
                running = false;
                break;
            case KEY_ARROW_LEFT:
                editor_cursor_left(&e);
                break;
            case KEY_ARROW_RIGHT:
                editor_cursor_right(&e);
                break;
            case KEY_ARROW_UP:
                editor_cursor_up(&e);
                break;
            case KEY_ARROW_DOWN:
                editor_cursor_down(&e);
                break;
            case KEY_HOME:
                editor_cursor_home(&e);
                break;
            case KEY_END:
                editor_cursor_end(&e);
                break;
            case KEY_ENTER:
                editor_line_break(&e);
                break;
            case KEY_BACKSPACE:
                editor_rune_delete(&e);
                break;
            default:
                if (c < 32 || c > 126) break;
                editor_rune_insert(&e, c);
                break;
        }
    }

    editor_free(&e);
    return EXIT_SUCCESS;
}
