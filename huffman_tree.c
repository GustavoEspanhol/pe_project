#include <limits.h>
#include <stdlib.h>
#include "huffman_tree.h"
#include "min_heap.h"

static HuffmanNode *create_node(unsigned long frequency,
                                unsigned int minimum_symbol,
                                unsigned long order,
                                int is_leaf,
                                unsigned char symbol,
                                HuffmanNode *left,
                                HuffmanNode *right)
{
    HuffmanNode *node;

    node = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    if (node == NULL)
        return NULL;
    node->frequency = frequency;
    node->minimum_symbol = minimum_symbol;
    node->order = order;
    node->is_leaf = is_leaf;
    node->symbol = symbol;
    node->left = left;
    node->right = right;
    return node;
}

HuffmanNode *huffman_tree_build(const unsigned long frequencies[256])
{
    MinHeap *heap;
    HuffmanNode *node;
    HuffmanNode *left;
    HuffmanNode *right;
    HuffmanNode *parent;
    unsigned int symbol;
    unsigned long next_order;

    heap = min_heap_create();
    if (heap == NULL)
        return NULL;
    next_order = 0;
    for (symbol = 0; symbol < 256; ++symbol) {
        if (frequencies[symbol] != 0) {
            node = create_node(frequencies[symbol], symbol, next_order++, 1,
                               (unsigned char)symbol, NULL, NULL);
            if (node == NULL || !min_heap_push(heap, node)) {
                free(node);
                while ((node = min_heap_pop(heap)) != NULL)
                    huffman_tree_free(node);
                min_heap_destroy(heap);
                return NULL;
            }
        }
    }

    node = min_heap_pop(heap);
    if (node == NULL) {
        min_heap_destroy(heap);
        return NULL;
    }
    if (!min_heap_push(heap, node)) {
        huffman_tree_free(node);
        while ((node = min_heap_pop(heap)) != NULL)
            huffman_tree_free(node);
        min_heap_destroy(heap);
        return NULL;
    }

    while (1) {
        left = min_heap_pop(heap);
        if (left == NULL)
            break;
        right = min_heap_pop(heap);
        if (right == NULL) {
            min_heap_destroy(heap);
            return left;
        }
        if (left->frequency > ULONG_MAX - right->frequency) {
            huffman_tree_free(left);
            huffman_tree_free(right);
            while ((node = min_heap_pop(heap)) != NULL)
                huffman_tree_free(node);
            min_heap_destroy(heap);
            return NULL;
        }
        parent = create_node(left->frequency + right->frequency,
                             left->minimum_symbol < right->minimum_symbol ?
                                 left->minimum_symbol : right->minimum_symbol,
                             next_order++, 0, 0, left, right);
        if (parent == NULL || !min_heap_push(heap, parent)) {
            if (parent == NULL) {
                huffman_tree_free(left);
                huffman_tree_free(right);
            } else {
                huffman_tree_free(parent);
            }
            while ((node = min_heap_pop(heap)) != NULL)
                huffman_tree_free(node);
            min_heap_destroy(heap);
            return NULL;
        }
    }
    min_heap_destroy(heap);
    return NULL;
}

void huffman_tree_free(HuffmanNode *root)
{
    if (root == NULL)
        return;
    huffman_tree_free(root->left);
    huffman_tree_free(root->right);
    free(root);
}