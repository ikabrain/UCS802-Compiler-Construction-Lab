#ifndef DFA_H
#define DFA_H

#include "nfa.h"

// Structure to represent one state of DFA; it stands for a set of NFA states
typedef struct State
{
    Node *set[MAX_STATES]; // NFA states in this DFA state, sorted by id
    int to[MAX_SYMBOLS];   // to[k] is the id of the state reached on alphabet[k], -1 if none
    int n, id;             // n is the number of NFA states in set, id is the state id
} State;

typedef struct DFA
{
    State *states[MAX_STATES];  // All DFA states
    int count;                  // Number of DFA states
    int start;                  // Id of the start state, ε-closure({s0})
    int final[MAX_STATES];      // Ids of the final states, those whose set holds the NFA's accept state
    int n_final;                // Number of final states
    char alphabet[MAX_SYMBOLS]; // Input symbols, from get_alphabet
    int n_symbols;              // Number of input symbols
} DFA;


void free_dfa(DFA *dfa);
void print_dfa(DFA *dfa);
DFA *nfa_to_dfa(NFA *nfa);

#endif
