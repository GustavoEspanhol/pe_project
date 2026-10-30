#ifndef BITIO_H
#define BITIO_H

#include <stdio.h>

#define BITIO_BUFFER_SIZE 8192

typedef struct {
    FILE *file;
    unsigned char buffer[BITIO_BUFFER_SIZE];
    unsigned int used;
    unsigned char current;
    unsigned int bit_count;
    unsigned long bytes_written;
    int failed;
} BitWriter;

typedef struct {
    FILE *file;
    unsigned char buffer[BITIO_BUFFER_SIZE];
    size_t position;
    size_t length;
    unsigned char current;
    unsigned int remaining;
    int has_current;
    int failed;
} BitReader;

void bit_writer_init(BitWriter *writer, FILE *file);
int bit_writer_write_bit(BitWriter *writer, unsigned int bit);
int bit_writer_write_code(BitWriter *writer, const unsigned char *bits,
                          unsigned int length);
int bit_writer_finish(BitWriter *writer);
void bit_reader_init(BitReader *reader, FILE *file);
int bit_reader_read_bit(BitReader *reader, unsigned int *bit);
int bit_reader_finish(BitReader *reader, unsigned int valid_bits);

#endif