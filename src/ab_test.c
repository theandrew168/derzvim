#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ab.c"

bool
test_ab(void)
{
    struct ab ab = { 0 };
    ab_append(&ab, "foo", 3);
    ab_append(&ab, " ", 1);
    ab_append(&ab, "bar", 3);

    if (ab.size != 7) {
        fprintf(stderr, "invalid ab size: want %d, got %ld\n", 7, ab.size);
        ab_free(&ab);
        return false;
    }

    if (strncmp(ab.buf, "foo bar", 7) != 0) {
        fprintf(stderr, "invalid ab contents: expected 'foo_bar'");
        ab_free(&ab);
        return false;
    }

    ab_free(&ab);
    return true;
}
