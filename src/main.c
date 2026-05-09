#include "charvec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *stream) {
    fprintf(stream,
            "Usage:\n"
            "  charvec build -i <dataset.txt> -o <model.cvec>\n"
            "  charvec ask -m <model.cvec> -q <question>\n"
            "  charvec inspect -m <model.cvec>\n");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 2; i + 1 < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

static int cmd_build(int argc, char **argv) {
    const char *input = arg_value(argc, argv, "-i");
    const char *output = arg_value(argc, argv, "-o");
    char error[256] = {0};
    CharvecModel model;

    if (!input || !output) {
        usage(stderr);
        return 2;
    }

    charvec_model_init(&model);
    if (!charvec_load_dataset(input, &model, error, sizeof(error)) ||
        !charvec_build_charset(&model, error, sizeof(error)) ||
        !charvec_encode_model(&model, error, sizeof(error)) ||
        !charvec_save_model(output, &model, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        charvec_model_free(&model);
        return 1;
    }

    printf("wrote %s\n", output);
    printf("pairs: %zu\n", model.pair_count);
    printf("max_len: %zu\n", model.max_text_len);
    printf("charset_size: %zu\n", model.charset_size);
    charvec_model_free(&model);
    return 0;
}

static int cmd_ask(int argc, char **argv) {
    const char *model_path = arg_value(argc, argv, "-m");
    const char *query = arg_value(argc, argv, "-q");
    char error[256] = {0};
    CharvecModel model;
    CharvecMatch match;

    if (!model_path || !query) {
        usage(stderr);
        return 2;
    }

    charvec_model_init(&model);
    if (!charvec_load_model(model_path, &model, error, sizeof(error)) ||
        !charvec_find_best(&model, query, &match, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        charvec_model_free(&model);
        return 1;
    }

    printf("score: %.4f\n", match.score);
    printf("matched_question: %s\n", model.pairs[match.index].question);
    printf("answer: %s\n", model.pairs[match.index].answer);

    charvec_model_free(&model);
    return 0;
}

static int cmd_inspect(int argc, char **argv) {
    const char *model_path = arg_value(argc, argv, "-m");
    char error[256] = {0};
    CharvecModel model;

    if (!model_path) {
        usage(stderr);
        return 2;
    }

    charvec_model_init(&model);
    if (!charvec_load_model(model_path, &model, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        charvec_model_free(&model);
        return 1;
    }

    printf("model: %s\n", model_path);
    printf("pairs: %zu\n", model.pair_count);
    printf("max_len: %zu\n", model.max_text_len);
    printf("charset_size: %zu\n", model.charset_size);

    charvec_model_free(&model);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }

    if (strcmp(argv[1], "build") == 0) {
        return cmd_build(argc, argv);
    }
    if (strcmp(argv[1], "ask") == 0) {
        return cmd_ask(argc, argv);
    }
    if (strcmp(argv[1], "inspect") == 0) {
        return cmd_inspect(argc, argv);
    }

    usage(stderr);
    return 2;
}

