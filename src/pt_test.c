#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pt.c"

bool
test_pt_size(void)
{
    const char orig[] = "Hello New World";

    struct pt pt = { 0 };
    pt_init(&pt, orig, strlen(orig));

    long orig_size = strlen(orig);
    long real_size = pt_size(&pt);
    if (orig_size != real_size) {
        fprintf(stderr, "wrong size: want %ld, got %ld\n", orig_size, real_size);
        return false;
    }

    pt_free(&pt);
    return true;
}

bool
test_pt_get(void)
{
    const char orig[] = "Hello New World";

    struct pt pt = { 0 };
    pt_init(&pt, orig, strlen(orig));

    // basic pt_get
    char c = pt_get(&pt, 4);
    if (c != 'o') {
        fprintf(stderr, "failed to get char: want %c, got %c (0x%02x)\n", 'o', c, c);
        return false;
    }

    // out-of-bounds pt_get
    c = pt_get(&pt, 50);
    if (c != -1) {
        fprintf(stderr, "expected out of bounds error\n");
        return false;
    }

    pt_free(&pt);
    return true;
}

bool
test_pt_insert(void)
{
    const char orig[] = "Hello New World";

    struct pt pt = { 0 };
    pt_init(&pt, orig, strlen(orig));

    // insert at start of entry
    pt_insert(&pt, 0, '1');
    char c = pt_get(&pt, 0);
    if (c != '1') {
        fprintf(stderr, "insert failed at start of entry\n");
        return false;
    }

    // insert at end of entry
    pt_insert(&pt, pt_size(&pt), '2');
    c = pt_get(&pt, pt_size(&pt) - 1);
    if (c != '2') {
        fprintf(stderr, "insert failed at end of entry\n");
        return false;
    }

    // insert at end of entry again
    pt_insert(&pt, pt_size(&pt), '3');
    c = pt_get(&pt, pt_size(&pt) - 1);
    if (c != '3') {
        fprintf(stderr, "insert failed at end of entry again\n");
        return false;
    }

    // insert at start of entry again
    pt_insert(&pt, 1, '4');
    c = pt_get(&pt, 1);
    if (c != '4') {
        fprintf(stderr, "insert failed at start of entry again\n");
        return false;
    }

    // insert at start of entry again again
    pt_insert(&pt, 1, '5');
    c = pt_get(&pt, 1);
    if (c != '5') {
        fprintf(stderr, "insert failed at start of entry again again\n");
        return false;
    }

    // insert at middle of entry
    pt_insert(&pt, 6, '6');
    c = pt_get(&pt, 6);
    if (c != '6') {
        fprintf(stderr, "insert failed at middle of entry\n");
        return false;
    }

    pt_free(&pt);
    return true;
}

bool
test_pt_delete(void)
{
    const char orig[] = "Hello New World";

    struct pt pt = { 0 };
    pt_init(&pt, orig, strlen(orig));

    pt_delete(&pt, pt_size(&pt) - 1);
    char c = pt_get(&pt, pt_size(&pt) - 1);
    if (c != 'l') {
        fprintf(stderr, "delete failed at end of entry\n");
        return false;
    }

    pt_delete(&pt, 0);
    c = pt_get(&pt, 0);
    if (c != 'e') {
        fprintf(stderr, "delete failed at start of entry\n");
        return false;
    }

    pt_delete(&pt, 4);
    c = pt_get(&pt, 4);
    if (c != 'N') {
        fprintf(stderr, "delete failed at middle of entry\n");
        return false;
    }

    pt_free(&pt);
    return true;
}
