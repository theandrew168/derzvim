#ifndef DERZVIM_ARRAY_H_INCLUDED
#define DERZVIM_ARRAY_H_INCLUDED

enum array_status {
    ARRAY_OK = 0,
    ARRAY_ERROR,
};

struct array {
    long capacity;
    long size;
    char* buf;
};

int array_init(struct array* array);
int array_free(struct array* array);

char array_get(const struct array* array, long index);
long array_size(const struct array* array);
char* array_buffer(const struct array* array);

int array_insert(struct array* array, long index, char c);
int array_delete(struct array* array, long index);
int array_append(struct array* array, char c);

#endif
