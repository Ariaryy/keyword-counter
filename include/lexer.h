#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#include "types.h"

void tokenize(FILE* f, MapEntry** keyword_map, MapEntry** identifier_map);

#endif