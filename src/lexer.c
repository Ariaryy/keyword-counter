#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_ds.h"
#include "types.h"

#define MAP_INC(map, key)               \
    do {                                \
        int idx = shgeti(map, key);     \
        if (idx != -1) {                \
            (map)[idx].value++;         \
        } else {                        \
            shput(map, strdup(key), 1); \
        }                               \
    } while (0)

#define MAX_WORD_LEN 100
#define MAX_LINE_TOKENS 16

static const char* KEYWORDS[] = {"int", "float", "char"};
static const size_t NUM_KEYWORDS = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

typedef enum {
    TOKEN_TYPE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_ASSIGN,
    TOKEN_OPERATOR,
    TOKEN_SEMICOLON
} LineTokenKind;

static int is_keyword(const char* word) {
    for (size_t i = 0; i < NUM_KEYWORDS; i++) {
        if (!strcmp(word, KEYWORDS[i])) return 1;
    }

    return 0;
}

static void process_word(char* word, MapEntry** keyword_map,
                         MapEntry** identifier_map) {
    if (is_keyword(word)) {
        MAP_INC(*keyword_map, word);
        return;
    }

    MAP_INC(*identifier_map, word);
}

static int is_identifier_start(int c) { return isalpha(c) || c == '_'; }

static int is_identifier_char(int c) { return isalnum(c) || c == '_'; }

static int is_token_separator(int c) {
    return c == EOF || isspace(c) || strchr(";,+-*=(){}[]<>!&|/%", c) != NULL;
}

static void report_error(int* error_count, int line, int column,
                         const char* message, const char* token) {
    (*error_count)++;

    if (token != NULL && token[0] != '\0') {
        printf("Error at line %d, column %d: %s `%s`\n", line, column, message,
               token);
        return;
    }

    printf("Error at line %d, column %d: %s\n", line, column, message);
}

static void merge_map_counts(MapEntry** destination, MapEntry* source) {
    for (int i = 0; i < shlen(source); i++) {
        for (int count = 0; count < source[i].value; count++) {
            MAP_INC(*destination, source[i].key);
        }
    }
}

static void clear_map(MapEntry** map) {
    for (int i = 0; i < shlen(*map); i++) {
        free((*map)[i].key);
    }

    shfree(*map);
    *map = NULL;
}

static void add_line_token(LineTokenKind* tokens, int* token_count,
                           LineTokenKind token) {
    if (*token_count < MAX_LINE_TOKENS) {
        tokens[*token_count] = token;
        (*token_count)++;
    }
}

static int is_value_token(LineTokenKind token) {
    return token == TOKEN_IDENTIFIER || token == TOKEN_NUMBER;
}

static int validate_line_tokens(const LineTokenKind* tokens, int token_count) {
    if (token_count == 0) return 1;

    if (token_count == 3 && tokens[0] == TOKEN_TYPE &&
        tokens[1] == TOKEN_IDENTIFIER && tokens[2] == TOKEN_SEMICOLON) {
        return 1;
    }

    if (token_count == 4 && tokens[0] == TOKEN_IDENTIFIER &&
        tokens[1] == TOKEN_ASSIGN && is_value_token(tokens[2]) &&
        tokens[3] == TOKEN_SEMICOLON) {
        return 1;
    }

    if (token_count == 6 && tokens[0] == TOKEN_IDENTIFIER &&
        tokens[1] == TOKEN_ASSIGN && is_value_token(tokens[2]) &&
        tokens[3] == TOKEN_OPERATOR && is_value_token(tokens[4]) &&
        tokens[5] == TOKEN_SEMICOLON) {
        return 1;
    }

    return 0;
}

static void finalize_line(LineTokenKind* tokens, int* token_count,
                          int* error_count, int line, int* line_has_error,
                          MapEntry** keyword_map, MapEntry** identifier_map,
                          MapEntry** line_keyword_map,
                          MapEntry** line_identifier_map) {
    int is_valid_line = validate_line_tokens(tokens, *token_count);

    if (!*line_has_error && !is_valid_line) {
        report_error(error_count, line, 1, "invalid statement", NULL);
    }

    if (!*line_has_error && is_valid_line) {
        merge_map_counts(keyword_map, *line_keyword_map);
        merge_map_counts(identifier_map, *line_identifier_map);
    }

    clear_map(line_keyword_map);
    clear_map(line_identifier_map);
    *token_count = 0;
    *line_has_error = 0;
}

static void skip_single_line_comment(FILE* f, int* line, int* column) {
    int c;

    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            (*line)++;
            *column = 1;
            return;
        }

        (*column)++;
    }
}

static void skip_multi_line_comment(FILE* f, int* line, int* column,
                                    int* error_count) {
    int c;
    int prev = 0;

    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            (*line)++;
            *column = 1;
        } else {
            (*column)++;
        }

        if (prev == '*' && c == '/') return;
        prev = c;
    }

    report_error(error_count, *line, *column, "unterminated block comment",
                 NULL);
}

static void read_identifier_token(FILE* f, int* c, int* column, char* token,
                                  int* invalid_identifier) {
    int len = 0;

    do {
        if (len < MAX_WORD_LEN - 1) token[len++] = (char)*c;
        *c = fgetc(f);
        (*column)++;

        if (*c != EOF && !is_identifier_char(*c) && !is_token_separator(*c)) {
            *invalid_identifier = 1;
        }
    } while (*c != EOF && !is_token_separator(*c));

    while (*invalid_identifier && *c != EOF && !is_token_separator(*c)) {
        if (len < MAX_WORD_LEN - 1) token[len++] = (char)*c;
        *c = fgetc(f);
        (*column)++;
    }

    token[len] = '\0';
}

static void read_number_token(FILE* f, int* c, int* column, char* token,
                              int* has_identifier_part) {
    int len = 0;

    do {
        if (len < MAX_WORD_LEN - 1) token[len++] = (char)*c;
        *c = fgetc(f);
        (*column)++;

        if (*c != EOF && (isalpha(*c) || *c == '_')) {
            *has_identifier_part = 1;
        }
    } while (*c != EOF && !is_token_separator(*c));

    while (*has_identifier_part && *c != EOF && !is_token_separator(*c)) {
        if (len < MAX_WORD_LEN - 1) token[len++] = (char)*c;
        *c = fgetc(f);
        (*column)++;
    }

    token[len] = '\0';
}

static void handle_separator(int c, LineTokenKind* line_tokens,
                             int* line_token_count, int* error_count, int* line,
                             int* column, int* line_has_error,
                             int* expect_identifier_after_type,
                             MapEntry** keyword_map, MapEntry** identifier_map,
                             MapEntry** line_keyword_map,
                             MapEntry** line_identifier_map) {
    if (c == EOF) return;

    if (c == '\n') {
        finalize_line(line_tokens, line_token_count, error_count, *line,
                      line_has_error, keyword_map, identifier_map,
                      line_keyword_map, line_identifier_map);
        (*line)++;
        *column = 1;
        return;
    }

    if (c == ';') {
        if (*expect_identifier_after_type) {
            report_error(error_count, *line, *column - 1,
                         "expected identifier after type keyword", NULL);
            *expect_identifier_after_type = 0;
            *line_has_error = 1;
        }
        add_line_token(line_tokens, line_token_count, TOKEN_SEMICOLON);
        (*column)++;
        return;
    }

    if (c == '=') {
        add_line_token(line_tokens, line_token_count, TOKEN_ASSIGN);
        (*column)++;
        return;
    }

    if (strchr("+-*/%", c) != NULL) {
        if (*expect_identifier_after_type) {
            report_error(error_count, *line, *column - 1,
                         "expected identifier after type keyword", NULL);
            *expect_identifier_after_type = 0;
            *line_has_error = 1;
        }
        add_line_token(line_tokens, line_token_count, TOKEN_OPERATOR);
        (*column)++;
        return;
    }

    (*column)++;
}

int tokenize(FILE* f, MapEntry** keyword_map, MapEntry** identifier_map) {
    int c;
    int line = 1;
    int column = 1;
    int error_count = 0;
    int expect_identifier_after_type = 0;
    int line_has_error = 0;
    LineTokenKind line_tokens[MAX_LINE_TOKENS];
    int line_token_count = 0;
    MapEntry* line_keyword_map = NULL;
    MapEntry* line_identifier_map = NULL;

    while ((c = fgetc(f)) != EOF) {
        int token_line = line;
        int token_column = column;

        if (c == '\n') {
            finalize_line(line_tokens, &line_token_count, &error_count, line,
                          &line_has_error, keyword_map, identifier_map,
                          &line_keyword_map, &line_identifier_map);
            line++;
            column = 1;
            continue;
        }

        if (isspace(c)) {
            column++;
            continue;
        }

        if (c == '/') {
            int next = fgetc(f);

            if (next == '/') {
                finalize_line(line_tokens, &line_token_count, &error_count,
                              line, &line_has_error, keyword_map,
                              identifier_map, &line_keyword_map,
                              &line_identifier_map);
                skip_single_line_comment(f, &line, &column);
                expect_identifier_after_type = 0;
                continue;
            }

            if (next == '*') {
                skip_multi_line_comment(f, &line, &column, &error_count);
                continue;
            }

            if (next != EOF) ungetc(next, f);

            if (expect_identifier_after_type) {
                report_error(&error_count, token_line, token_column,
                             "expected identifier after type keyword", NULL);
                expect_identifier_after_type = 0;
                line_has_error = 1;
            }

            add_line_token(line_tokens, &line_token_count, TOKEN_OPERATOR);
            column++;
            continue;
        }

        if (is_identifier_start(c)) {
            char token[MAX_WORD_LEN];
            int invalid_identifier = 0;

            read_identifier_token(f, &c, &column, token, &invalid_identifier);

            if (invalid_identifier) {
                report_error(&error_count, token_line, token_column,
                             "invalid identifier", token);
                expect_identifier_after_type = 0;
                line_has_error = 1;
            } else {
                int token_is_keyword = is_keyword(token);

                process_word(token, &line_keyword_map, &line_identifier_map);

                if (expect_identifier_after_type) {
                    if (token_is_keyword) {
                        report_error(&error_count, token_line, token_column,
                                     "expected identifier after type keyword, "
                                     "found keyword",
                                     token);
                        line_has_error = 1;
                    }
                    expect_identifier_after_type = 0;
                } else if (token_is_keyword) {
                    expect_identifier_after_type = 1;
                }

                add_line_token(
                    line_tokens, &line_token_count,
                    token_is_keyword ? TOKEN_TYPE : TOKEN_IDENTIFIER);
            }

            handle_separator(
                c, line_tokens, &line_token_count, &error_count, &line, &column,
                &line_has_error, &expect_identifier_after_type, keyword_map,
                identifier_map, &line_keyword_map, &line_identifier_map);
            continue;
        }

        if (isdigit(c)) {
            char token[MAX_WORD_LEN];
            int has_identifier_part = 0;

            read_number_token(f, &c, &column, token, &has_identifier_part);

            if (has_identifier_part) {
                report_error(&error_count, token_line, token_column,
                             "invalid identifier", token);
                line_has_error = 1;
            } else if (expect_identifier_after_type) {
                report_error(&error_count, token_line, token_column,
                             "expected identifier after type keyword", token);
                line_has_error = 1;
            } else {
                add_line_token(line_tokens, &line_token_count, TOKEN_NUMBER);
            }

            expect_identifier_after_type = 0;

            handle_separator(
                c, line_tokens, &line_token_count, &error_count, &line, &column,
                &line_has_error, &expect_identifier_after_type, keyword_map,
                identifier_map, &line_keyword_map, &line_identifier_map);
            continue;
        }

        if (c == ';') {
            if (expect_identifier_after_type) {
                report_error(&error_count, token_line, token_column,
                             "expected identifier after type keyword", NULL);
                expect_identifier_after_type = 0;
                line_has_error = 1;
            }
            add_line_token(line_tokens, &line_token_count, TOKEN_SEMICOLON);
            column++;
            continue;
        }

        if (c == '=') {
            add_line_token(line_tokens, &line_token_count, TOKEN_ASSIGN);
            column++;
            continue;
        }

        if (strchr("+-*%", c) != NULL) {
            add_line_token(line_tokens, &line_token_count, TOKEN_OPERATOR);
            column++;
            continue;
        }

        if (!strchr("(){}[],<>!&|", c)) {
            char token[2] = {(char)c, '\0'};
            report_error(&error_count, token_line, token_column,
                         "invalid character", token);
            expect_identifier_after_type = 0;
            line_has_error = 1;
        }

        column++;
    }

    finalize_line(line_tokens, &line_token_count, &error_count, line,
                  &line_has_error, keyword_map, identifier_map,
                  &line_keyword_map, &line_identifier_map);

    return error_count;
}
