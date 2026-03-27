#define STB_DS_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "stb_ds.h"
#include "types.h"
#include "utils.h"

MapEntry* keyword_map = NULL;
MapEntry* identifier_map = NULL;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE* fptr = fopen(argv[1], "r");

    if (fptr == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    sh_new_strdup(keyword_map);
    sh_new_strdup(identifier_map);

    tokenize(fptr, &keyword_map, &identifier_map);

    printf("\nKeywords Count:\n");
    print_map(keyword_map);

    printf("\nIdentifiers Count:\n");
    print_map(identifier_map);

    return 0;
}