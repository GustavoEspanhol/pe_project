#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stddef.h>

typedef struct {
    unsigned long input_size;
    unsigned long output_size;
    double ratio;
} ZipzStats;

int compress_file(const char *input_path,
                  const char *output_path,
                  ZipzStats *stats,
                  char *error_message,
                  size_t error_message_size);

int decompress_file(const char *input_path,
                    const char *output_path,
                    ZipzStats *stats,
                    char *error_message,
                    size_t error_message_size);

#endif
