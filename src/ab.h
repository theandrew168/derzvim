#ifndef DERZVIM_AB_H_INCLUDED
#define DERZVIM_AB_H_INCLUDED

struct ab {
    long size;
    char* buf;
};

enum ab_status {
    AB_OK = 0,
    AB_ERROR,
};

int ab_append(struct ab* ab, const char* s, long size);
void ab_free(struct ab* ab);

#endif
