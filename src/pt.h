#ifndef DERZVIM_PT_H_INCLUDED
#define DERZVIM_PT_H_INCLUDED

// Piece Table (nice data structure for text sequences)
// https://www.averylaird.com/programming/the%20text%20editor/2017/09/30/the-piece-table/
// https://en.wikipedia.org/wiki/Piece_table
// https://www.cs.unm.edu/~crowley/papers/sds.pdf

#include "ab.h"

enum pt_status {
    PT_OK = 0,
    PT_EOF,
    PT_ERROR,
};

enum pt_file {
    PT_FILE_ORIG,
    PT_FILE_ADD,
};

struct pt_entry {
    int file;
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
long pt_size(const struct pt* pt);
int pt_get(const struct pt* pt, long index, char* c);
//int pt_get_seq(const struct pt* pt, long start, long end, char* buf, long size);
int pt_insert(struct pt* pt, long index, char c);
//int pt_delete(struct pt* pt, long index);
void pt_free(struct pt* pt);

#endif
