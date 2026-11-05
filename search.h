#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>

#define SEARCH_MAX_LISTED 20

typedef struct {
    unsigned long count;
    unsigned long decoded;
    unsigned int listed;
    unsigned long offsets[SEARCH_MAX_LISTED];
    unsigned long lines[SEARCH_MAX_LISTED];
    unsigned long columns[SEARCH_MAX_LISTED];
} SearchResult;

int search_zipz(const char *path, const unsigned char *pattern, size_t length,
                SearchResult *result);

#endif
