#include <stdlib.h>
#include <string.h>

#include "ab.h"

int
ab_append(struct ab* ab, const char* s, long size)
{
    // alloc a new buffer that is big enough to hold everything
    char* buf = realloc(ab->buf, ab->size + size);
    if (buf == NULL) return AB_ERROR;

    // append the new contents onto the new buffer
    memmove(&buf[ab->size], s, size);

    // update the struct ab with the new buffer and size
    ab->buf = buf;
    ab->size += size;

    return AB_OK;
}

void
ab_free(struct ab* ab)
{
    free(ab->buf);
}
