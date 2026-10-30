#include "min_heap.h"

#include <stdlib.h>

static void swap_nodes(HuffmanNode **a, HuffmanNode **b) {
    HuffmanNode *tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

static int min_heap_resize(MinHeap *heap, size_t new_capacity) {
    HuffmanNode **new_data;
    new_data = (HuffmanNode **)realloc(heap->data, new_capacity * sizeof(HuffmanNode *));
    if (new_data == NULL) {
        return 0;
    }
    heap->data = new_data;
    heap->capacity = new_capacity;
    return 1;
}

int min_heap_init(MinHeap *heap, size_t capacity) {
    if (heap == NULL || capacity == 0) {
        return 0;
    }
    heap->data = (HuffmanNode **)malloc(capacity * sizeof(HuffmanNode *));
    if (heap->data == NULL) {
        return 0;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return 1;
}

void min_heap_destroy(MinHeap *heap) {
    if (heap == NULL) {
        return;
    }
    free(heap->data);
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

int min_heap_push(MinHeap *heap, HuffmanNode *node) {
    size_t index;
    if (heap == NULL || node == NULL) {
        return 0;
    }

    if (heap->size == heap->capacity) {
        size_t new_capacity;
        new_capacity = heap->capacity * 2;
        if (new_capacity == 0 || !min_heap_resize(heap, new_capacity)) {
            return 0;
        }
    }

    index = heap->size;
    heap->data[index] = node;
    heap->size++;

    while (index > 0) {
        size_t parent;
        parent = (index - 1U) / 2U;
        if (heap->data[parent]->freq <= heap->data[index]->freq) {
            break;
        }
        swap_nodes(&heap->data[parent], &heap->data[index]);
        index = parent;
    }

    return 1;
}

HuffmanNode *min_heap_pop(MinHeap *heap) {
    HuffmanNode *result;
    size_t index;
    if (heap == NULL || heap->size == 0) {
        return NULL;
    }

    result = heap->data[0];
    heap->size--;
    if (heap->size == 0) {
        return result;
    }

    heap->data[0] = heap->data[heap->size];
    index = 0;

    while (1) {
        size_t left;
        size_t right;
        size_t smallest;

        left = index * 2U + 1U;
        right = left + 1U;
        smallest = index;

        if (left < heap->size && heap->data[left]->freq < heap->data[smallest]->freq) {
            smallest = left;
        }
        if (right < heap->size && heap->data[right]->freq < heap->data[smallest]->freq) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }

        swap_nodes(&heap->data[index], &heap->data[smallest]);
        index = smallest;
    }

    return result;
}
