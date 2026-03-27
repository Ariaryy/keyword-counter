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

static const char* KEYWORDS[] = {"int", "float", "char"};
static const size_t NUM_KEYWORDS = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

void process_word(char* word, MapEntry** keyword_map,
                  MapEntry** identifier_map) {
    for (size_t i = 0; i < NUM_KEYWORDS; i++) {
        if (!strcmp(word, KEYWORDS[i])) {
            MAP_INC(*keyword_map, word);
            return;
        }
    }
    MAP_INC(*identifier_map, word);
}

void tokenize(FILE* f, MapEntry** keyword_map, MapEntry** identifier_map) {
    int c;
    char word[MAX_WORD_LEN];
    int wi = 0;

    while ((c = fgetc(f)) != EOF) {
        if (isalpha(c) || c == '_') {
            if (wi < MAX_WORD_LEN - 1) word[wi++] = c;
        } else {
            if (wi > 0) {
                word[wi] = '\0';
                process_word(word, keyword_map, identifier_map);
                wi = 0;
            }
        }
    }

    if (wi > 0) {
        word[wi] = '\0';
        process_word(word, keyword_map, identifier_map);
    }
}