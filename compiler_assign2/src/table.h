#ifndef TABLE_H
#define TABLE_H

#include "items.h"

// Structure to represent one ACTION entry: sN shifts to state N, rN reduces by production N, acc accepts, blank is an error
typedef struct Action
{
    char type; // 's' shift, 'r' reduce, 'a' accept, 0 blank
    int n;     // State to shift to, or production to reduce by
} Action;

typedef struct Table
{
    Action action[MAX_SETS][MAX_SYMBOLS]; // action[state][a] for terminals a and $
    int go[MAX_SETS][MAX_SYMBOLS];        // go[state][A] for non-terminals A, -1 if blank
    int n_states;                         // Number of states, one per item set
    int conflicts;                        // Number of conflicting entries; nonzero means the grammar is not SLR(1)
} Table;


void print_table(Grammar *g, Table *t);
Table *build_table(Grammar *g, Collection *c);

#endif
