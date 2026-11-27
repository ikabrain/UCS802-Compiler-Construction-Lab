#ifndef PARSER_H
#define PARSER_H

#include "table.h"

#define MAX_STACK (4 * MAX_INPUT) // States and grammar symbols on the parse stack


int parse(Grammar *g, Table *t, const char *str);

#endif
