#include <limits.h>
#include <string.h>
#include "bitio.h"

static int flush_writer_buffer(BitWriter *writer)
{
    if (writer->used == 0)
        return 1;
    if (writer->bytes_written > ULONG_MAX - writer->used ||
        fwrite(writer->buffer, 1, writer->used, writer->file) != writer->used) {
        writer->failed = 1;
        return 0;
    }
    writer->bytes_written += writer->used;
    writer->used = 0;
    return 1;
}

static int append_byte(BitWriter *writer, unsigned char byte)
{
    writer->buffer[writer->used++] = byte;
    if (writer->used == BITIO_BUFFER_SIZE)
        return flush_writer_buffer(writer);
    return 1;
}

void bit_writer_init(BitWriter *writer, FILE *file)
{
    writer->file = file;
    writer->used = 0;
    writer->current = 0;
    writer->bit_count = 0;
    writer->bytes_written = 0;
    writer->failed = 0;
}

int bit_writer_write_bit(BitWriter *writer, unsigned int bit)
{
    if (writer->failed)
        return 0;
    writer->current = (unsigned char)((writer->current << 1) | (bit & 1));
    ++writer->bit_count;
    if (writer->bit_count == 8) {
        if (!append_byte(writer, writer->current))
            return 0;
        writer->current = 0;
        writer->bit_count = 0;
    }
    return 1;
}

int bit_writer_write_code(BitWriter *writer, const unsigned char *bits,
                          unsigned int length)
{
    unsigned int index;

    for (index = 0; index < length; ++index) {
        if (!bit_writer_write_bit(writer, bits[index]))
            return 0;
    }
    return 1;
}

int bit_writer_finish(BitWriter *writer)
{
    if (writer->failed)
        return 0;
    if (writer->bit_count != 0) {
        writer->current = (unsigned char)(writer->current <<
                                          (8 - writer->bit_count));
        if (!append_byte(writer, writer->current))
            return 0;
    }
    return flush_writer_buffer(writer);
}

void bit_reader_init(BitReader *reader, FILE *file)
{
    reader->file = file;
    reader->position = 0;
    reader->length = 0;
    reader->current = 0;
    reader->remaining = 0;
    reader->has_current = 0;
    reader->failed = 0;
}

int bit_reader_read_bit(BitReader *reader, unsigned int *bit)
{
    size_t count;

    if (reader->failed)
        return -1;
    if (reader->remaining == 0) {
        if (reader->position == reader->length) {
            count = fread(reader->buffer, 1, BITIO_BUFFER_SIZE, reader->file);
            if (count == 0) {
                if (ferror(reader->file)) {
                    reader->failed = 1;
                    return -1;
                }
                return 0;
            }
            reader->position = 0;
            reader->length = count;
        }
        reader->current = reader->buffer[reader->position++];
        reader->remaining = 8;
        reader->has_current = 1;
    }
    *bit = (reader->current >> 7) & 1;
    reader->current = (unsigned char)(reader->current << 1);
    --reader->remaining;
    return 1;
}

int bit_reader_finish(BitReader *reader, unsigned int valid_bits)
{
    unsigned char extra[BITIO_BUFFER_SIZE];
    size_t count;

    if (reader->failed)
        return 0;
    if (valid_bits == 0) {
        if (reader->has_current)
            return 0;
    } else {
        if (!reader->has_current || valid_bits > 8 ||
            reader->remaining != 8 - valid_bits || reader->current != 0)
            return 0;
    }
    if (reader->position != reader->length)
        return 0;
    count = fread(extra, 1, sizeof(extra), reader->file);
    if (count != 0 || ferror(reader->file))
        return 0;
    return 1;
}