#include <limits.h>
#include <string.h>
#include "decoder.h"
#include "format.h"

static int read_ulong(FILE *file, unsigned long *value)
{
    unsigned char bytes[8];
    unsigned int index;
    unsigned long result;

    if (fread(bytes, 1, 8, file) != 8)
        return 0;
    result = 0;
    for (index = 0; index < 8; ++index) {
        if (result > (ULONG_MAX - bytes[index]) / 256UL)
            return 0;
        result = result * 256UL + bytes[index];
    }
    *value = result;
    return 1;
}

static int read_header(Decoder *decoder)
{
    unsigned char magic[ZIPZ_MAGIC_SIZE];
    unsigned char version;
    unsigned char valid;
    unsigned int index;
    unsigned long sum;
    FILE *file;

    file = decoder->input;
    if (fread(magic, 1, ZIPZ_MAGIC_SIZE, file) != ZIPZ_MAGIC_SIZE ||
        memcmp(magic, ZIPZ_MAGIC, ZIPZ_MAGIC_SIZE) != 0 ||
        fread(&version, 1, 1, file) != 1 || version != ZIPZ_VERSION ||
        !read_ulong(file, &decoder->original_size) ||
        fread(&valid, 1, 1, file) != 1)
        return 0;
    decoder->valid_bits = valid;
    sum = 0;
    for (index = 0; index < 256; ++index) {
        if (!read_ulong(file, &decoder->frequencies[index]) ||
            sum > ULONG_MAX - decoder->frequencies[index])
            return 0;
        sum += decoder->frequencies[index];
    }
    if (sum != decoder->original_size)
        return 0;
    if ((decoder->original_size == 0 && decoder->valid_bits != 0) ||
        (decoder->original_size != 0 &&
         (decoder->valid_bits == 0 || decoder->valid_bits > 8)))
        return 0;
    return 1;
}

int decoder_open(Decoder *decoder, const char *path)
{
    decoder->root = NULL;
    decoder->decoded = 0;
    decoder->failed = 0;
    memset(decoder->decoded_frequencies, 0,
           sizeof(decoder->decoded_frequencies));
    decoder->input = fopen(path, "rb");
    if (decoder->input == NULL)
        return 0;
    if (!read_header(decoder)) {
        decoder_close(decoder);
        return 0;
    }
    if (decoder->original_size != 0) {
        decoder->root = huffman_tree_build(decoder->frequencies);
        if (decoder->root == NULL) {
            decoder_close(decoder);
            return 0;
        }
    }
    bit_reader_init(&decoder->reader, decoder->input);
    return 1;
}

/* retorna 1 com um byte, 0 no fim dos dados e -1 se o arquivo estiver errado */
int decoder_next(Decoder *decoder, unsigned char *byte)
{
    const HuffmanNode *node;
    unsigned int bit;

    if (decoder->failed)
        return -1;
    if (decoder->decoded == decoder->original_size)
        return 0;
    node = decoder->root;
    if (node->is_leaf) {
        if (bit_reader_read_bit(&decoder->reader, &bit) != 1 || bit != 0) {
            decoder->failed = 1;
            return -1;
        }
    } else {
        while (!node->is_leaf) {
            if (bit_reader_read_bit(&decoder->reader, &bit) != 1) {
                decoder->failed = 1;
                return -1;
            }
            node = bit == 0 ? node->left : node->right;
            if (node == NULL) {
                decoder->failed = 1;
                return -1;
            }
        }
    }
    *byte = node->symbol;
    ++decoder->decoded_frequencies[node->symbol];
    ++decoder->decoded;
    return 1;
}

int decoder_finish(Decoder *decoder)
{
    if (decoder->failed || decoder->decoded != decoder->original_size)
        return 0;
    if (memcmp(decoder->frequencies, decoder->decoded_frequencies,
               sizeof(decoder->frequencies)) != 0)
        return 0;
    return bit_reader_finish(&decoder->reader, decoder->valid_bits);
}

int decoder_close(Decoder *decoder)
{
    int success;

    success = 1;
    huffman_tree_free(decoder->root);
    decoder->root = NULL;
    if (decoder->input != NULL && fclose(decoder->input) != 0)
        success = 0;
    decoder->input = NULL;
    return success;
}
