#include <assert.h>
#include <stdlib.h>

#include <stdio.h>

#include "ab.h"
#include "pt.h"

static int
pt_entry_find(const struct pt* pt, long index, long* entry_index, long* entry_offset)
{
    assert(pt != NULL);
    assert(index >= 0);
    assert(entry_index != NULL);
    assert(entry_offset != NULL);

    for (long i = 0; i < pt->entry_count; i++) {
        struct pt_entry entry = pt->entries[i];
        if (index < entry.size) {
            *entry_index = i;
            *entry_offset = entry.start + index;
            return PT_OK;
        }

        index -= entry.size;
    }

    // if the index is immediately after existing entries, point to the end
    if (index == 0) {
        struct pt_entry entry = pt->entries[pt->entry_count - 1];
        *entry_index = pt->entry_count - 1;
        *entry_offset = entry.start + entry.size;
        return PT_OK;
    }

    return PT_ERROR_OUT_OF_BOUNDS;
}

static int
pt_entry_insert(struct pt* pt, long index, struct pt_entry entry)
{
    if (index < 0 || index > pt->entry_count || pt->entry_count >= 256) {
        return PT_ERROR_OUT_OF_BOUNDS;
    }

    // move entries up if necessary
    for (long i = pt->entry_count; i > index; i--) {
        pt->entries[i] = pt->entries[i - 1];
    }

    pt->entries[index] = entry;
    pt->entry_count++;
    return PT_OK;
}

static int
pt_entry_delete(struct pt* pt, long index)
{
    if (index < 0 || index >= pt->entry_count || pt->entry_count >= 255) {
        return PT_ERROR_OUT_OF_BOUNDS;
    }

    pt->entry_count -= 1;
    for (long i = index; i < pt->entry_count; i++) {
        pt->entries[i] = pt->entries[i + 1];
    }

    return PT_OK;
}

int
pt_init(struct pt* pt, const char* orig, long size)
{
    assert(pt != NULL);
    assert(orig != NULL);

    pt->orig_buf = orig;

    // setup initial table entry
    struct pt_entry entry = {
        .source = PT_SOURCE_ORIG,
        .start = 0,
        .size = size,
    };

    pt_entry_insert(pt, 0, entry);
    return PT_OK;
}

void
pt_free(struct pt* pt)
{
    assert(pt != NULL);
    ab_free(&pt->add_buf);
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

char
pt_get(const struct pt* pt, long index)
{
    assert(pt != NULL);

    long entry_index = 0;
    long entry_offset = 0;
    if (pt_entry_find(pt, index, &entry_index, &entry_offset) != PT_OK) {
        return -1;
    }

    struct pt_entry entry = pt->entries[entry_index];
    if (entry.source == PT_SOURCE_ORIG) {
        return pt->orig_buf[entry_offset];
    } else if (entry.source == PT_SOURCE_ADD) {
        return pt->add_buf.buf[entry_offset];
    }

    return -1;
}

int
pt_insert(struct pt* pt, long index, char c)
{
    assert(pt != NULL);
    if (index < 0 || index > pt_size(pt)) {
        return PT_ERROR_OUT_OF_BOUNDS;
    }

    // append new character to the add buffer
    long add_index = pt->add_buf.size;
    ab_append(&pt->add_buf, &c, 1);

    // find where the new char is going to go
    long entry_index = 0;
    long entry_offset = 0;
    if (pt_entry_find(pt, index, &entry_index, &entry_offset) != PT_OK) {
        return PT_ERROR_OUT_OF_BOUNDS;
    }

    // most common case: appending to the end of the add buffer
    struct pt_entry* target = &pt->entries[entry_index];
    if (target->source == PT_SOURCE_ADD &&
        entry_offset == target->start + target->size &&
        add_index == target->start + target->size) {
        target->size += 1;
        return PT_OK;
    }

    // first half of the split
    struct pt_entry split_before = {
        .source = target->source,
        .start = target->start,
        .size = entry_offset - target->start,
    };

    // new entry between the two splits
    struct pt_entry new_entry = {
        .source = PT_SOURCE_ADD,
        .start = add_index,
        .size = 1,
    };

    // second half of the split
    struct pt_entry split_after = {
        .source = target->source,
        .start = entry_offset,
        .size = target->size - (entry_offset - split_before.start),
    };

    // delete the original target entry
    pt_entry_delete(pt, entry_index);

    // gather all of the new entries in reverse order for simpler insertion
    struct pt_entry entries[] = {
        split_after,
        new_entry,
        split_before,
    };

    // only insert entries with size greather than zero
    long entry_count = sizeof(entries) / sizeof(entries[0]);
    for (long i = 0; i < entry_count; i++) {
        if (entries[i].size <= 0) continue;
        pt_entry_insert(pt, entry_index, entries[i]);
    }

    return PT_OK;
}

int
pt_delete(struct pt* pt, long index)
{
    assert(pt != NULL);
    if (index < 0 || index >= pt_size(pt)) {
        return PT_ERROR_OUT_OF_BOUNDS;
    }

    // default entry is the last one
    struct pt_entry* target = &pt->entries[pt->entry_count - 1];

    // determine which piece table entry this delete affects
    long offset = 0;
    long entry_index = 0;
    for (long i = 0; i < pt->entry_count; i++) {
        if (index < offset + pt->entries[i].size) {
            target = &pt->entries[i];
            break;
        };

        offset += pt->entries[i].size;
        entry_index++;
    }

    printf("index: %ld, offset: %ld\n", index, offset);

    if (index == target->size - 1) {
        target->size--;
    } else {
        // adjust target size to account for the split
        long prev_size = target->size;
        target->size = (index - offset - 1);

        // split the target entry into two separate entries
        struct pt_entry split = {
            .source = target->source,
            .start = target->start + target->size,
            .size = prev_size - target->size,
        };
        pt_entry_insert(pt, entry_index + 1, split);
    }

    return PT_OK;
}

void
pt_print(const struct pt* pt)
{
    printf("\n");
    printf("orig: %s\n", pt->orig_buf);
    printf("add:  %.*s\n", (int)pt->add_buf.size, pt->add_buf.buf);
    printf("real: ");
    for (long i = 0; i < pt_size(pt); i++) {
        printf("%c", pt_get(pt, i));
    }
    printf("\n");

    long offset = 0;
    for (long i = 0; i < pt->entry_count; i++) {
        struct pt_entry entry = pt->entries[i];
        printf("source: %s | start: %4ld | size: %4ld\n",
            entry.source == PT_SOURCE_ORIG ? "ORIG" : " ADD",
            entry.start,
            entry.size);
        offset += entry.size;
    }
    printf("total size: %ld\n", pt_size(pt));
}
