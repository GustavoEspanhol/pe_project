#include <stdlib.h>
#include "huffman_tree.h"
#include "min_heap.h"

struct MinHeap {
    struct HuffmanNode *items[256];
    unsigned int size;
};

static int comes_before(const struct HuffmanNode *left,
                       const struct HuffmanNode *right)
{
    if (left->frequency != right->frequency)
        return left->frequency < right->frequency;
    if (left->minimum_symbol != right->minimum_symbol)
        return left->minimum_symbol < right->minimum_symbol;
    return left->order < right->order;
}

MinHeap *min_heap_create(void)
{
    MinHeap *heap;

    heap = (MinHeap *)malloc(sizeof(MinHeap));
    if (heap != NULL)
        heap->size = 0;
    return heap;
}

int min_heap_push(MinHeap *heap, struct HuffmanNode *node)
{
    unsigned int position;

    if (heap == NULL || node == NULL || heap->size >= 256)
        return 0;
    position = heap->size++;
    while (position > 0) {
        unsigned int parent;

        parent = (position - 1) / 2;
        if (!comes_before(node, heap->items[parent]))
            break;
        heap->items[position] = heap->items[parent];
        position = parent;
    }
    heap->items[position] = node;
    return 1;
}

struct HuffmanNode *min_heap_pop(MinHeap *heap)
{
    struct HuffmanNode *result;
    struct HuffmanNode *last;
    unsigned int position;

    if (heap == NULL || heap->size == 0)
        return NULL;
    result = heap->items[0];
    last = heap->items[--heap->size];
    if (heap->size == 0)
        return result;

    position = 0;
    while (position * 2 + 1 < heap->size) {
        unsigned int child;

        child = position * 2 + 1;
        if (child + 1 < heap->size &&
            comes_before(heap->items[child + 1], heap->items[child]))
            ++child;
        if (!comes_before(heap->items[child], last))
            break;
        heap->items[position] = heap->items[child];
        position = child;
    }
    heap->items[position] = last;
    return result;
}

void min_heap_destroy(MinHeap *heap)
{
    free(heap);
}