#ifndef COMPRESSOR_H
#define COMPRESSOR_H

int compressor_compress(const char *input_path, const char *output_path,
                        unsigned long *original_size,
                        unsigned long *compressed_size);
int compressor_decompress(const char *input_path, const char *output_path,
                          unsigned long *restored_size);

#endif