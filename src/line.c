#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "line.h"

enum {
    LINE_DEFAULT_CAPACITY = 256,
    LINE_CAPACITY_GROWTH = 2,
};

int
line_init(struct line* line)
{
    assert(line != NULL);

    line->capacity = LINE_DEFAULT_CAPACITY;
    line->size = 0;
    line->buf = malloc(LINE_DEFAULT_CAPACITY);
    if (line->buf == NULL) {
        fprintf(stderr, "line: failed to allocate initial buffer\n");
        return LINE_ERROR;
    }

    return LINE_OK;
}

int
line_free(struct line* line)
{
    assert(line != NULL);
    free(line->buf);
    return LINE_OK;
}

char
line_get(const struct line* line, long index)
{
    assert(line != NULL);
    assert(index >= 0);
    assert(index < line->size);

    return line->buf[index];
}

int
line_append(struct line* line, char c)
{
    assert(line != NULL);

    line_insert(line, line->size, c);

    return LINE_OK;
}

int
line_insert(struct line* line, long pos, char c)
{
    assert(line != NULL);
    assert(pos >= 0);
    assert(pos <= line->size); // pos can equal size here to imply inserting at the end

    // grow the buffer if current capacity is reached
    if (line->size >= line->capacity) {
        line->capacity *= LINE_CAPACITY_GROWTH;
        line->buf = realloc(line->buf, line->capacity);
        if (line->buf == NULL) return LINE_ERROR;
    }

    // shift buffer contents up
    memmove(&line->buf[pos + 1], &line->buf[pos], line->size - pos);

    // insert the new character
    line->buf[pos] = c;
    line->size++;

    return LINE_OK;
}

int
line_delete(struct line* line, long pos)
{
    assert(line != NULL);
    assert(pos >= 0);
    assert(pos < line->size);

    // shift buffer contents down
    memmove(&line->buf[pos], &line->buf[pos + 1], line->size - pos);
    line->size--;

    return LINE_OK;
}
