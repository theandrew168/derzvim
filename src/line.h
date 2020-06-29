#ifndef DERZVIM_LINE_H_INLCLUDED
#define DERZVIM_LINE_H_INLCLUDED

struct line {
    struct line* prev;
    struct line* next;
    long capacity;
    long size;
    char* buf;
};

enum line_status {
    LINE_OK = 0,
    LINE_ERROR,
};

int line_init(struct line* line);
int line_free(struct line* line);

char line_get(const struct line* line, long index);
int line_append(struct line* line, char c);
int line_insert(struct line* line, long pos, char c);
int line_delete(struct line* line, long pos);

// TODO: get head and tail from a FILE* ?
// TODO: serialize head into a FILE* ?
// TODO: extra helper stuff like split / join?

#endif
