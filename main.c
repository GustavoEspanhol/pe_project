#include "compressor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INPUT_BUFFER_SIZE 1024
#define ERROR_BUFFER_SIZE 256

static void trim_newline(char *text) {
    size_t len;
    len = strlen(text);
    if (len > 0U && text[len - 1U] == '\n') {
        text[len - 1U] = '\0';
    }
}

static int read_line(const char *prompt, char *buffer, size_t buffer_size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return 0;
    }
    trim_newline(buffer);
    return 1;
}

static void run_compress(void) {
    char input_path[INPUT_BUFFER_SIZE];
    char error_message[ERROR_BUFFER_SIZE];
    char *output_path;
    size_t output_len;
    ZipzStats stats;
    clock_t start_time;
    clock_t end_time;
    double elapsed;

    if (!read_line("Arquivo de entrada: ", input_path, sizeof(input_path))) {
        printf("Falha ao ler caminho do arquivo.\n");
        return;
    }
    if (input_path[0] == '\0') {
        printf("Caminho inválido.\n");
        return;
    }

    output_len = strlen(input_path) + 6U;
    output_path = (char *)malloc(output_len);
    if (output_path == NULL) {
        printf("Falha de memória ao montar nome de saída.\n");
        return;
    }

    strcpy(output_path, input_path);
    strcat(output_path, ".zipz");

    start_time = clock();
    if (!compress_file(input_path, output_path, &stats, error_message, sizeof(error_message))) {
        printf("Erro na compressão: %s\n", error_message);
        free(output_path);
        return;
    }
    end_time = clock();

    elapsed = (double)(end_time - start_time) / (double)CLOCKS_PER_SEC;

    printf("Arquivo original: %s   Tamanho: %lu Bytes\n", input_path, stats.input_size);
    printf("Arquivo comprimido: %s   Tamanho: %lu Bytes\n", output_path, stats.output_size);
    printf("Razão de compressão: %.2f\n", stats.ratio);
    printf("Tempo de compressão: %.2f s\n", elapsed);

    free(output_path);
}

static void run_decompress(void) {
    char input_path[INPUT_BUFFER_SIZE];
    char output_path[INPUT_BUFFER_SIZE];
    char error_message[ERROR_BUFFER_SIZE];
    ZipzStats stats;
    clock_t start_time;
    clock_t end_time;
    double elapsed;

    if (!read_line("Arquivo .zipz de entrada: ", input_path, sizeof(input_path))) {
        printf("Falha ao ler caminho do arquivo.\n");
        return;
    }
    if (input_path[0] == '\0') {
        printf("Caminho inválido.\n");
        return;
    }

    if (!read_line("Nome do arquivo restaurado: ", output_path, sizeof(output_path))) {
        printf("Falha ao ler caminho de saída.\n");
        return;
    }
    if (output_path[0] == '\0') {
        printf("Caminho de saída inválido.\n");
        return;
    }

    start_time = clock();
    if (!decompress_file(input_path, output_path, &stats, error_message, sizeof(error_message))) {
        printf("Erro na descompressão: %s\n", error_message);
        return;
    }
    end_time = clock();

    elapsed = (double)(end_time - start_time) / (double)CLOCKS_PER_SEC;

    printf("Arquivo comprimido: %s   Tamanho: %lu Bytes\n", input_path, stats.input_size);
    printf("Arquivo restaurado: %s   Tamanho: %lu Bytes\n", output_path, stats.output_size);
    printf("Tempo de descompressão: %.2f s\n", elapsed);
}

int main(void) {
    int option;
    char option_line[32];

    while (1) {
        printf("\nPrograma - UFABC Zipz\n");
        printf("1) Comprimir\n");
        printf("2) Descomprimir\n");
        printf("3) Sair\n");
        printf("Opção: ");

        if (fgets(option_line, sizeof(option_line), stdin) == NULL) {
            break;
        }

        option = atoi(option_line);

        if (option == 1) {
            run_compress();
        } else if (option == 2) {
            run_decompress();
        } else if (option == 3) {
            printf("Saindo...\n");
            break;
        } else {
            printf("Opção inválida.\n");
        }
    }

    return 0;
}
