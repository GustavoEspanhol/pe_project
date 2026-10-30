#include <stdio.h>
#include <string.h>
#include <time.h>
#include "compressor.h"

#define PATH_SIZE 4096

static int read_line(const char *prompt, char buffer[PATH_SIZE])
{
    size_t length;
    int character;

    printf("%s", prompt);
    if (fgets(buffer, PATH_SIZE, stdin) == NULL)
        return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else if (!feof(stdin)) {
        while ((character = getchar()) != '\n' && character != EOF)
            ;
        return 0;
    }
    if (length > 0 && buffer[length - 1] == '\r')
        buffer[length - 1] = '\0';
    return 1;
}

static void compress_action(void)
{
    char input_path[PATH_SIZE];
    char output_path[PATH_SIZE];
    size_t length;
    unsigned long original_size;
    unsigned long compressed_size;
    clock_t start;
    clock_t end;
    double seconds;

    if (!read_line("Arquivo para comprimir: ", input_path) ||
        input_path[0] == '\0') {
        printf("Caminho invalido.\n");
        return;
    }
    length = strlen(input_path);
    if (length + 5 > PATH_SIZE) {
        printf("Caminho muito longo para acrescentar .zipz.\n");
        return;
    }
    memcpy(output_path, input_path, length);
    memcpy(output_path + length, ".zipz", 6);
    start = clock();
    if (!compressor_compress(input_path, output_path, &original_size,
                             &compressed_size)) {
        printf("Falha na compressao. Verifique a entrada, a saida e as permissoes.\n");
        return;
    }
    end = clock();
    seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;
    printf("\nArquivo original: %s   Tamanho: %lu Bytes\n",
           input_path, original_size);
    printf("Arquivo comprimido: %s   Tamanho: %lu Bytes\n",
           output_path, compressed_size);
    if (original_size == 0)
        printf("Razao de compressao: N/A (arquivo vazio)\n");
    else
        printf("Razao de compressao: %.4f\n",
               (double)compressed_size / (double)original_size);
    printf("Tempo de compressao: %.3f s\n", seconds);
}

static void decompress_action(void)
{
    char input_path[PATH_SIZE];
    char output_path[PATH_SIZE];
    unsigned long restored_size;
    clock_t start;
    clock_t end;
    double seconds;

    if (!read_line("Arquivo .zipz para descomprimir: ", input_path) ||
        input_path[0] == '\0' ||
        !read_line("Nome do arquivo restaurado: ", output_path) ||
        output_path[0] == '\0') {
        printf("Caminho invalido.\n");
        return;
    }
    start = clock();
    if (!compressor_decompress(input_path, output_path, &restored_size)) {
        printf("Falha na descompressao. Arquivo invalido ou erro de leitura/escrita.\n");
        return;
    }
    end = clock();
    seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;
    printf("\nArquivo restaurado: %s   Tamanho: %lu Bytes\n",
           output_path, restored_size);
    printf("Tempo de descompressao: %.3f s\n", seconds);
}

int main(void)
{
    char option[PATH_SIZE];

    for (;;) {
        printf("\nPrograma - UFABC Zipz\n\n");
        printf("1. Comprimir\n2. Descomprimir\n3. Sair\n");
        if (!read_line("Opcao: ", option)) {
            printf("Entrada invalida.\n");
            continue;
        }
        if (strcmp(option, "1") == 0)
            compress_action();
        else if (strcmp(option, "2") == 0)
            decompress_action();
        else if (strcmp(option, "3") == 0)
            break;
        else
            printf("Opcao invalida.\n");
    }
    return 0;
}