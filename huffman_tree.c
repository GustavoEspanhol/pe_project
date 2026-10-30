#include "huffman_tree.h"

#include "min_heap.h"

#include <stdlib.h>

HuffmanNode *huffman_node_create_leaf(unsigned char symbol, unsigned long freq) {
    HuffmanNode *node;
    node = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    if (node == NULL) {
        return NULL;
    }
    node->symbol = symbol;
    node->freq = freq;
    node->is_leaf = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

HuffmanNode *huffman_node_create_internal(HuffmanNode *left, HuffmanNode *right) {
    HuffmanNode *node;
    node = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    if (node == NULL) {
        return NULL;
    }
    node->symbol = 0;
    node->freq = 0;
    node->is_leaf = 0;
    node->left = left;
    node->right = right;
    if (left != NULL) {
        node->freq += left->freq;
    }
    if (right != NULL) {
        node->freq += right->freq;
    }
    return node;
}

HuffmanNode *huffman_tree_build_from_frequencies(const unsigned long frequencies[HUFFMAN_SYMBOLS]) {
    MinHeap heap;
    HuffmanNode *root;
    int i;

    if (!min_heap_init(&heap, HUFFMAN_SYMBOLS)) {
        return NULL;
    }

    for (i = 0; i < HUFFMAN_SYMBOLS; ++i) {
        if (frequencies[i] > 0UL) {
            HuffmanNode *leaf;
            leaf = huffman_node_create_leaf((unsigned char)i, frequencies[i]);
            if (leaf == NULL || !min_heap_push(&heap, leaf)) {
                huffman_tree_free(leaf);
                while (heap.size > 0U) {
                    huffman_tree_free(min_heap_pop(&heap));
                }
                min_heap_destroy(&heap);
                return NULL;
            }
        }
    }

    if (heap.size == 0U) {
        min_heap_destroy(&heap);
        return NULL;
    }

    while (heap.size > 1U) {
        HuffmanNode *a;
        HuffmanNode *b;
        HuffmanNode *parent;

        a = min_heap_pop(&heap);
        b = min_heap_pop(&heap);
        parent = huffman_node_create_internal(a, b);
        if (parent == NULL || !min_heap_push(&heap, parent)) {
            huffman_tree_free(parent);
            huffman_tree_free(a);
            huffman_tree_free(b);
            while (heap.size > 0U) {
                huffman_tree_free(min_heap_pop(&heap));
            }
            min_heap_destroy(&heap);
            return NULL;
        }
    }

    root = min_heap_pop(&heap);
    min_heap_destroy(&heap);
    return root;
}

void huffman_tree_free(HuffmanNode *root) {
    if (root == NULL) {
        return;
    }
    huffman_tree_free(root->left);
    huffman_tree_free(root->right);
    free(root);
}

int huffman_tree_is_leaf(const HuffmanNode *node) {
    return node != NULL && node->is_leaf;
}
