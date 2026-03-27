#include <stdio.h>

#include "stb_ds.h"
#include "types.h"

void print_map(MapEntry* map) {
    for (int i = 0; i < shlen(map); i++) {
        printf("%s: %i\n", map[i].key, map[i].value);
    }
}