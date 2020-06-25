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
