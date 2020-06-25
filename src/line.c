#include <assert.h>
#include <stdlib.h>

#include "array.h"
#include "line.h"

int
line_init(struct line* line)
{
    assert(line != NULL);
    array_init(&line->array);
    return LINE_OK;
}

int
line_free(struct line* line)
{
    assert(line != NULL);
    array_free(&line->array);
    return LINE_OK;
}

long
line_size(const struct line* line)
{
    assert(line != NULL);
    return line->array.size;
}

char
line_get(const struct line* line, long pos)
{
    assert(line != NULL);
    return array_get(&line->array, pos);
}

int
line_append(struct line* line, char c)
{
    assert(line != NULL);
    array_append(&line->array, c);
    return LINE_OK;
}

int
line_insert(struct line* line, long pos, char c)
{
    assert(line != NULL);
    array_insert(&line->array, pos, c);
    return LINE_OK;
}

int
line_delete(struct line* line, long pos)
{
    assert(line != NULL);
    array_delete(&line->array, pos);
    return LINE_OK;
}

struct line*
line_prev(const struct line* line)
{
    assert(line != NULL);
    return line->prev;
}

struct line*
line_next(const struct line* line)
{
    assert(line != NULL);
    return line->next;
}
