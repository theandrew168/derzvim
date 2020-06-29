#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "line.h"

enum {
    LINE_DEFAULT_CAPACITY = 256,
    LINE_CAPACITY_GROWTH = 2,
};

int
line_init(struct line* line)
{
    assert(line != NULL);

    line->capacity = LINE_DEFAULT_CAPACITY;
    line->size = 0;
    line->buf = malloc(LINE_DEFAULT_CAPACITY);
    if (line->buf == NULL) {
        fprintf(stderr, "line: failed to allocate initial buffer\n");
        return LINE_ERROR;
    }

    return LINE_OK;
}

int
line_free(struct line* line)
{
    assert(line != NULL);
    free(line->buf);
    return LINE_OK;
}

char
line_get(const struct line* line, long index)
{
    assert(line != NULL);
    assert(index >= 0);
    assert(index < line->size);

    return line->buf[index];
}

int
line_append(struct line* line, char c)
{
    assert(line != NULL);

    line_insert(line, line->size, c);

    return LINE_OK;
}

int
line_insert(struct line* line, long pos, char c)
{
    assert(line != NULL);
    assert(pos >= 0);
    assert(pos <= line->size); // pos can equal size here to imply inserting at the end

    // grow the buffer if current capacity is reached
    if (line->size >= line->capacity) {
        line->capacity *= LINE_CAPACITY_GROWTH;
        line->buf = realloc(line->buf, line->capacity);
        if (line->buf == NULL) return LINE_ERROR;
    }

    // shift buffer contents up
    memmove(&line->buf[pos + 1], &line->buf[pos], line->size - pos);

    // insert the new character
    line->buf[pos] = c;
    line->size++;

    return LINE_OK;
}

int
line_delete(struct line* line, long pos)
{
    assert(line != NULL);
    assert(pos >= 0);
    assert(pos < line->size);

    // shift buffer contents down
    memmove(&line->buf[pos], &line->buf[pos + 1], line->size - pos);
    line->size--;

    return LINE_OK;
}

int
line_break(struct line* line, long pos)
{
    assert(line != NULL);
    assert(pos >= 0);
    assert(pos < line->size);

    struct line* new = calloc(1, sizeof(struct line));
    line_init(new);

    // copy rest of existing line into the new one
    for (long i = pos; i < line->size; i++) {
        line_append(new, line_get(line, i));
    }

    // delete rest of existing line
    for (long i = line->size - 1; i >= pos; i--) {
        line_delete(line, i);
    }

    // link the new line in
    new->prev = line;
    new->next = line->next;
    if (line->next != NULL) line->next->prev = new;
    line->next = new;

    return LINE_OK;
}

int
line_merge(struct line* dest, struct line* src)
{
    assert(dest != NULL);
    assert(src != NULL);

    // append src line to dest line
    for (long i = 0; i < src->size; i++) {
        line_append(dest, line_get(src, i));
    }

    // unlink and free the src line
    if (src->next != NULL) src->next->prev = src->prev;
    if (src->prev != NULL) src->prev->next = src->next;
    line_free(src);
    free(src);

    return LINE_OK;
}

int
lines_init(struct line** head, struct line** tail, long* count, const char* path)
{
    assert(head != NULL);
    assert(tail != NULL);
    assert(count != NULL);
    assert(path != NULL);

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "failed to open file: %s\n", path);
        return LINE_ERROR;
    }

    // read through each character in the file
    int c = 0;
    bool newline = false;
    while ((c = fgetc(fp)) != EOF) {
        if (newline) {
            struct line* line = calloc(1, sizeof(struct line));
            line_init(line);
            line->prev = *tail;
            (*tail)->next = line;
            *tail = line;

            newline = false;
            *count++;
        }

        // only add a line if text comes after the NL (handles trailing NL)
        if (c == '\n') {
            newline = true;
        } else {
            line_append(*tail, c);
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "IO error while reading file: %s\n", path);
        return LINE_ERROR;
    }

    fclose(fp);
    return LINE_OK;
}

int
lines_free(struct line** head, struct line** tail)
{
    struct line* line = *head;
    while (line != NULL) {
        struct line* current = line;
        line = line->next;
        line_free(current);
        free(current);
    }

    *head = NULL;
    *tail = NULL;

    return LINE_OK;
}

int
lines_write(const struct line* head, const char* path)
{
    FILE* fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open output file: %s\n", path);
        return LINE_ERROR;
    }

    for (const struct line* line = head; line != NULL; line = line->next) {
        fwrite(line->buf, line->size, 1, fp);
        fputc('\n', fp);
    }

    fclose(fp);
    return LINE_OK;
}
