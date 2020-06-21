#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pt.c"

bool
test_pt(void)
{
    const char orig[] = "Hello World!";
    const char dest[] = "Hello New World!";

    struct pt pt = { 0 };
    pt_init(&pt, orig, strlen(orig));

    long orig_size = strlen(orig);
    long real_size = pt_size(&pt);
    if (orig_size != real_size) {
        fprintf(stderr, "wrong size: want %ld, got %ld\n", orig_size, real_size);
        return false;
    }

    char c = '\0';
    pt_get(&pt, 0, &c);
    if (c != 'H') {
        fprintf(stderr, "failed to get char: want %c, got %c (0x%02x)\n", 'H', c, c);
        return false;
    }

    pt_get(&pt, 4, &c);
    if (c != 'o') {
        fprintf(stderr, "failed to get char: want %c, got %c (0x%02x)\n", 'o', c, c);
        return false;
    }

    int rc = pt_get(&pt, 50, &c);
    if (rc != PT_EOF) {
        fprintf(stderr, "expected EOF for OOB pt_get\n");
        return false;
    }

    pt_insert(&pt, 6, 'N');
    pt_insert(&pt, 7, 'e');
    pt_insert(&pt, 8, 'w');
    pt_insert(&pt, 9, ' ');

    for (long i = 0; i < (long)strlen(dest); i++) {
        char c = '\0';
        if (pt_get(&pt, i, &c) != PT_OK) {
            fprintf(stderr, "error getting next pt char\n");
            pt_free(&pt);
            return false;
        }

        if (c != dest[i]) {
            fprintf(stderr, "mismatched chars: want %c, got %c (0x%02x)\n", dest[i], c, c);
            pt_free(&pt);
            return false;
        }
    }

    pt_free(&pt);
    return true;
}
