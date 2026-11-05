#include <stdio.h>
#include <string.h>
#include <time.h>
#include "analysis.h"
#include "compressor.h"
#include "format.h"
#include "search.h"

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

static const char *scenario_name(int scenario)
{
    switch (scenario) {
    case SCENARIO_LOW:
        return "BAIXA redundancia (como testes/baixa.bin)";
    case SCENARIO_MEDIUM:
        return "MEDIA redundancia (como testes/media.txt)";
    case SCENARIO_HIGH:
        return "ALTA redundancia (como testes/alta.txt)";
    }
    return "indefinido (arquivo com menos de 8 bytes)";
}

static void print_verdict(const Analysis *analysis)
{
    if (analysis->size == 0)
        printf("Veredito: NAO COMPENSA, arquivo vazio (o .zipz teria %lu Bytes).\n",
               ZIPZ_HEADER_SIZE);
    else if (analysis->verdict == VERDICT_NOT_WORTH)
        printf("Veredito: NAO COMPENSA, o .zipz ficaria %.0f Bytes maior que o original.\n",
               analysis->predicted_size - (double)analysis->size);
    else if (analysis->verdict == VERDICT_LITTLE)
        printf("Veredito: COMPENSA POUCO, economia prevista de %.1f%%.\n",
               (1.0 - analysis->predicted_ratio) * 100.0);
    else
        printf("Veredito: COMPENSA, economia prevista de %.1f%%.\n",
               (1.0 - analysis->predicted_ratio) * 100.0);
}

static void print_analysis(const Analysis *analysis, int full)
{
    double repeated;

    repeated = 0.0;
    if (analysis->windows > 0)
        repeated = 1.0 - (double)analysis->new_windows / (double)analysis->windows;
    if (analysis->size == 0) {
        print_verdict(analysis);
        return;
    }
    if (!full) {
        printf("\nAnalise previa: entropia %.4f bits/byte, razao minima %.4f, "
               "razao prevista %.4f\n", analysis->entropy, analysis->min_ratio,
               analysis->predicted_ratio);
        printf("Cenario: %s\n", scenario_name(analysis->scenario));
        print_verdict(analysis);
        return;
    }
    printf("Tamanho: %lu Bytes   Bytes distintos: %u de 256\n",
           analysis->size, analysis->distinct);
    printf("Entropia: %.4f bits por byte (maximo 8)\n", analysis->entropy);
    printf("Razao minima teorica (entropia / 8): %.4f\n", analysis->min_ratio);
    printf("Huffman do Zipz: %.4f bits por byte\n", analysis->bits_per_byte);
    printf("Razao prevista do Zipz: %.4f (%.0f Bytes, com cabecalho de %lu)\n",
           analysis->predicted_ratio, analysis->predicted_size, ZIPZ_HEADER_SIZE);
    if (analysis->windows > 0) {
        printf("Trechos de 8 bytes repetidos: %.3f%%", repeated * 100.0);
        if (analysis->sample_full)
            printf(" (nos primeiros %lu Bytes)", analysis->sampled);
        printf("\n");
    }
    printf("Cenario: %s\n", scenario_name(analysis->scenario));
    print_verdict(analysis);
    if (repeated >= 0.9) {
        printf("Obs.: a maior parte do arquivo repete trechos anteriores. O Huffman por\n"
               "byte nao aproveita isso; com LZ77 (como no gzip) daria para ficar bem\n"
               "abaixo de %.4f.\n", analysis->min_ratio);
    }
}

static void compress_action(void)
{
    char input_path[PATH_SIZE];
    char output_path[PATH_SIZE];
    char answer[PATH_SIZE];
    size_t length;
    unsigned long original_size;
    unsigned long compressed_size;
    clock_t start;
    clock_t end;
    double seconds;
    Analysis analysis;

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
    if (!analysis_run(input_path, &analysis)) {
        printf("Falha na compressao. Verifique a entrada, a saida e as permissoes.\n");
        return;
    }
    print_analysis(&analysis, 0);
    if (analysis.verdict == VERDICT_NOT_WORTH) {
        if (!read_line("Comprimir mesmo assim? (s/n): ", answer) ||
            (strcmp(answer, "s") != 0 && strcmp(answer, "S") != 0)) {
            printf("Compressao cancelada.\n");
            return;
        }
    }
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

static void search_action(void)
{
    char input_path[PATH_SIZE];
    char word[PATH_SIZE];
    SearchResult result;
    unsigned int index;
    clock_t start;
    clock_t end;
    double seconds;

    if (!read_line("Arquivo .zipz para buscar: ", input_path) ||
        input_path[0] == '\0' ||
        !read_line("Palavra ou trecho: ", word) || word[0] == '\0') {
        printf("Caminho ou palavra invalida.\n");
        return;
    }
    start = clock();
    if (!search_zipz(input_path, (const unsigned char *)word, strlen(word),
                     &result)) {
        printf("Falha na busca. Arquivo invalido ou erro de leitura.\n");
        return;
    }
    end = clock();
    seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;
    printf("\nOcorrencias de \"%s\": %lu\n", word, result.count);
    if (result.count > 0) {
        printf("Posicoes (byte a partir de 0, linha e coluna a partir de 1):\n");
        for (index = 0; index < result.listed; ++index)
            printf("  %2u. byte %lu (linha %lu, coluna %lu)\n", index + 1,
                   result.offsets[index], result.lines[index],
                   result.columns[index]);
        if (result.count > result.listed)
            printf("  ... e mais %lu ocorrencias.\n",
                   result.count - result.listed);
    }
    printf("Bytes decodificados: %lu (nenhum arquivo foi criado)\n",
           result.decoded);
    printf("Tempo de busca: %.3f s\n", seconds);
}

static void analysis_action(void)
{
    char input_path[PATH_SIZE];
    Analysis analysis;

    if (!read_line("Arquivo para analisar: ", input_path) ||
        input_path[0] == '\0') {
        printf("Caminho invalido.\n");
        return;
    }
    if (!analysis_run(input_path, &analysis)) {
        printf("Falha na analise. Verifique o arquivo e as permissoes.\n");
        return;
    }
    printf("\nArquivo: %s\n", input_path);
    print_analysis(&analysis, 1);
}

int main(void)
{
    char option[PATH_SIZE];

    for (;;) {
        printf("\nPrograma - UFABC Zipz\n\n");
        printf("1. Comprimir\n2. Descomprimir\n3. Sair\n");
        printf("4. Buscar palavra em arquivo .zipz\n5. Analisar redundancia\n");
        if (!read_line("Opcao: ", option)) {
            if (feof(stdin) || ferror(stdin))
                break;
            printf("Entrada invalida.\n");
            continue;
        }
        if (strcmp(option, "1") == 0)
            compress_action();
        else if (strcmp(option, "2") == 0)
            decompress_action();
        else if (strcmp(option, "3") == 0)
            break;
        else if (strcmp(option, "4") == 0)
            search_action();
        else if (strcmp(option, "5") == 0)
            analysis_action();
        else
            printf("Opcao invalida.\n");
    }
    return 0;
}
