/*
 * dfa.c - Function to convert ε-NFA to DFA using subset construction
 */
#include <stdio.h>
#include <stdlib.h>

#include "dfa.h"


// Function to compare two NFA states by id, for qsort
int compare_ids(const void *a, const void *b)
{
    return (*(Node **)a)->id - (*(Node **)b)->id;
}

// Function to find move(T, sym): every state reachable from a state in T by one edge labelled sym.
// Fills result[] (caller gives MAX_STATES slots) and returns the number of states in it.
int move(Node **T, int t_count, char sym, Node **result)
{
    int count = 0;
    for (int i = 0; i < t_count; i++)
    {
        for (int j = 0; j < T[i]->n; j++)
        {
            if (T[i]->sym[j] == sym)
            {
                result[count++] = T[i]->to[j];
            }
        }
    }
    return count;
}

// Function to create a new DFA state from a set of NFA states (already sorted by id)
State *create_state(Node **set, int n, int id, int n_symbols)
{
    State *state = (State *)malloc(sizeof(State));
    for (int i = 0; i < n; i++)
    {
        state->set[i] = set[i];
    }
    for (int k = 0; k < n_symbols; k++)
    {
        state->to[k] = -1;
    }
    state->n = n;
    state->id = id;
    return state;
}

// Function to find the DFA state with the same sorted set of NFA states; returns its id, or -1 if it's new
int find_state(DFA *dfa, Node **set, int n)
{
    for (int i = 0; i < dfa->count; i++)
    {
        State *state = dfa->states[i];
        if (state->n != n) continue; // Sets of different lengths can't be the same
        int same = 1;
        for (int j = 0; j < n; j++)
        {
            if (state->set[j] != set[j])
            {
                same = 0;
                break;
            }
        }
        if (same)
        {
            return state->id;
        }
    }
    return -1;
}

// Function to free the memory allocated for a DFA with all its states; the NFA states in the sets are not freed
void free_dfa(DFA *dfa) {
    if (dfa == NULL) return;

    for (int i = 0; i < dfa->count; i++)
    {
        free(dfa->states[i]);
    }
    free(dfa); // Free the DFA structure itself
}

// Function to print the DFA states as State X = {NFA states} and transitions as State X --symbol--> State Y
void print_dfa(DFA *dfa)
{
    if (dfa == NULL)
    {
        printf("DFA is NULL\n");
        return;
    }
    for (int i = 0; i < dfa->count; i++)
    {
        State *state = dfa->states[i];
        printf("State %d = {", state->id);
        for (int j = 0; j < state->n; j++)
        {
            printf(j == 0 ? "%d" : ", %d", state->set[j]->id);
        }
        printf("}\n");
    }
    printf("\n");

    for (int i = 0; i < dfa->count; i++)
    {
        State *state = dfa->states[i];
        for (int k = 0; k < dfa->n_symbols; k++)
        {
            if (state->to[k] != -1)
            {
                printf("State %d --%c--> State %d\n", state->id, dfa->alphabet[k], state->to[k]);
            }
        }
    }
    printf("\n");

    // Print the start state and the final states
    printf("Start State: %d\n", dfa->start);
    printf("Final States: {");
    for (int i = 0; i < dfa->n_final; i++)
    {
        printf(i == 0 ? "%d" : ", %d", dfa->final[i]);
    }
    printf("}\n\n");
}

// Convert an ε-NFA to a DFA using subset construction
DFA *nfa_to_dfa(NFA *nfa)
{
    if (nfa == NULL) return NULL;

    DFA *dfa = (DFA *)malloc(sizeof(DFA));
    dfa->count = 0;
    dfa->n_final = 0;
    dfa->n_symbols = get_alphabet(nfa, dfa->alphabet);

    Node *moved[MAX_STATES];   // move(S1, a)
    Node *closure[MAX_STATES]; // ε-closure(move(S1, a))
    int n = epsilon_closure(&nfa->start, 1, closure); // The start state of the DFA is ε-closure({s0})
    qsort(closure, n, sizeof(Node *), compare_ids);   // Sorted, so sets can be compared element by element
    dfa->states[dfa->count++] = create_state(closure, n, 0, dfa->n_symbols);
    dfa->start = 0;

    // States are added in order, so every state before i is marked and every state from i on is unmarked
    for (int i = 0; i < dfa->count; i++)
    {
        State *s1 = dfa->states[i]; // Mark S1
        for (int k = 0; k < dfa->n_symbols; k++) // For each input symbol a
        {
            int m = move(s1->set, s1->n, dfa->alphabet[k], moved);
            if (m == 0) continue; // No edge on this symbol
            n = epsilon_closure(moved, m, closure);
            qsort(closure, n, sizeof(Node *), compare_ids);
            int id = find_state(dfa, closure, n);
            if (id == -1) // S2 is not in DS: add it as an unmarked state
            {
                // ponytail: DFA states capped at MAX_STATES; subset construction can blow up exponentially
                if (dfa->count >= MAX_STATES)
                {
                    fprintf(stderr, "Error: Cannot create more than %d DFA states.\n", MAX_STATES);
                    return dfa;
                }
                id = dfa->count;
                dfa->states[dfa->count++] = create_state(closure, n, id, dfa->n_symbols);
            }
            s1->to[k] = id; // transfunc[S1, a] = S2
        }
    }

    // A DFA state is final if its set holds the NFA's accept state
    for (int i = 0; i < dfa->count; i++)
    {
        State *state = dfa->states[i];
        for (int j = 0; j < state->n; j++)
        {
            if (state->set[j] == nfa->end)
            {
                dfa->final[dfa->n_final++] = state->id;
                break;
            }
        }
    }

    return dfa;
}
