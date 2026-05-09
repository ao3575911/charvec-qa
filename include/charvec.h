#ifndef CHARVEC_H
#define CHARVEC_H

#include <stddef.h>

#define CHARVEC_MAX_TEXT 1024
#define CHARVEC_MAX_CHARSET 256

typedef struct {
    char question[CHARVEC_MAX_TEXT];
    char answer[CHARVEC_MAX_TEXT];
    char *question_bits;
    char *answer_bits;
} CharvecPair;

typedef struct {
    CharvecPair *pairs;
    size_t pair_count;
    size_t max_text_len;
    unsigned char charset[CHARVEC_MAX_CHARSET];
    size_t charset_size;
} CharvecModel;

typedef struct {
    size_t index;
    double score;
} CharvecMatch;

void charvec_model_init(CharvecModel *model);
void charvec_model_free(CharvecModel *model);

int charvec_load_dataset(const char *path, CharvecModel *model, char *error, size_t error_size);
int charvec_build_charset(CharvecModel *model, char *error, size_t error_size);
int charvec_encode_model(CharvecModel *model, char *error, size_t error_size);
int charvec_save_model(const char *path, const CharvecModel *model, char *error, size_t error_size);
int charvec_load_model(const char *path, CharvecModel *model, char *error, size_t error_size);
int charvec_find_best(const CharvecModel *model, const char *query, CharvecMatch *match, char *error, size_t error_size);

#endif

