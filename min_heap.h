#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <stddef.h>
#include "huffman_tree.h"

typedef struct {
    HuffmanNode **data;
    size_t size;
    size_t capacity;
} MinHeap;

int min_heap_init(MinHeap *heap, size_t capacity);
void min_heap_destroy(MinHeap *heap);
int min_heap_push(MinHeap *heap, HuffmanNode *node);
HuffmanNode *min_heap_pop(MinHeap *heap);

#endif
