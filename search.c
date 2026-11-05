#include <stdlib.h>
#include "decoder.h"
#include "search.h"

/* tabela de prefixos do KMP */
static void build_prefix(const unsigned char *pattern, size_t length,
                         size_t *prefix)
{
    size_t index;
    size_t matched;

    prefix[0] = 0;
    matched = 0;
    for (index = 1; index < length; ++index) {
        while (matched > 0 && pattern[index] != pattern[matched])
            matched = prefix[matched - 1];
        if (pattern[index] == pattern[matched])
            ++matched;
        prefix[index] = matched;
    }
}

int search_zipz(const char *path, const unsigned char *pattern, size_t length,
                SearchResult *result)
{
    Decoder decoder;
    size_t *prefix;
    size_t matched;
    unsigned char byte;
    unsigned long position;
    unsigned long start;
    unsigned long line;
    unsigned long line_start;
    int status;
    int success;

    result->count = 0;
    result->decoded = 0;
    result->listed = 0;
    if (pattern == NULL || length == 0)
        return 0;
    prefix = (size_t *)malloc(length * sizeof(size_t));
    if (prefix == NULL)
        return 0;
    build_prefix(pattern, length, prefix);
    if (!decoder_open(&decoder, path)) {
        free(prefix);
        return 0;
    }

    matched = 0;
    position = 0;
    line = 1;
    line_start = 0;
    while ((status = decoder_next(&decoder, &byte)) == 1) {
        while (matched > 0 && byte != pattern[matched])
            matched = prefix[matched - 1];
        if (byte == pattern[matched])
            ++matched;
        if (matched == length) {
            start = position - (unsigned long)(length - 1);
            if (result->listed < SEARCH_MAX_LISTED) {
                result->offsets[result->listed] = start;
                result->lines[result->listed] = line;
                result->columns[result->listed] = start - line_start + 1;
                ++result->listed;
            }
            ++result->count;
            matched = prefix[length - 1];
        }
        if (byte == '\n') {
            ++line;
            line_start = position + 1;
        }
        ++position;
    }
    success = status == 0 && decoder_finish(&decoder);
    if (!decoder_close(&decoder))
        success = 0;
    result->decoded = position;
    free(prefix);
    return success;
}
