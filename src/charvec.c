#include "charvec.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *fmt, ...) {
    if (!error || error_size == 0) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(error, error_size, fmt, args);
    va_end(args);
}

static int checked_vector_len(size_t max_len, size_t charset_size, size_t *out) {
    if (charset_size == 0 || max_len == 0) {
        return 0;
    }
    if (max_len > ((size_t)-1) / charset_size) {
        return 0;
    }
    *out = max_len * charset_size;
    return 1;
}

static void trim_newline(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

static int is_blank(const char *line) {
    while (*line) {
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

static int read_line(FILE *fp, char **out) {
    size_t cap = 128;
    size_t len = 0;
    int ch;
    char *line = malloc(cap);

    if (!line) {
        return -1;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (len + 1 >= cap) {
            size_t next_cap = cap * 2;
            char *next = realloc(line, next_cap);
            if (!next) {
                free(line);
                return -1;
            }
            line = next;
            cap = next_cap;
        }
        line[len++] = (char)ch;
        if (ch == '\n') {
            break;
        }
    }

    if (len == 0 && ch == EOF) {
        free(line);
        return 0;
    }

    line[len] = '\0';
    trim_newline(line);
    *out = line;
    return 1;
}

static int ensure_pair_capacity(CharvecModel *model, size_t *capacity, char *error, size_t error_size) {
    if (model->pair_count < *capacity) {
        return 1;
    }

    size_t next_capacity = (*capacity == 0) ? 8 : *capacity * 2;
    CharvecPair *next = realloc(model->pairs, next_capacity * sizeof(*next));
    if (!next) {
        set_error(error, error_size, "failed to allocate pair storage");
        return 0;
    }

    memset(next + *capacity, 0, (next_capacity - *capacity) * sizeof(*next));
    model->pairs = next;
    *capacity = next_capacity;
    return 1;
}

static int append_pair(CharvecModel *model, size_t *capacity, const char *question, const char *answer, char *error, size_t error_size) {
    if (!question[0] || !answer[0]) {
        set_error(error, error_size, "records must include non-empty Q: and A: lines");
        return 0;
    }
    if (strlen(question) >= CHARVEC_MAX_TEXT || strlen(answer) >= CHARVEC_MAX_TEXT) {
        set_error(error, error_size, "record text exceeds %d bytes", CHARVEC_MAX_TEXT - 1);
        return 0;
    }
    if (!ensure_pair_capacity(model, capacity, error, error_size)) {
        return 0;
    }

    CharvecPair *pair = &model->pairs[model->pair_count++];
    strncpy(pair->question, question, sizeof(pair->question) - 1);
    strncpy(pair->answer, answer, sizeof(pair->answer) - 1);

    size_t q_len = strlen(pair->question);
    size_t a_len = strlen(pair->answer);
    if (q_len > model->max_text_len) {
        model->max_text_len = q_len;
    }
    if (a_len > model->max_text_len) {
        model->max_text_len = a_len;
    }
    return 1;
}

static int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static void write_hex(FILE *fp, const char *text) {
    const unsigned char *cursor = (const unsigned char *)text;
    while (*cursor) {
        fprintf(fp, "%02X", *cursor++);
    }
}

static int decode_hex_text(const char *hex, char *out, size_t out_size, char *error, size_t error_size) {
    size_t len = strlen(hex);
    if (len % 2 != 0) {
        set_error(error, error_size, "hex text has odd length");
        return 0;
    }
    if ((len / 2) >= out_size) {
        set_error(error, error_size, "decoded text exceeds buffer");
        return 0;
    }

    for (size_t i = 0; i < len; i += 2) {
        int high = hex_value(hex[i]);
        int low = hex_value(hex[i + 1]);
        if (high < 0 || low < 0) {
            set_error(error, error_size, "invalid hex text");
            return 0;
        }
        out[i / 2] = (char)((high << 4) | low);
    }
    out[len / 2] = '\0';
    return 1;
}

static int parse_prefixed_value(const char *line, const char *prefix, const char **value) {
    size_t prefix_len = strlen(prefix);
    if (strncmp(line, prefix, prefix_len) != 0) {
        return 0;
    }
    *value = line + prefix_len;
    return 1;
}

static int parse_size_line(const char *line, const char *prefix, size_t *out) {
    const char *value;
    char *end = NULL;
    unsigned long parsed;

    if (!parse_prefixed_value(line, prefix, &value)) {
        return 0;
    }

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        return 0;
    }
    *out = (size_t)parsed;
    return 1;
}

static int parse_charset_hex(const char *line, CharvecModel *model, char *error, size_t error_size) {
    const char *value;
    size_t count = 0;

    if (!parse_prefixed_value(line, "charset_hex ", &value)) {
        set_error(error, error_size, "missing charset_hex line");
        return 0;
    }

    while (*value) {
        while (*value == ' ') {
            value++;
        }
        if (!*value) {
            break;
        }
        if (!isxdigit((unsigned char)value[0]) || !isxdigit((unsigned char)value[1])) {
            set_error(error, error_size, "invalid charset hex byte");
            return 0;
        }
        if (count >= CHARVEC_MAX_CHARSET) {
            set_error(error, error_size, "charset exceeds %d entries", CHARVEC_MAX_CHARSET);
            return 0;
        }
        model->charset[count++] = (unsigned char)((hex_value(value[0]) << 4) | hex_value(value[1]));
        value += 2;
        if (*value && *value != ' ') {
            set_error(error, error_size, "charset bytes must be space separated");
            return 0;
        }
    }

    if (count != model->charset_size) {
        set_error(error, error_size, "charset_size does not match charset_hex");
        return 0;
    }
    return 1;
}

static int charset_index(const CharvecModel *model, unsigned char ch) {
    for (size_t i = 0; i < model->charset_size; i++) {
        if (model->charset[i] == ch) {
            return (int)i;
        }
    }
    return -1;
}

static char *encode_text(const CharvecModel *model, const char *text, char *error, size_t error_size) {
    size_t vector_len;
    if (!checked_vector_len(model->max_text_len, model->charset_size, &vector_len)) {
        set_error(error, error_size, "invalid model dimensions");
        return NULL;
    }

    char *bits = malloc(vector_len + 1);
    if (!bits) {
        set_error(error, error_size, "failed to allocate vector");
        return NULL;
    }
    memset(bits, '0', vector_len);
    bits[vector_len] = '\0';

    size_t text_len = strlen(text);
    for (size_t i = 0; i < text_len && i < model->max_text_len; i++) {
        int index = charset_index(model, (unsigned char)text[i]);
        if (index >= 0) {
            bits[(i * model->charset_size) + (size_t)index] = '1';
        }
    }
    return bits;
}

static int valid_bits(const char *bits, size_t expected_len) {
    if (strlen(bits) != expected_len) {
        return 0;
    }
    for (size_t i = 0; i < expected_len; i++) {
        if (bits[i] != '0' && bits[i] != '1') {
            return 0;
        }
    }
    return 1;
}

void charvec_model_init(CharvecModel *model) {
    memset(model, 0, sizeof(*model));
}

void charvec_model_free(CharvecModel *model) {
    if (!model) {
        return;
    }
    for (size_t i = 0; i < model->pair_count; i++) {
        free(model->pairs[i].question_bits);
        free(model->pairs[i].answer_bits);
    }
    free(model->pairs);
    charvec_model_init(model);
}

int charvec_load_dataset(const char *path, CharvecModel *model, char *error, size_t error_size) {
    FILE *fp = fopen(path, "r");
    char line[CHARVEC_MAX_TEXT + 16];
    char question[CHARVEC_MAX_TEXT] = {0};
    char answer[CHARVEC_MAX_TEXT] = {0};
    int have_question = 0;
    int have_answer = 0;
    size_t capacity = 0;

    if (!fp) {
        set_error(error, error_size, "cannot open dataset: %s", path);
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if (is_blank(line)) {
            continue;
        }
        if (strcmp(line, "---") == 0) {
            if ((have_question || have_answer) && !append_pair(model, &capacity, question, answer, error, error_size)) {
                fclose(fp);
                return 0;
            }
            question[0] = '\0';
            answer[0] = '\0';
            have_question = 0;
            have_answer = 0;
            continue;
        }
        if (strncmp(line, "Q:", 2) == 0) {
            const char *value = line + 2;
            while (*value == ' ') {
                value++;
            }
            strncpy(question, value, sizeof(question) - 1);
            question[sizeof(question) - 1] = '\0';
            have_question = 1;
            continue;
        }
        if (strncmp(line, "A:", 2) == 0) {
            const char *value = line + 2;
            while (*value == ' ') {
                value++;
            }
            strncpy(answer, value, sizeof(answer) - 1);
            answer[sizeof(answer) - 1] = '\0';
            have_answer = 1;
            continue;
        }
        fclose(fp);
        set_error(error, error_size, "unrecognized dataset line: %s", line);
        return 0;
    }

    fclose(fp);

    if (have_question || have_answer) {
        if (!append_pair(model, &capacity, question, answer, error, error_size)) {
            return 0;
        }
    }
    if (model->pair_count == 0) {
        set_error(error, error_size, "dataset contains no QA records");
        return 0;
    }
    return 1;
}

int charvec_build_charset(CharvecModel *model, char *error, size_t error_size) {
    int seen[CHARVEC_MAX_CHARSET] = {0};

    for (size_t i = 0; i < model->pair_count; i++) {
        const unsigned char *texts[] = {
            (const unsigned char *)model->pairs[i].question,
            (const unsigned char *)model->pairs[i].answer
        };
        for (size_t t = 0; t < 2; t++) {
            for (size_t j = 0; texts[t][j]; j++) {
                seen[texts[t][j]] = 1;
            }
        }
    }

    model->charset_size = 0;
    for (size_t i = 0; i < CHARVEC_MAX_CHARSET; i++) {
        if (seen[i]) {
            model->charset[model->charset_size++] = (unsigned char)i;
        }
    }

    if (model->charset_size == 0) {
        set_error(error, error_size, "cannot build an empty charset");
        return 0;
    }
    return 1;
}

int charvec_encode_model(CharvecModel *model, char *error, size_t error_size) {
    if (model->pair_count == 0 || model->charset_size == 0 || model->max_text_len == 0) {
        set_error(error, error_size, "model must have pairs, charset, and dimensions before encoding");
        return 0;
    }

    for (size_t i = 0; i < model->pair_count; i++) {
        free(model->pairs[i].question_bits);
        free(model->pairs[i].answer_bits);
        model->pairs[i].question_bits = encode_text(model, model->pairs[i].question, error, error_size);
        model->pairs[i].answer_bits = encode_text(model, model->pairs[i].answer, error, error_size);
        if (!model->pairs[i].question_bits || !model->pairs[i].answer_bits) {
            return 0;
        }
    }
    return 1;
}

int charvec_save_model(const char *path, const CharvecModel *model, char *error, size_t error_size) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        set_error(error, error_size, "cannot write model: %s", path);
        return 0;
    }

    fprintf(fp, "CVEC1\n");
    fprintf(fp, "pairs %zu\n", model->pair_count);
    fprintf(fp, "max_len %zu\n", model->max_text_len);
    fprintf(fp, "charset_size %zu\n", model->charset_size);
    fprintf(fp, "charset_hex");
    for (size_t i = 0; i < model->charset_size; i++) {
        fprintf(fp, " %02X", model->charset[i]);
    }
    fprintf(fp, "\n---\n");

    for (size_t i = 0; i < model->pair_count; i++) {
        fprintf(fp, "q_text_hex ");
        write_hex(fp, model->pairs[i].question);
        fprintf(fp, "\n");
        fprintf(fp, "a_text_hex ");
        write_hex(fp, model->pairs[i].answer);
        fprintf(fp, "\n");
        fprintf(fp, "q_bits %s\n", model->pairs[i].question_bits);
        fprintf(fp, "a_bits %s\n", model->pairs[i].answer_bits);
        fprintf(fp, "---\n");
    }

    fclose(fp);
    return 1;
}

int charvec_load_model(const char *path, CharvecModel *model, char *error, size_t error_size) {
    FILE *fp = fopen(path, "r");
    char *line = NULL;
    size_t expected_vector_len = 0;

    if (!fp) {
        set_error(error, error_size, "cannot open model: %s", path);
        return 0;
    }

    if (read_line(fp, &line) <= 0 || strcmp(line, "CVEC1") != 0) {
        free(line);
        fclose(fp);
        set_error(error, error_size, "model is missing CVEC1 header");
        return 0;
    }
    free(line);

    if (read_line(fp, &line) <= 0 || !parse_size_line(line, "pairs ", &model->pair_count)) {
        free(line);
        fclose(fp);
        set_error(error, error_size, "invalid pairs line");
        return 0;
    }
    free(line);

    if (read_line(fp, &line) <= 0 || !parse_size_line(line, "max_len ", &model->max_text_len)) {
        free(line);
        fclose(fp);
        set_error(error, error_size, "invalid max_len line");
        return 0;
    }
    free(line);

    if (read_line(fp, &line) <= 0 || !parse_size_line(line, "charset_size ", &model->charset_size) || model->charset_size > CHARVEC_MAX_CHARSET) {
        free(line);
        fclose(fp);
        set_error(error, error_size, "invalid charset_size line");
        return 0;
    }
    free(line);

    if (read_line(fp, &line) <= 0 || !parse_charset_hex(line, model, error, error_size)) {
        free(line);
        fclose(fp);
        return 0;
    }
    free(line);

    if (read_line(fp, &line) <= 0 || strcmp(line, "---") != 0) {
        free(line);
        fclose(fp);
        set_error(error, error_size, "missing header separator");
        return 0;
    }
    free(line);

    if (!checked_vector_len(model->max_text_len, model->charset_size, &expected_vector_len)) {
        fclose(fp);
        set_error(error, error_size, "invalid vector dimensions");
        return 0;
    }

    if (model->pair_count == 0) {
        fclose(fp);
        set_error(error, error_size, "model contains no pairs");
        return 0;
    }

    model->pairs = calloc(model->pair_count, sizeof(*model->pairs));
    if (!model->pairs) {
        fclose(fp);
        set_error(error, error_size, "failed to allocate model pairs");
        return 0;
    }

    for (size_t i = 0; i < model->pair_count; i++) {
        const char *value;

        if (read_line(fp, &line) <= 0 || !parse_prefixed_value(line, "q_text_hex ", &value) ||
            !decode_hex_text(value, model->pairs[i].question, sizeof(model->pairs[i].question), error, error_size)) {
            free(line);
            fclose(fp);
            return 0;
        }
        free(line);

        if (read_line(fp, &line) <= 0 || !parse_prefixed_value(line, "a_text_hex ", &value) ||
            !decode_hex_text(value, model->pairs[i].answer, sizeof(model->pairs[i].answer), error, error_size)) {
            free(line);
            fclose(fp);
            return 0;
        }
        free(line);

        if (read_line(fp, &line) <= 0 || !parse_prefixed_value(line, "q_bits ", &value) || !valid_bits(value, expected_vector_len)) {
            free(line);
            fclose(fp);
            set_error(error, error_size, "invalid q_bits record");
            return 0;
        }
        model->pairs[i].question_bits = malloc(expected_vector_len + 1);
        if (!model->pairs[i].question_bits) {
            free(line);
            fclose(fp);
            set_error(error, error_size, "failed to allocate q_bits");
            return 0;
        }
        memcpy(model->pairs[i].question_bits, value, expected_vector_len + 1);
        free(line);

        if (read_line(fp, &line) <= 0 || !parse_prefixed_value(line, "a_bits ", &value) || !valid_bits(value, expected_vector_len)) {
            free(line);
            fclose(fp);
            set_error(error, error_size, "invalid a_bits record");
            return 0;
        }
        model->pairs[i].answer_bits = malloc(expected_vector_len + 1);
        if (!model->pairs[i].answer_bits) {
            free(line);
            fclose(fp);
            set_error(error, error_size, "failed to allocate a_bits");
            return 0;
        }
        memcpy(model->pairs[i].answer_bits, value, expected_vector_len + 1);
        free(line);

        if (read_line(fp, &line) <= 0 || strcmp(line, "---") != 0) {
            free(line);
            fclose(fp);
            set_error(error, error_size, "missing record separator");
            return 0;
        }
        free(line);
    }

    fclose(fp);
    return 1;
}

int charvec_find_best(const CharvecModel *model, const char *query, CharvecMatch *match, char *error, size_t error_size) {
    size_t vector_len;
    char *query_bits;

    if (!checked_vector_len(model->max_text_len, model->charset_size, &vector_len)) {
        set_error(error, error_size, "invalid model dimensions");
        return 0;
    }

    query_bits = encode_text(model, query, error, error_size);
    if (!query_bits) {
        return 0;
    }

    match->index = 0;
    match->score = -1.0;

    for (size_t i = 0; i < model->pair_count; i++) {
        size_t equal = 0;
        for (size_t j = 0; j < vector_len; j++) {
            if (query_bits[j] == model->pairs[i].question_bits[j]) {
                equal++;
            }
        }
        double score = (double)equal / (double)vector_len;
        if (score > match->score) {
            match->index = i;
            match->score = score;
        }
    }

    free(query_bits);
    return 1;
}

