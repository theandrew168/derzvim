#ifndef DERZVIM_LINE_H_INLCLUDED
#define DERZVIM_LINE_H_INLCLUDED

#include "array.h"

// TODO: just bake the array in?
struct line {
    struct array array;
    struct line* prev;
    struct line* next;
};

enum line_status {
    LINE_OK = 0,
    LINE_ERROR,
};

// TODO: init / free
// TODO: get head and tail from a FILE* ?
// TODO: serialize head into a FILE* ?
// TODO: extra helper stuff like split / join?

#endif
