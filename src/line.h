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

int line_break(struct line* line, long pos);
int line_merge(struct line* dest, struct line* src);

int lines_init(struct line** head, struct line** tail, long* count, const char* path);
int lines_free(struct line** head, struct line** tail);

int lines_write(const struct line* head, const char* path);

#endif
