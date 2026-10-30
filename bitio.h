#ifndef BITIO_H
#define BITIO_H

#include <stdio.h>

#define BITIO_BUFFER_SIZE 4096

typedef struct {
    FILE *fp;
    unsigned char current_byte;
    int bit_count;
    unsigned char io_buffer[BITIO_BUFFER_SIZE];
    size_t io_count;
    unsigned long data_bytes_written;
} BitWriter;

typedef struct {
    FILE *fp;
    unsigned char current_byte;
    int bits_remaining;
    unsigned char io_buffer[BITIO_BUFFER_SIZE];
    size_t io_pos;
    size_t io_size;
    unsigned long bytes_consumed;
    unsigned long data_bytes;
    unsigned char last_byte_valid_bits;
} BitReader;

void bit_writer_init(BitWriter *writer, FILE *fp);
int bit_writer_write_bit(BitWriter *writer, int bit);
int bit_writer_write_code(BitWriter *writer, const unsigned char *bits, unsigned short length);
int bit_writer_finish(BitWriter *writer, unsigned char *last_valid_bits);

void bit_reader_init(BitReader *reader, FILE *fp, unsigned long data_bytes, unsigned char last_byte_valid_bits);
int bit_reader_read_bit(BitReader *reader, int *bit);

#endif
