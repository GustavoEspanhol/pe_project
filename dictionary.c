#include "dictionary.h"

#include <string.h>

static void dictionary_store_code(HuffmanCode *code, const unsigned char *path, unsigned short length) {
    unsigned short i;
    memset(code->bits, 0, sizeof(code->bits));
    code->length = length;
    code->defined = 1;

    for (i = 0; i < length; ++i) {
        if (path[i] != 0U) {
            unsigned short byte_index;
            unsigned short bit_index;
            byte_index = (unsigned short)(i / 8U);
            bit_index = (unsigned short)(7U - (i % 8U));
            code->bits[byte_index] |= (unsigned char)(1U << bit_index);
        }
    }
}

static int dictionary_build_recursive(Dictionary *dictionary,
                                      const HuffmanNode *node,
                                      unsigned char *path,
                                      unsigned short depth) {
    if (node == NULL) {
        return 0;
    }

    if (huffman_tree_is_leaf(node)) {
        if (depth == 0U) {
            /* Caso especial: arquivo com apenas um símbolo distinto. */
            path[0] = 0U;
            depth = 1U;
        }
        dictionary_store_code(&dictionary->codes[node->symbol], path, depth);
        return 1;
    }

    if (depth >= MAX_CODE_BITS) {
        return 0;
    }

    path[depth] = 0U;
    if (!dictionary_build_recursive(dictionary, node->left, path, (unsigned short)(depth + 1U))) {
        return 0;
    }

    path[depth] = 1U;
    if (!dictionary_build_recursive(dictionary, node->right, path, (unsigned short)(depth + 1U))) {
        return 0;
    }

    return 1;
}

void dictionary_init(Dictionary *dictionary) {
    int i;
    if (dictionary == NULL) {
        return;
    }
    for (i = 0; i < HUFFMAN_SYMBOLS; ++i) {
        dictionary->codes[i].length = 0U;
        dictionary->codes[i].defined = 0;
        memset(dictionary->codes[i].bits, 0, sizeof(dictionary->codes[i].bits));
    }
}

int dictionary_build_from_tree(Dictionary *dictionary, const HuffmanNode *root) {
    unsigned char path[MAX_CODE_BITS];
    if (dictionary == NULL || root == NULL) {
        return 0;
    }
    dictionary_init(dictionary);
    return dictionary_build_recursive(dictionary, root, path, 0U);
}

const HuffmanCode *dictionary_get_code(const Dictionary *dictionary, unsigned char symbol) {
    if (dictionary == NULL) {
        return NULL;
    }
    if (!dictionary->codes[symbol].defined) {
        return NULL;
    }
    return &dictionary->codes[symbol];
}

int huffman_code_get_bit(const HuffmanCode *code, unsigned short index) {
    unsigned short byte_index;
    unsigned short bit_index;
    if (code == NULL || index >= code->length) {
        return 0;
    }
    byte_index = (unsigned short)(index / 8U);
    bit_index = (unsigned short)(7U - (index % 8U));
    return ((code->bits[byte_index] & (unsigned char)(1U << bit_index)) != 0U) ? 1 : 0;
}
