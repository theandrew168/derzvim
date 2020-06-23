#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

enum {
    ARRAY_DEFAULT_CAPACITY = 256,
    ARRAY_CAPACITY_GROWTH = 2,
};

int
array_init(struct array* array)
{
    assert(array != NULL);

    array->capacity = ARRAY_DEFAULT_CAPACITY;
    array->size = 0;
    array->buf = malloc(ARRAY_DEFAULT_CAPACITY);

    if (array->buf == NULL) {
        fprintf(stderr, "array: failed to allocate initial buffer\n");
        return ARRAY_ERROR;
    }

    return ARRAY_OK;
}

int
array_free(struct array* array)
{
    assert(array != NULL);
    free(array->buf);
    return ARRAY_OK;
}

char
array_get(const struct array* array, long index)
{
    assert(array != NULL);
    assert(index >= 0);
    assert(index < array->size);
    
    return array->buf[index];
}

long
array_size(const struct array* array)
{
    assert(array != NULL);
    return array->size;
}

char*
array_buffer(const struct array* array)
{
    return array->buf;
}

int
array_insert(struct array* array, long index, char c)
{
    assert(array != NULL);
    assert(index >= 0);
    assert(index <= array->size);

    if (array->size == array->capacity) {
        array->capacity *= ARRAY_CAPACITY_GROWTH;
        array->buf = realloc(array->buf, array->capacity);
        if (array->buf == NULL) return ARRAY_ERROR;
    }
    
    // shift array contents up
    memmove(&array->buf[index + 1], &array->buf[index], array->size - index);
    array->buf[index] = c;
    array->size++;

    return ARRAY_OK;
}

int
array_delete(struct array* array, long index)
{
    assert(array != NULL);
    assert(index >= 0);
    assert(index < array->size);
    
    // shift array contents down
    memmove(&array->buf[index], &array->buf[index + 1], array->size - index);
    array->size--;

    return ARRAY_OK;
}

int
array_append(struct array* array, char c)
{
    assert(array != NULL);
    return array_insert(array, array->size, c);
}
