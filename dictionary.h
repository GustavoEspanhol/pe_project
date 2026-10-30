#ifndef DICTIONARY_H
#define DICTIONARY_H

#define HUFFMAN_MAX_CODE_BITS 255

#include "huffman_tree.h"

typedef struct {
    unsigned long frequency;
    unsigned short length;
    unsigned char bits[32];
} DictionaryEntry;

typedef struct {
    DictionaryEntry entries[256];
} Dictionary;

void dictionary_init(Dictionary *dictionary);
int dictionary_from_tree(Dictionary *dictionary, const HuffmanNode *root);

#endif