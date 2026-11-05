#ifndef DECODER_H
#define DECODER_H

#include <stdio.h>
#include "bitio.h"
#include "huffman_tree.h"

typedef struct {
    FILE *input;
    HuffmanNode *root;
    BitReader reader;
    unsigned long frequencies[256];
    unsigned long decoded_frequencies[256];
    unsigned long original_size;
    unsigned long decoded;
    unsigned int valid_bits;
    int failed;
} Decoder;

int decoder_open(Decoder *decoder, const char *path);
int decoder_next(Decoder *decoder, unsigned char *byte);
int decoder_finish(Decoder *decoder);
int decoder_close(Decoder *decoder);

#endif
