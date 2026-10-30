#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "bitio.h"
#include "compressor.h"
#include "dictionary.h"
#include "huffman_tree.h"

#define IO_BUFFER_SIZE 8192
#define HEADER_SIZE 2062UL

static const unsigned char MAGIC[4] = { 'U', 'Z', 'P', 'Z' };

static int write_ulong(FILE *file, unsigned long value)
{
    unsigned char bytes[8];
    unsigned int index;

    for (index = 0; index < 8; ++index) {
        bytes[7 - index] = (unsigned char)(value & 255UL);
        value /= 256UL;
    }
    return fwrite(bytes, 1, 8, file) == 8;
}

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

static int write_header(FILE *file, unsigned long original_size,
                        unsigned int valid_bits,
                        const unsigned long frequencies[256])
{
    unsigned int index;
    unsigned char version;
    unsigned char valid;

    version = 1;
    valid = (unsigned char)valid_bits;
    if (fwrite(MAGIC, 1, 4, file) != 4 ||
        fwrite(&version, 1, 1, file) != 1 ||
        !write_ulong(file, original_size) ||
        fwrite(&valid, 1, 1, file) != 1)
        return 0;
    for (index = 0; index < 256; ++index) {
        if (!write_ulong(file, frequencies[index]))
            return 0;
    }
    return 1;
}

static int read_header(FILE *file, unsigned long *original_size,
                       unsigned int *valid_bits,
                       unsigned long frequencies[256])
{
    unsigned char magic[4];
    unsigned char version;
    unsigned char valid;
    unsigned int index;
    unsigned long sum;

    if (fread(magic, 1, 4, file) != 4 || memcmp(magic, MAGIC, 4) != 0 ||
        fread(&version, 1, 1, file) != 1 || version != 1 ||
        !read_ulong(file, original_size) ||
        fread(&valid, 1, 1, file) != 1)
        return 0;
    *valid_bits = valid;
    sum = 0;
    for (index = 0; index < 256; ++index) {
        if (!read_ulong(file, &frequencies[index]) ||
            sum > ULONG_MAX - frequencies[index])
            return 0;
        sum += frequencies[index];
    }
    if (sum != *original_size)
        return 0;
    if ((*original_size == 0 && *valid_bits != 0) ||
        (*original_size != 0 && (*valid_bits == 0 || *valid_bits > 8)))
        return 0;
    return 1;
}

static unsigned int last_byte_bits(const unsigned long frequencies[256],
                                   const Dictionary *dictionary)
{
    unsigned int index;
    unsigned int remainder;
    unsigned long frequency_remainder;

    remainder = 0;
    for (index = 0; index < 256; ++index) {
        frequency_remainder = frequencies[index] % 8UL;
        remainder = (remainder + (unsigned int)(frequency_remainder *
                    dictionary->entries[index].length % 8UL)) % 8;
    }
    return remainder == 0 ? 8 : remainder;
}

static int output_is_new(const char *path)
{
    FILE *existing;

    existing = fopen(path, "rb");
    if (existing == NULL)
        return 1;
    fclose(existing);
    return 0;
}

static int count_input(FILE *input, unsigned long frequencies[256],
                       unsigned long *total)
{
    unsigned char buffer[IO_BUFFER_SIZE];
    size_t count;
    size_t index;

    memset(frequencies, 0, 256 * sizeof(unsigned long));
    *total = 0;
    while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
        if (*total > ULONG_MAX - count)
            return 0;
        *total += (unsigned long)count;
        for (index = 0; index < count; ++index) {
            if (frequencies[buffer[index]] == ULONG_MAX)
                return 0;
            ++frequencies[buffer[index]];
        }
    }
    return !ferror(input);
}

int compressor_compress(const char *input_path, const char *output_path,
                        unsigned long *original_size,
                        unsigned long *compressed_size)
{
    FILE *input;
    FILE *output;
    unsigned long frequencies[256];
    unsigned long second_frequencies[256];
    unsigned long total;
    unsigned long second_total;
    unsigned long output_size;
    unsigned char buffer[IO_BUFFER_SIZE];
    size_t count;
    size_t index;
    unsigned int symbol;
    unsigned int valid_bits;
    HuffmanNode *root;
    Dictionary dictionary;
    BitWriter writer;
    int created;
    int success;

    input = fopen(input_path, "rb");
    if (input == NULL)
        return 0;
    /* Etapa 1: analisa frequencias em blocos na primeira leitura. */
    if (!count_input(input, frequencies, &total) || fseek(input, 0, SEEK_SET) != 0) {
        fclose(input);
        return 0;
    }
    root = NULL;
    if (total != 0) {
        /* Etapas 2 e 3: constroi a arvore pelo heap e gera os codigos. */
        root = huffman_tree_build(frequencies);
        if (root == NULL || !dictionary_from_tree(&dictionary, root)) {
            huffman_tree_free(root);
            fclose(input);
            return 0;
        }
    } else {
        dictionary_init(&dictionary);
    }
    valid_bits = total == 0 ? 0 : last_byte_bits(frequencies, &dictionary);

    /* Etapa 4: grava o cabecalho antes da segunda leitura e da codificacao. */
    if (!output_is_new(output_path)) {
        huffman_tree_free(root);
        fclose(input);
        return 0;
    }
    output = fopen(output_path, "wb");
    if (output == NULL) {
        huffman_tree_free(root);
        fclose(input);
        return 0;
    }
    created = 1;
    success = write_header(output, total, valid_bits, frequencies);
    memset(second_frequencies, 0, sizeof(second_frequencies));
    second_total = 0;
    bit_writer_init(&writer, output);

    while (success && (count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
        if (second_total > ULONG_MAX - count) {
            success = 0;
            break;
        }
        second_total += (unsigned long)count;
        for (index = 0; index < count; ++index) {
            symbol = buffer[index];
            if (second_frequencies[symbol] == ULONG_MAX) {
                success = 0;
                break;
            }
            ++second_frequencies[symbol];
            if (!bit_writer_write_code(&writer,
                    dictionary.entries[symbol].bits,
                    dictionary.entries[symbol].length)) {
                success = 0;
                break;
            }
        }
    }
    if (ferror(input) || second_total != total ||
        memcmp(frequencies, second_frequencies, sizeof(frequencies)) != 0)
        success = 0;
    if (success && !bit_writer_finish(&writer))
        success = 0;
    if (fclose(input) != 0)
        success = 0;
    if (fflush(output) != 0)
        success = 0;
    if (fclose(output) != 0)
        success = 0;
    if (success && (writer.bytes_written > ULONG_MAX - HEADER_SIZE))
        success = 0;
    output_size = HEADER_SIZE + writer.bytes_written;
    if (!success && created)
        remove(output_path);
    if (success) {
        *original_size = total;
        *compressed_size = output_size;
    }
    huffman_tree_free(root);
    return success;
}

int compressor_decompress(const char *input_path, const char *output_path,
                          unsigned long *restored_size)
{
    FILE *input;
    FILE *output;
    unsigned long frequencies[256];
    unsigned long decoded_frequencies[256];
    unsigned long original_size;
    unsigned long written;
    unsigned int valid_bits;
    unsigned int bit;
    unsigned int symbol;
    unsigned char output_buffer[IO_BUFFER_SIZE];
    size_t output_used;
    HuffmanNode *root;
    HuffmanNode *node;
    BitReader reader;
    int status;
    int success;
    int created;

    input = fopen(input_path, "rb");
    if (input == NULL)
        return 0;
    /* Etapa 5: valida o cabecalho, reconstroi a arvore e decodifica os bits. */
    if (!read_header(input, &original_size, &valid_bits, frequencies)) {
        fclose(input);
        return 0;
    }
    root = NULL;
    if (original_size != 0) {
        root = huffman_tree_build(frequencies);
        if (root == NULL) {
            fclose(input);
            return 0;
        }
    }
    if (!output_is_new(output_path)) {
        huffman_tree_free(root);
        fclose(input);
        return 0;
    }
    output = fopen(output_path, "wb");
    if (output == NULL) {
        huffman_tree_free(root);
        fclose(input);
        return 0;
    }
    created = 1;
    bit_reader_init(&reader, input);
    memset(decoded_frequencies, 0, sizeof(decoded_frequencies));
    written = 0;
    output_used = 0;
    success = 1;

    while (success && written < original_size) {
        if (root->is_leaf) {
            status = bit_reader_read_bit(&reader, &bit);
            if (status != 1 || bit != 0) {
                success = 0;
                break;
            }
            symbol = root->symbol;
        } else {
            node = root;
            while (!node->is_leaf) {
                status = bit_reader_read_bit(&reader, &bit);
                if (status != 1) {
                    success = 0;
                    break;
                }
                node = bit == 0 ? node->left : node->right;
                if (node == NULL) {
                    success = 0;
                    break;
                }
            }
            if (!success)
                break;
            symbol = node->symbol;
        }
        output_buffer[output_used++] = (unsigned char)symbol;
        ++decoded_frequencies[symbol];
        ++written;
        if (output_used == sizeof(output_buffer)) {
            if (fwrite(output_buffer, 1, output_used, output) != output_used) {
                success = 0;
                break;
            }
            output_used = 0;
        }
    }
    if (output_used != 0 &&
        fwrite(output_buffer, 1, output_used, output) != output_used)
        success = 0;
    if (success && memcmp(frequencies, decoded_frequencies,
                          sizeof(frequencies)) != 0)
        success = 0;
    if (success && !bit_reader_finish(&reader, valid_bits))
        success = 0;
    if (fclose(input) != 0)
        success = 0;
    if (fflush(output) != 0)
        success = 0;
    if (fclose(output) != 0)
        success = 0;
    if (!success && created)
        remove(output_path);
    if (success)
        *restored_size = original_size;
    huffman_tree_free(root);
    return success;
}