#include "compressor.h"

#include "bitio.h"
#include "dictionary.h"
#include "huffman_tree.h"

#include <stdio.h>
#include <string.h>

#define IO_BLOCK_SIZE 8192

static const unsigned char ZIPZ_MAGIC[4] = { 'Z', 'P', 'Z', '1' };

static void set_error(char *buffer, size_t buffer_size, const char *message) {
    if (buffer != NULL && buffer_size > 0U) {
        strncpy(buffer, message, buffer_size - 1U);
        buffer[buffer_size - 1U] = '\0';
    }
}

static int file_exists(const char *path) {
    FILE *fp;
    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

static int get_file_size(FILE *fp, unsigned long *size_out) {
    long current;
    long end;

    current = ftell(fp);
    if (current < 0L) {
        return 0;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        return 0;
    }
    end = ftell(fp);
    if (end < 0L) {
        return 0;
    }
    if (fseek(fp, current, SEEK_SET) != 0) {
        return 0;
    }

    *size_out = (unsigned long)end;
    return 1;
}

static int write_symbol_repeated(FILE *fp, unsigned char symbol, unsigned long count) {
    unsigned char buffer[IO_BLOCK_SIZE];
    unsigned long remaining;
    size_t i;

    for (i = 0U; i < IO_BLOCK_SIZE; ++i) {
        buffer[i] = symbol;
    }

    remaining = count;
    while (remaining > 0UL) {
        size_t chunk;
        if (remaining > (unsigned long)IO_BLOCK_SIZE) {
            chunk = IO_BLOCK_SIZE;
        } else {
            chunk = (size_t)remaining;
        }
        if (fwrite(buffer, 1U, chunk, fp) != chunk) {
            return 0;
        }
        remaining -= (unsigned long)chunk;
    }

    return 1;
}

int compress_file(const char *input_path,
                  const char *output_path,
                  ZipzStats *stats,
                  char *error_message,
                  size_t error_message_size) {
    FILE *input;
    FILE *output;
    unsigned char buffer[IO_BLOCK_SIZE];
    unsigned long frequencies[HUFFMAN_SYMBOLS];
    unsigned long original_size;
    size_t read_count;
    int i;
    HuffmanNode *tree;
    Dictionary dictionary;
    long valid_bits_position;
    unsigned char last_valid_bits;

    input = NULL;
    output = NULL;
    tree = NULL;
    original_size = 0UL;
    last_valid_bits = 0U;

    if (file_exists(output_path)) {
        set_error(error_message, error_message_size, "Arquivo de saída já existe.");
        return 0;
    }

    input = fopen(input_path, "rb");
    if (input == NULL) {
        set_error(error_message, error_message_size, "Não foi possível abrir o arquivo de entrada.");
        return 0;
    }

    for (i = 0; i < HUFFMAN_SYMBOLS; ++i) {
        frequencies[i] = 0UL;
    }

    /* Etapa 1: análise de frequência (1a leitura do arquivo). */
    while ((read_count = fread(buffer, 1U, IO_BLOCK_SIZE, input)) > 0U) {
        size_t j;
        original_size += (unsigned long)read_count;
        for (j = 0U; j < read_count; ++j) {
            frequencies[buffer[j]]++;
        }
    }
    if (ferror(input)) {
        set_error(error_message, error_message_size, "Falha na leitura do arquivo de entrada.");
        fclose(input);
        return 0;
    }

    /* Etapa 2: construção da árvore de Huffman. */
    if (original_size > 0UL) {
        tree = huffman_tree_build_from_frequencies(frequencies);
        if (tree == NULL) {
            set_error(error_message, error_message_size, "Falha ao construir a árvore de Huffman.");
            fclose(input);
            return 0;
        }

        /* Etapa 3: geração dos códigos prefixados. */
        if (!dictionary_build_from_tree(&dictionary, tree)) {
            set_error(error_message, error_message_size, "Falha ao gerar dicionário de códigos.");
            huffman_tree_free(tree);
            fclose(input);
            return 0;
        }
    }

    output = fopen(output_path, "wb");
    if (output == NULL) {
        set_error(error_message, error_message_size, "Não foi possível criar o arquivo de saída.");
        huffman_tree_free(tree);
        fclose(input);
        return 0;
    }

    /* Etapa 4: codificação com gravação de cabeçalho. */
    if (fwrite(ZIPZ_MAGIC, 1U, 4U, output) != 4U ||
        fwrite(&original_size, sizeof(unsigned long), 1U, output) != 1U ||
        fwrite(frequencies, sizeof(unsigned long), HUFFMAN_SYMBOLS, output) != HUFFMAN_SYMBOLS) {
        set_error(error_message, error_message_size, "Falha ao gravar cabeçalho .zipz.");
        huffman_tree_free(tree);
        fclose(input);
        fclose(output);
        return 0;
    }

    valid_bits_position = ftell(output);
    if (valid_bits_position < 0L) {
        set_error(error_message, error_message_size, "Falha ao posicionar cabeçalho .zipz.");
        huffman_tree_free(tree);
        fclose(input);
        fclose(output);
        return 0;
    }
    if (fputc(0, output) == EOF) {
        set_error(error_message, error_message_size, "Falha ao gravar cabeçalho .zipz.");
        huffman_tree_free(tree);
        fclose(input);
        fclose(output);
        return 0;
    }

    if (original_size > 0UL) {
        BitWriter writer;

        if (fseek(input, 0L, SEEK_SET) != 0) {
            set_error(error_message, error_message_size, "Falha ao reposicionar leitura do arquivo de entrada.");
            huffman_tree_free(tree);
            fclose(input);
            fclose(output);
            return 0;
        }

        bit_writer_init(&writer, output);

        while ((read_count = fread(buffer, 1U, IO_BLOCK_SIZE, input)) > 0U) {
            size_t j;
            for (j = 0U; j < read_count; ++j) {
                const HuffmanCode *code;
                code = dictionary_get_code(&dictionary, buffer[j]);
                if (code == NULL || !bit_writer_write_code(&writer, code->bits, code->length)) {
                    set_error(error_message, error_message_size, "Falha ao escrever dados comprimidos.");
                    huffman_tree_free(tree);
                    fclose(input);
                    fclose(output);
                    return 0;
                }
            }
        }
        if (ferror(input) || !bit_writer_finish(&writer, &last_valid_bits)) {
            set_error(error_message, error_message_size, "Falha na codificação do arquivo.");
            huffman_tree_free(tree);
            fclose(input);
            fclose(output);
            return 0;
        }
    }

    if (fseek(output, valid_bits_position, SEEK_SET) != 0 ||
        fwrite(&last_valid_bits, 1U, 1U, output) != 1U ||
        fseek(output, 0L, SEEK_END) != 0) {
        set_error(error_message, error_message_size, "Falha ao finalizar cabeçalho .zipz.");
        huffman_tree_free(tree);
        fclose(input);
        fclose(output);
        return 0;
    }

    if (stats != NULL) {
        unsigned long compressed_size;
        compressed_size = 0UL;
        if (!get_file_size(output, &compressed_size)) {
            set_error(error_message, error_message_size, "Falha ao obter tamanho do arquivo comprimido.");
            huffman_tree_free(tree);
            fclose(input);
            fclose(output);
            return 0;
        }
        stats->input_size = original_size;
        stats->output_size = compressed_size;
        if (original_size == 0UL) {
            stats->ratio = 0.0;
        } else {
            stats->ratio = (double)compressed_size / (double)original_size;
        }
    }

    huffman_tree_free(tree);
    fclose(input);
    if (fclose(output) != 0) {
        set_error(error_message, error_message_size, "Falha ao fechar arquivo comprimido.");
        return 0;
    }
    return 1;
}

int decompress_file(const char *input_path,
                    const char *output_path,
                    ZipzStats *stats,
                    char *error_message,
                    size_t error_message_size) {
    FILE *input;
    FILE *output;
    unsigned char magic[4];
    unsigned long original_size;
    unsigned long frequencies[HUFFMAN_SYMBOLS];
    unsigned char last_valid_bits;
    unsigned long frequency_sum;
    int i;
    HuffmanNode *tree;

    input = NULL;
    output = NULL;
    tree = NULL;
    original_size = 0UL;
    last_valid_bits = 0U;
    frequency_sum = 0UL;

    if (file_exists(output_path)) {
        set_error(error_message, error_message_size, "Arquivo de saída já existe.");
        return 0;
    }

    input = fopen(input_path, "rb");
    if (input == NULL) {
        set_error(error_message, error_message_size, "Não foi possível abrir o arquivo .zipz.");
        return 0;
    }

    /* Etapa 5: decodificação (leitura do cabeçalho e reconstrução da árvore). */
    if (fread(magic, 1U, 4U, input) != 4U || memcmp(magic, ZIPZ_MAGIC, 4U) != 0) {
        set_error(error_message, error_message_size, "Arquivo inválido: magic number .zipz incorreto.");
        fclose(input);
        return 0;
    }

    if (fread(&original_size, sizeof(unsigned long), 1U, input) != 1U ||
        fread(frequencies, sizeof(unsigned long), HUFFMAN_SYMBOLS, input) != HUFFMAN_SYMBOLS ||
        fread(&last_valid_bits, 1U, 1U, input) != 1U) {
        set_error(error_message, error_message_size, "Cabeçalho .zipz corrompido.");
        fclose(input);
        return 0;
    }

    for (i = 0; i < HUFFMAN_SYMBOLS; ++i) {
        frequency_sum += frequencies[i];
    }
    if (frequency_sum != original_size) {
        set_error(error_message, error_message_size, "Arquivo .zipz inválido: frequências inconsistentes.");
        fclose(input);
        return 0;
    }

    if (original_size > 0UL) {
        tree = huffman_tree_build_from_frequencies(frequencies);
        if (tree == NULL) {
            set_error(error_message, error_message_size, "Falha ao reconstruir árvore de Huffman.");
            fclose(input);
            return 0;
        }
    }

    output = fopen(output_path, "wb");
    if (output == NULL) {
        set_error(error_message, error_message_size, "Não foi possível criar o arquivo restaurado.");
        huffman_tree_free(tree);
        fclose(input);
        return 0;
    }

    if (original_size > 0UL) {
        if (huffman_tree_is_leaf(tree)) {
            if (!write_symbol_repeated(output, tree->symbol, original_size)) {
                set_error(error_message, error_message_size, "Falha ao escrever arquivo restaurado.");
                huffman_tree_free(tree);
                fclose(input);
                fclose(output);
                return 0;
            }
        } else {
            unsigned long produced;
            long header_end;
            long end_pos;
            unsigned long data_bytes;
            BitReader reader;
            unsigned char out_buffer[IO_BLOCK_SIZE];
            size_t out_count;

            header_end = ftell(input);
            if (header_end < 0L || fseek(input, 0L, SEEK_END) != 0) {
                set_error(error_message, error_message_size, "Falha ao validar dados comprimidos.");
                huffman_tree_free(tree);
                fclose(input);
                fclose(output);
                return 0;
            }
            end_pos = ftell(input);
            if (end_pos < header_end || fseek(input, header_end, SEEK_SET) != 0) {
                set_error(error_message, error_message_size, "Falha ao validar dados comprimidos.");
                huffman_tree_free(tree);
                fclose(input);
                fclose(output);
                return 0;
            }

            data_bytes = (unsigned long)(end_pos - header_end);
            if (data_bytes == 0UL || last_valid_bits == 0U || last_valid_bits > 8U) {
                set_error(error_message, error_message_size, "Arquivo .zipz inválido: bloco de bits inconsistente.");
                huffman_tree_free(tree);
                fclose(input);
                fclose(output);
                return 0;
            }

            bit_reader_init(&reader, input, data_bytes, last_valid_bits);
            produced = 0UL;
            out_count = 0U;

            while (produced < original_size) {
                const HuffmanNode *node;
                node = tree;

                while (!huffman_tree_is_leaf(node)) {
                    int bit;
                    if (!bit_reader_read_bit(&reader, &bit)) {
                        set_error(error_message, error_message_size, "Dados comprimidos truncados ou inválidos.");
                        huffman_tree_free(tree);
                        fclose(input);
                        fclose(output);
                        return 0;
                    }
                    node = bit ? node->right : node->left;
                    if (node == NULL) {
                        set_error(error_message, error_message_size, "Dados comprimidos inválidos para a árvore de Huffman.");
                        huffman_tree_free(tree);
                        fclose(input);
                        fclose(output);
                        return 0;
                    }
                }

                out_buffer[out_count] = node->symbol;
                out_count++;
                produced++;

                if (out_count == IO_BLOCK_SIZE) {
                    if (fwrite(out_buffer, 1U, out_count, output) != out_count) {
                        set_error(error_message, error_message_size, "Falha ao escrever arquivo restaurado.");
                        huffman_tree_free(tree);
                        fclose(input);
                        fclose(output);
                        return 0;
                    }
                    out_count = 0U;
                }
            }

            if (out_count > 0U && fwrite(out_buffer, 1U, out_count, output) != out_count) {
                set_error(error_message, error_message_size, "Falha ao escrever arquivo restaurado.");
                huffman_tree_free(tree);
                fclose(input);
                fclose(output);
                return 0;
            }
        }
    }

    if (stats != NULL) {
        unsigned long compressed_size;
        compressed_size = 0UL;
        if (!get_file_size(input, &compressed_size)) {
            set_error(error_message, error_message_size, "Falha ao obter tamanho do arquivo .zipz.");
            huffman_tree_free(tree);
            fclose(input);
            fclose(output);
            return 0;
        }
        stats->input_size = compressed_size;
        stats->output_size = original_size;
        if (compressed_size == 0UL) {
            stats->ratio = 0.0;
        } else {
            stats->ratio = (double)original_size / (double)compressed_size;
        }
    }

    huffman_tree_free(tree);
    if (fclose(output) != 0) {
        set_error(error_message, error_message_size, "Falha ao fechar arquivo restaurado.");
        fclose(input);
        return 0;
    }
    fclose(input);
    return 1;
}
