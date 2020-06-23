#ifndef DERZVIM_PT_H_INCLUDED
#define DERZVIM_PT_H_INCLUDED

// Piece Table (nice data structure for text sequences)
//
// References:
// https://www.averylaird.com/programming/the%20text%20editor/2017/09/30/the-piece-table/
// https://en.wikipedia.org/wiki/Piece_table
// https://www.cs.unm.edu/~crowley/papers/sds.pdf
// https://github.com/sparkeditor/piece-table/blob/master/index.js

#include "ab.h"

enum pt_status {
    PT_OK = 0,
    PT_ERROR_OUT_OF_BOUNDS,
};

enum pt_source {
    PT_SOURCE_ORIG,
    PT_SOURCE_ADD,
};

struct pt_entry {
    int source;
    long start;
    long size;
};

struct pt {
    const char* orig_buf;
    struct ab add_buf;
    long entry_count;
    struct pt_entry entries[256];
};

int pt_init(struct pt* pt, const char* orig, long size);
void pt_free(struct pt* pt);

long pt_size(const struct pt* pt);
char pt_get(const struct pt* pt, long index);
int pt_insert(struct pt* pt, long index, char c);
int pt_delete(struct pt* pt, long index);

//int pt_get_seq(const struct pt* pt, long start, long end, char* buf, long size);
void pt_print(const struct pt* pt);

#endif
