#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis.h"
#include "format.h"
#include "huffman_tree.h"

#define BUFFER_SIZE 8192
#define WINDOW 8
#define TABLE_SLOTS 2097152UL
#define TABLE_LIMIT 1572864UL

/* limites dos cenarios, calibrados com os arquivos de testes/ */
#define LOW_MIN_ENTROPY 7.5
#define LOW_MIN_NEW 0.5
#define HIGH_MAX_NEW 0.001
#define LITTLE_GAIN_RATIO 0.9

typedef struct {
    unsigned char *keys;
    unsigned char *used;
    unsigned long count;
} WindowSet;

static unsigned long window_hash(const unsigned char *window)
{
    unsigned long hash;
    unsigned int index;

    hash = 2166136261UL;
    for (index = 0; index < WINDOW; ++index) {
        hash ^= window[index];
        hash = (hash * 16777619UL) & 0xFFFFFFFFUL;
    }
    return hash;
}

/* devolve 1 se a janela ainda nao tinha aparecido */
static int window_insert(WindowSet *set, const unsigned char *window)
{
    unsigned long slot;

    slot = window_hash(window) & (TABLE_SLOTS - 1);
    while (set->used[slot]) {
        if (memcmp(set->keys + slot * WINDOW, window, WINDOW) == 0)
            return 0;
        slot = (slot + 1) & (TABLE_SLOTS - 1);
    }
    set->used[slot] = 1;
    memcpy(set->keys + slot * WINDOW, window, WINDOW);
    ++set->count;
    return 1;
}

static double tree_bits(const HuffmanNode *node, unsigned int depth)
{
    if (node->is_leaf)
        return (double)node->frequency * (depth == 0 ? 1 : depth);
    return tree_bits(node->left, depth + 1) + tree_bits(node->right, depth + 1);
}

static int read_file(const char *path, Analysis *analysis,
                     unsigned long frequencies[256])
{
    FILE *input;
    unsigned char buffer[BUFFER_SIZE];
    unsigned char window[WINDOW];
    size_t count;
    size_t index;
    WindowSet set;
    int success;

    input = fopen(path, "rb");
    if (input == NULL)
        return 0;
    set.keys = (unsigned char *)malloc(TABLE_SLOTS * WINDOW);
    set.used = (unsigned char *)calloc(TABLE_SLOTS, 1);
    set.count = 0;
    if (set.keys == NULL || set.used == NULL) {
        free(set.keys);
        free(set.used);
        set.keys = NULL;
        set.used = NULL;
    }
    memset(window, 0, sizeof(window));
    success = 1;

    while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
        if (analysis->size > ULONG_MAX - count) {
            success = 0;
            break;
        }
        for (index = 0; index < count; ++index) {
            ++frequencies[buffer[index]];
            if (set.used == NULL || analysis->sample_full)
                continue;
            memmove(window, window + 1, WINDOW - 1);
            window[WINDOW - 1] = buffer[index];
            ++analysis->sampled;
            if (analysis->sampled < WINDOW)
                continue;
            ++analysis->windows;
            analysis->new_windows += window_insert(&set, window);
            if (set.count >= TABLE_LIMIT)
                analysis->sample_full = 1;
        }
        analysis->size += (unsigned long)count;
    }
    if (ferror(input))
        success = 0;
    fclose(input);
    free(set.keys);
    free(set.used);
    return success;
}

int analysis_run(const char *path, Analysis *analysis)
{
    unsigned long frequencies[256];
    unsigned int symbol;
    double p;
    double bits;
    double new_share;
    HuffmanNode *root;

    memset(analysis, 0, sizeof(Analysis));
    memset(frequencies, 0, sizeof(frequencies));
    if (!read_file(path, analysis, frequencies))
        return 0;

    for (symbol = 0; symbol < 256; ++symbol) {
        if (frequencies[symbol] == 0)
            continue;
        ++analysis->distinct;
        p = (double)frequencies[symbol] / (double)analysis->size;
        analysis->entropy += p * (log(1.0 / p) / log(2.0));
    }
    analysis->min_ratio = analysis->entropy / 8.0;

    bits = 0.0;
    if (analysis->size > 0) {
        root = huffman_tree_build(frequencies);
        if (root == NULL)
            return 0;
        bits = tree_bits(root, 0);
        huffman_tree_free(root);
        analysis->bits_per_byte = bits / (double)analysis->size;
        analysis->predicted_size = (double)ZIPZ_HEADER_SIZE + ceil(bits / 8.0);
        analysis->predicted_ratio =
            analysis->predicted_size / (double)analysis->size;
    } else {
        analysis->predicted_size = (double)ZIPZ_HEADER_SIZE;
    }

    if (analysis->windows == 0) {
        analysis->scenario = SCENARIO_UNKNOWN;
    } else {
        new_share = (double)analysis->new_windows / (double)analysis->windows;
        if (new_share < HIGH_MAX_NEW)
            analysis->scenario = SCENARIO_HIGH;
        else if (analysis->entropy >= LOW_MIN_ENTROPY && new_share >= LOW_MIN_NEW)
            analysis->scenario = SCENARIO_LOW;
        else
            analysis->scenario = SCENARIO_MEDIUM;
    }

    if (analysis->size == 0 || analysis->predicted_ratio >= 1.0)
        analysis->verdict = VERDICT_NOT_WORTH;
    else if (analysis->predicted_ratio > LITTLE_GAIN_RATIO)
        analysis->verdict = VERDICT_LITTLE;
    else
        analysis->verdict = VERDICT_WORTH;
    return 1;
}
