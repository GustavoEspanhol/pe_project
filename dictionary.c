#include <string.h>
#include "dictionary.h"

static void fill_dictionary(Dictionary *dictionary, const HuffmanNode *node,
                            unsigned char path[HUFFMAN_MAX_CODE_BITS + 1],
                            unsigned int depth)
{
    if (node->is_leaf) {
        unsigned int index;
        unsigned int code_length;

        code_length = depth == 0 ? 1 : depth;
        dictionary->entries[node->symbol].frequency = node->frequency;
        dictionary->entries[node->symbol].length = (unsigned short)code_length;
        if (depth == 0)
            path[0] = 0;
        for (index = 0; index < code_length; ++index)
            dictionary->entries[node->symbol].bits[index] = path[index];
        return;
    }
    path[depth] = 0;
    fill_dictionary(dictionary, node->left, path, depth + 1);
    path[depth] = 1;
    fill_dictionary(dictionary, node->right, path, depth + 1);
}

void dictionary_init(Dictionary *dictionary)
{
    memset(dictionary, 0, sizeof(Dictionary));
}

int dictionary_from_tree(Dictionary *dictionary, const HuffmanNode *root)
{
    unsigned char path[HUFFMAN_MAX_CODE_BITS + 1];

    if (dictionary == NULL || root == NULL)
        return 0;
    dictionary_init(dictionary);
    fill_dictionary(dictionary, root, path, 0);
    return 1;
}