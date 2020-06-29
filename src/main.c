#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX 2008 dependencies
#include <termios.h>
#include <unistd.h>

// Lua 5.1 dependencies
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifndef LUA_OK
#define LUA_OK 0
#endif

#include "editor.h"
#include "line.h"
#include "term.h"

// TODO page up / page down
// TODO delete key
// TODO tabs
// TODO write back real file

#include <stdarg.h>
void error(lua_State* L, const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    lua_close(L);
    exit(EXIT_FAILURE);
}

enum plugin_status {
    PLUGIN_OK = 0,
    PLUGIN_ERROR,
};

#include <assert.h>
const char* on_line_draw(lua_State* L, const char* line, long size) {
    assert(L != NULL);
    assert(line != NULL);

    lua_getglobal(L, "on_line_draw");
    lua_pushlstring(L, line, size);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        error(L, "error running func: %s\n", lua_tostring(L, -1));
    }

    const char* res = lua_tostring(L, -1);
    printf("res: %s\n", res);
    lua_pop(L, 1);

    return NULL;
}

double f(lua_State* L, double x, double y) {
    lua_getglobal(L, "f");
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        error(L, "error running func: %s\n", lua_tostring(L, -1));
    }

    double z = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return z;
}

int
main(int argc, char* argv[])
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_loadfile(L, "plugin.lua") || lua_pcall(L, 0, 0, 0)) {
        error(L, "cannot load plugin: %s\n", lua_tostring(L, -1));
    }

    const char* line = on_line_draw(L, "foobar", strlen("foobar"));

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
    lua_close(L);
    return EXIT_SUCCESS;
}
