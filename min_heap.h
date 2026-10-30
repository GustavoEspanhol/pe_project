#ifndef MIN_HEAP_H
#define MIN_HEAP_H

struct HuffmanNode;
typedef struct MinHeap MinHeap;

MinHeap *min_heap_create(void);
int min_heap_push(MinHeap *heap, struct HuffmanNode *node);
struct HuffmanNode *min_heap_pop(MinHeap *heap);
void min_heap_destroy(MinHeap *heap);

#endif