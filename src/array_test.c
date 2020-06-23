#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "array.c"

bool
test_array_insert(void)
{
    struct array array = { 0 };
    array_init(&array);

    if (array_insert(&array, 0, '@') != ARRAY_OK) {
        fprintf(stderr, "test_array_insert: failed to insert at start of array\n");
        return false;
    }

    if (array_size(&array) != 1) {
        fprintf(stderr, "test_array_insert: size != 1\n");
        return false;
    }

    if (array_get(&array, 0) != '@') {
        fprintf(stderr, "test_array_insert: wrong char at index zero\n");
        return false;
    }

    array_free(&array);
    return true;
}
