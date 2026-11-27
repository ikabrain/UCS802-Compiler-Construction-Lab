#ifndef GRAMMAR_H
#define GRAMMAR_H

#define MAX_SYMBOLS 64     // Terminals, non-terminals, $ and the augmented start symbol
#define MAX_NAME 16        // Longest symbol name plus '\0', e.g. "id" or "E'"
#define MAX_PRODUCTIONS 64 // Productions after splitting on '|', plus S' -> S
#define MAX_RHS 32         // Symbols on the right side of one production
#define MAX_INPUT 255      // Longest production line or input string

// Structure to represent one production A -> X Y Z; n is 0 for A -> ε
typedef struct Production
{
    int lhs;          // Non-terminal on the left side
    int rhs[MAX_RHS]; // Symbols on the right side
    int n;            // Number of symbols on the right side
} Production;

typedef struct Grammar
{
    char names[MAX_SYMBOLS][MAX_NAME];    // names[X] is the name of symbol X
    int is_terminal[MAX_SYMBOLS];         // 1 for terminals and $, 0 for non-terminals
    int n_symbols;                        // Number of symbols
    Production prods[MAX_PRODUCTIONS];    // prods[0] is the augmented production S' -> S
    int n_prods;                          // Number of productions
    int start, end;                       // Ids of the augmented start symbol S' and the end marker $
    int nullable[MAX_SYMBOLS];            // nullable[X] is 1 if X derives ε
    int first[MAX_SYMBOLS][MAX_SYMBOLS];  // first[X][a] is 1 if terminal a is in FIRST(X)
    int follow[MAX_SYMBOLS][MAX_SYMBOLS]; // follow[A][a] is 1 if terminal a is in FOLLOW(A)
} Grammar;


Grammar *init_grammar(char **lines, int n);
int tokenize(Grammar *g, const char *str, int *tokens);
void print_production(Grammar *g, int i, int dot);
void print_grammar(Grammar *g);
void print_first_follow(Grammar *g);

#endif
