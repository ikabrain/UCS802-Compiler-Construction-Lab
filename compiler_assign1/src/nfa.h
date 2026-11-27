#ifndef NFA_H
#define NFA_H

#include "regex.h"

// Each symbol, '|' and '*' adds 2 states, so a regex of n characters needs at most 2n states
#define MAX_STATES (2 * MAX_REGEX_LENGTH)
#define MAX_SYMBOLS 52 // Allowed symbols are a-z, A-Z

// Structure to represent one state of NFA; at most 2 outgoing edges; sym 0 means epsilon.
typedef struct Node
{
    char sym[2];        // Symbols on edges
    struct Node *to[2]; // Where the edges go to
    int n, id;          // n is the number of actual outgoing edges, id is the state id
} Node;

typedef struct NFA
{
    Node *start; // Start state of the NFA
    Node *end;   // Accept state of the NFA
} NFA;


void free_nfa(NFA *nfa);
void print_nfa(NFA *nfa);
NFA *postfix_to_nfa(const char *postfix);
int epsilon_closure(Node **T, int t_count, Node **closure);
int get_alphabet(NFA *nfa, char *alphabet);

#endif