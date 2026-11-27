/*
 * dfa.c - Function to convert ε-NFA to DFA using subset construction, and to minimise that DFA
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
        if (state->n != n)
            continue; // Sets of different lengths can't be the same
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
void free_dfa(DFA *dfa)
{
    if (dfa == NULL)
        return;

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
    if (nfa == NULL)
        return NULL;

    DFA *dfa = (DFA *)malloc(sizeof(DFA));
    dfa->count = 0;
    dfa->n_final = 0;
    dfa->n_symbols = get_alphabet(nfa, dfa->alphabet);

    Node *moved[MAX_STATES];                          // move(S1, a)
    Node *closure[MAX_STATES];                        // ε-closure(move(S1, a))
    int n = epsilon_closure(&nfa->start, 1, closure); // The start state of the DFA is ε-closure({s0})
    qsort(closure, n, sizeof(Node *), compare_ids);   // Sorted, so sets can be compared element by element
    dfa->states[dfa->count++] = create_state(closure, n, 0, dfa->n_symbols);
    dfa->start = 0;

    // States are added in order, so every state before i is marked and every state from i on is unmarked
    for (int i = 0; i < dfa->count; i++)
    {
        State *s1 = dfa->states[i];              // Mark S1
        for (int k = 0; k < dfa->n_symbols; k++) // For each input symbol a
        {
            int m = move(s1->set, s1->n, dfa->alphabet[k], moved);
            if (m == 0)
                continue; // No edge on this symbol
            n = epsilon_closure(moved, m, closure);
            qsort(closure, n, sizeof(Node *), compare_ids);
            int id = find_state(dfa, closure, n);
            if (id == -1) // S2 is not in DS: add it as an unmarked state
            {
                // DFA states capped at MAX_STATES as subset construction can blow up exponentially
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

// Function to check whether a DFA state is final
int is_final(DFA *dfa, int id)
{
    for (int i = 0; i < dfa->n_final; i++)
    {
        if (dfa->final[i] == id)
        {
            return 1;
        }
    }
    return 0;
}

// Minimise a DFA by partition refinement: start with the groups {final, non-final} and keep splitting
// until no group splits. Returns the new minimal DFA, or NULL if the DFA is already minimal
DFA *minimise_dfa(DFA *dfa)
{
    if (dfa == NULL)
        return NULL;

    int group[MAX_STATES];     // group[i] is the group of DFA state i
    int new_group[MAX_STATES]; // Groups after one more round of splitting
    for (int i = 0; i < dfa->count; i++)
    {
        group[i] = is_final(dfa, i); // Final states start in group 1, the rest in group 0
    }
    int n_groups = (dfa->n_final > 0) + (dfa->n_final < dfa->count);

    // A missing transition (-1) stays -1, so it acts as a dead state of its own. Every Thompson NFA state
    // can reach the accept state, so no real DFA state is equivalent to that dead state
    int changed = 1;
    while (changed)
    {
        int new_count = 0;
        for (int i = 0; i < dfa->count; i++)
        {
            new_group[i] = -1;
            for (int j = 0; j < i; j++) // Join the first earlier state that is still equivalent to i
            {
                if (group[j] != group[i])
                    continue; // Already split apart
                int same = 1;
                for (int k = 0; k < dfa->n_symbols; k++) // Every symbol must lead into the same group
                {
                    int ti = dfa->states[i]->to[k], tj = dfa->states[j]->to[k];
                    if ((ti == -1 ? -1 : group[ti]) != (tj == -1 ? -1 : group[tj]))
                    {
                        same = 0;
                        break;
                    }
                }
                if (same)
                {
                    new_group[i] = new_group[j];
                    break;
                }
            }
            if (new_group[i] == -1)
            {
                new_group[i] = new_count++; // No equivalent earlier state: start a new group
            }
        }
        changed = (new_count != n_groups); // Groups only ever split, so the same count means the same groups
        n_groups = new_count;
        for (int i = 0; i < dfa->count; i++)
        {
            group[i] = new_group[i];
        }
    }

    if (n_groups == dfa->count)
    {
        fprintf(stdout, "DFA is already minimal.\n");
        return NULL;
    }

    DFA *min = (DFA *)malloc(sizeof(DFA));
    min->count = 0;
    min->n_final = 0;
    min->n_symbols = dfa->n_symbols;
    for (int k = 0; k < dfa->n_symbols; k++)
    {
        min->alphabet[k] = dfa->alphabet[k];
    }
    min->start = group[dfa->start]; // Groups are numbered in order of first appearance, so this is 0

    // Each group becomes one state of the minimal DFA
    Node *set[MAX_STATES]; // Union of the NFA sets of the states in one group
    for (int g = 0; g < n_groups; g++)
    {
        int n = 0;
        int rep = -1; // First state of the group; every state in it has the same transitions
        for (int i = 0; i < dfa->count; i++)
        {
            if (group[i] != g)
                continue;
            if (rep == -1)
                rep = i;
            State *state = dfa->states[i];
            for (int j = 0; j < state->n; j++)
            {
                // Add the NFA state to the union if it's not already there
                int found = 0;
                for (int k = 0; k < n; k++)
                {
                    if (set[k] == state->set[j])
                    {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    set[n++] = state->set[j];
                }
            }
        }
        qsort(set, n, sizeof(Node *), compare_ids);
        State *state = create_state(set, n, g, min->n_symbols);
        for (int k = 0; k < min->n_symbols; k++)
        {
            int t = dfa->states[rep]->to[k];
            state->to[k] = (t == -1) ? -1 : group[t];
        }
        min->states[min->count++] = state;
        if (is_final(dfa, rep))
        {
            min->final[min->n_final++] = g;
        }
    }

    return min;
}

// Function to run a string through a DFA; returns 1 if the DFA accepts it, 0 if not
int is_accepted(DFA *dfa, const char *str)
{
    if (dfa == NULL || str == NULL)
        return 0;

    int current = dfa->start;
    for (const char *p = str; *p != '\0'; p++) // p is pointer to current char in str
    {
        int k = 0;
        while (k < dfa->n_symbols && dfa->alphabet[k] != *p)
            k++; // Find the index of the symbol in the alphabet
        if (k == dfa->n_symbols || dfa->states[current]->to[k] == -1)
            return 0; // Symbol not in Σ, or no transition on it: the string is rejected
        current = dfa->states[current]->to[k];
    }
    return is_final(dfa, current); // Accepted only if the string ends in a final state
}
