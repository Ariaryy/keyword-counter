#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Incorrect Usage.\n");
        exit(1);
    }

    FILE* fptr = fopen(argv[1], "r");

    if (fptr == NULL) {
        printf("There was an error opening the file.");
        exit(1);
    }

    int c;

    while ((c = fgetc(fptr)) != EOF) {
        printf("%c\n", c);
    }

    return 0;
}