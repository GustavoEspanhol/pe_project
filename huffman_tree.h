#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

typedef struct HuffmanNode {
    unsigned long frequency;
    unsigned long order;
    unsigned int minimum_symbol;
    unsigned char symbol;
    int is_leaf;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

HuffmanNode *huffman_tree_build(const unsigned long frequencies[256]);
void huffman_tree_free(HuffmanNode *root);

#endif