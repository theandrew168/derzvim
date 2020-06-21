#include <assert.h>
#include <stdlib.h>

#include "ab.h"
#include "pt.h"

int
pt_init(struct pt* pt, const char* orig, long size)
{
    assert(pt != NULL);
    assert(orig != NULL);

    pt->orig_buf = orig;

    // setup initial table entry
    struct pt_entry entry = {
        .file = PT_FILE_ORIG,
        .start = 0,
        .size = size,
    };

    pt->entries[pt->entry_count] = entry;
    pt->entry_count++;

    return PT_OK;
}

long
pt_size(const struct pt* pt)
{
    assert(pt != NULL);

    long size = 0;
    for (long i = 0; i < pt->entry_count; i++) {
        size += pt->entries[i].size;
    }

    return size;
}

int
pt_get(const struct pt* pt, long index, char* c)
{
    assert(pt != NULL);

    long offset = 0;
    for (long i = 0; i < pt->entry_count; i++) {
        struct pt_entry entry = pt->entries[i];

        if (index < offset + entry.size) {
            if (entry.file == PT_FILE_ORIG) {
                *c = pt->orig_buf[entry.start + (index - offset)];
                return PT_OK;
            } else if (entry.file == PT_FILE_ADD) {
                *c = pt->add_buf.buf[entry.start + (index - offset)];
                return PT_OK;
            }
        }

        offset += entry.size;
    }

    return PT_EOF;
}

//int
//pt_get_seq(const struct pt* pt, long start, long end, char* buf, long size)
//{
//    assert(pt != NULL);
//
//    for (long i = 0; i < pt->entry_count; i++) {
//        size += pt->entries[i].size;
//    }
//
//    return PT_OK;
//}

int
pt_insert(struct pt* pt, long index, char c)
{
    assert(pt != NULL);
    return PT_OK;
}

//int
//pt_delete(struct pt* pt, long index)
//{
//    assert(pt != NULL);
//    return PT_OK;
//}

void
pt_free(struct pt* pt)
{
    assert(pt != NULL);
    ab_free(&pt->add_buf);
}
