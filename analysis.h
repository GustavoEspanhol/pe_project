#ifndef ANALYSIS_H
#define ANALYSIS_H

#define SCENARIO_UNKNOWN 0
#define SCENARIO_LOW 1
#define SCENARIO_MEDIUM 2
#define SCENARIO_HIGH 3

#define VERDICT_WORTH 0
#define VERDICT_LITTLE 1
#define VERDICT_NOT_WORTH 2

typedef struct {
    unsigned long size;
    unsigned int distinct;
    double entropy;
    double min_ratio;
    double bits_per_byte;
    double predicted_size;
    double predicted_ratio;
    unsigned long windows;
    unsigned long new_windows;
    unsigned long sampled;
    int sample_full;
    int scenario;
    int verdict;
} Analysis;

int analysis_run(const char *path, Analysis *analysis);

#endif
