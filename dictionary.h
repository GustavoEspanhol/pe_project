#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "huffman_tree.h"

#define MAX_CODE_BITS 256
#define MAX_CODE_BYTES 32

typedef struct {
    unsigned char bits[MAX_CODE_BYTES];
    unsigned short length;
    int defined;
} HuffmanCode;

typedef struct {
    HuffmanCode codes[HUFFMAN_SYMBOLS];
} Dictionary;

void dictionary_init(Dictionary *dictionary);
int dictionary_build_from_tree(Dictionary *dictionary, const HuffmanNode *root);
const HuffmanCode *dictionary_get_code(const Dictionary *dictionary, unsigned char symbol);
int huffman_code_get_bit(const HuffmanCode *code, unsigned short index);

#endif
