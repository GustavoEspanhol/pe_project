#include "bitio.h"

static int bit_writer_flush_buffer(BitWriter *writer) {
    if (writer->io_count == 0U) {
        return 1;
    }
    if (fwrite(writer->io_buffer, 1U, writer->io_count, writer->fp) != writer->io_count) {
        return 0;
    }
    writer->io_count = 0U;
    return 1;
}

static int bit_writer_push_byte(BitWriter *writer, unsigned char value) {
    writer->io_buffer[writer->io_count] = value;
    writer->io_count++;
    writer->data_bytes_written++;

    if (writer->io_count == BITIO_BUFFER_SIZE) {
        return bit_writer_flush_buffer(writer);
    }
    return 1;
}

void bit_writer_init(BitWriter *writer, FILE *fp) {
    writer->fp = fp;
    writer->current_byte = 0U;
    writer->bit_count = 0;
    writer->io_count = 0U;
    writer->data_bytes_written = 0UL;
}

int bit_writer_write_bit(BitWriter *writer, int bit) {
    writer->current_byte = (unsigned char)((writer->current_byte << 1) | (bit ? 1 : 0));
    writer->bit_count++;

    if (writer->bit_count == 8) {
        if (!bit_writer_push_byte(writer, writer->current_byte)) {
            return 0;
        }
        writer->current_byte = 0U;
        writer->bit_count = 0;
    }
    return 1;
}

int bit_writer_write_code(BitWriter *writer, const unsigned char *bits, unsigned short length) {
    unsigned short i;
    for (i = 0U; i < length; ++i) {
        unsigned short byte_index;
        unsigned short bit_index;
        int bit;

        byte_index = (unsigned short)(i / 8U);
        bit_index = (unsigned short)(7U - (i % 8U));
        bit = ((bits[byte_index] & (unsigned char)(1U << bit_index)) != 0U) ? 1 : 0;

        if (!bit_writer_write_bit(writer, bit)) {
            return 0;
        }
    }
    return 1;
}

int bit_writer_finish(BitWriter *writer, unsigned char *last_valid_bits) {
    unsigned char valid;
    if (writer->bit_count > 0) {
        writer->current_byte = (unsigned char)(writer->current_byte << (8 - writer->bit_count));
        if (!bit_writer_push_byte(writer, writer->current_byte)) {
            return 0;
        }
        valid = (unsigned char)writer->bit_count;
    } else if (writer->data_bytes_written > 0UL) {
        valid = 8U;
    } else {
        valid = 0U;
    }

    if (!bit_writer_flush_buffer(writer)) {
        return 0;
    }

    if (last_valid_bits != NULL) {
        *last_valid_bits = valid;
    }
    return 1;
}

static int bit_reader_fill_buffer(BitReader *reader) {
    if (reader->bytes_consumed >= reader->data_bytes) {
        return 0;
    }
    reader->io_size = fread(reader->io_buffer, 1U, BITIO_BUFFER_SIZE, reader->fp);
    reader->io_pos = 0U;
    return reader->io_size > 0U;
}

static int bit_reader_next_byte(BitReader *reader, unsigned char *value) {
    if (reader->bytes_consumed >= reader->data_bytes) {
        return 0;
    }

    if (reader->io_pos >= reader->io_size) {
        if (!bit_reader_fill_buffer(reader)) {
            return 0;
        }
    }

    *value = reader->io_buffer[reader->io_pos];
    reader->io_pos++;
    reader->bytes_consumed++;
    return 1;
}

void bit_reader_init(BitReader *reader, FILE *fp, unsigned long data_bytes, unsigned char last_byte_valid_bits) {
    reader->fp = fp;
    reader->current_byte = 0U;
    reader->bits_remaining = 0;
    reader->io_pos = 0U;
    reader->io_size = 0U;
    reader->bytes_consumed = 0UL;
    reader->data_bytes = data_bytes;
    reader->last_byte_valid_bits = last_byte_valid_bits;
}

int bit_reader_read_bit(BitReader *reader, int *bit) {
    if (reader->bits_remaining == 0) {
        unsigned char next_byte;
        int remaining_bits;

        if (!bit_reader_next_byte(reader, &next_byte)) {
            return 0;
        }

        reader->current_byte = next_byte;
        remaining_bits = 8;
        if (reader->bytes_consumed == reader->data_bytes) {
            remaining_bits = (int)reader->last_byte_valid_bits;
        }
        if (remaining_bits <= 0) {
            return 0;
        }
        reader->bits_remaining = remaining_bits;
    }

    *bit = ((reader->current_byte & 0x80U) != 0U) ? 1 : 0;
    reader->current_byte = (unsigned char)(reader->current_byte << 1);
    reader->bits_remaining--;
    return 1;
}
