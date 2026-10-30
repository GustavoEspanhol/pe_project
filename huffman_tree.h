#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#define HUFFMAN_SYMBOLS 256

typedef struct HuffmanNode {
    unsigned char symbol;
    unsigned long freq;
    int is_leaf;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

HuffmanNode *huffman_node_create_leaf(unsigned char symbol, unsigned long freq);
HuffmanNode *huffman_node_create_internal(HuffmanNode *left, HuffmanNode *right);
HuffmanNode *huffman_tree_build_from_frequencies(const unsigned long frequencies[HUFFMAN_SYMBOLS]);
void huffman_tree_free(HuffmanNode *root);
int huffman_tree_is_leaf(const HuffmanNode *node);

#endif
