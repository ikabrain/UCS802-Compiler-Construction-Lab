/*
 * nfa.c - Function to convert regular expression to ε-NFA
 */
#include <stdio.h>
#include <stdlib.h>

#include "nfa.h"

// Function to create a new NFA state
Node *create_node(int id)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->n = 0;
    node->id = id;
    return node;
}

// Function to create a new NFA
NFA *create_nfa(Node *start, Node *end)
{
    NFA *nfa = (NFA *)malloc(sizeof(NFA));
    nfa->start = start;
    nfa->end = end;
    return nfa;
}

// Function to free the memory allocated for an NFA with all its states
void free_nfa(NFA *nfa)
{
    if (nfa == NULL)
        return;

    // Collect every reachable state first (same traversal as print_nfa), so shared states and
    // cycles are seen exactly once, then free each one. Freeing during the walk would revisit freed nodes.
    Node *states[MAX_STATES];
    int count = 0;
    states[count++] = nfa->start;
    for (int i = 0; i < count; i++)
    {
        Node *state = states[i];
        for (int j = 0; j < state->n; j++)
        {
            int found = 0;
            for (int k = 0; k < count; k++)
            {
                if (states[k] == state->to[j])
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                states[count++] = state->to[j];
            }
        }
    }
    for (int i = 0; i < count; i++)
    {
        free(states[i]);
    }
    free(nfa); // Free the NFA structure itself
}

// Function to print the NFA transitions as State X --symbol--> State Y
void print_nfa(NFA *nfa)
{
    if (nfa == NULL)
    {
        printf("NFA is NULL\n");
        return;
    }
    Node *states[MAX_STATES]; // Array to hold states for printing
    int count = 0;
    states[count++] = nfa->start; // Start with the start state
    for (int i = 0; i < count; i++)
    {
        Node *state = states[i];
        for (int j = 0; j < state->n; j++)
        {
            if (state->sym[j] == 0)
            {
                printf("State %d --ε--> State %d\n", state->id, state->to[j]->id);
            }
            else
            {
                printf("State %d --%c--> State %d\n", state->id, state->sym[j], state->to[j]->id);
            }
            // Add the next state to the list if it's not already there
            int found = 0;
            for (int k = 0; k < count; k++)
            {
                if (states[k] == state->to[j])
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                states[count++] = state->to[j];
            }
        }
    }
    printf("\n");

    // Print the start and accept states
    printf("Start State: %d\n", nfa->start->id);
    printf("Accept State: %d\n\n", nfa->end->id);
}

// Function to create a new edge between two NFA states
void create_edge(Node *from, char sym, Node *to)
{
    if (from->n >= 2) // Thompson's construction limits outgoing edges only; the target's edges don't matter
    {
        fprintf(stderr, "Error: Cannot create more than 2 edges from a state.\n");
        return;
    }
    from->sym[from->n] = sym;
    from->to[from->n] = to;
    from->n++;
}

// Single symbol NFA
NFA *symbol_nfa(char sym, int start_id, int end_id)
{
    Node *start = create_node(start_id); // Create start state
    Node *end = create_node(end_id);     // Create accept state
    create_edge(start, sym, end);        // Create edge for the symbol
    return create_nfa(start, end);       // Return new NFA with start and end states
}

// Concatenate two NFAs with an epsilon transition from the end of the first NFA to the start of the second NFA
NFA *concatenate(NFA *nfa1, NFA *nfa2)
{
    create_edge(nfa1->end, 0, nfa2->start);           // Create epsilon transition from nfa1's end to nfa2's start
    NFA *result = create_nfa(nfa1->start, nfa2->end); // New NFA with nfa1's start and nfa2's end
    free(nfa1);                                       // The inputs are consumed: free only their wrappers, the states live on in result
    free(nfa2);
    return result;
}

// Union two NFAs with a new start state and a new accept state
NFA *union_nfa(NFA *nfa1, NFA *nfa2, int new_start_id, int new_end_id)
{
    Node *new_start = create_node(new_start_id); // Create a new start state
    Node *new_end = create_node(new_end_id);     // Create a new accept state
    create_edge(new_start, 0, nfa1->start);
    create_edge(new_start, 0, nfa2->start);
    create_edge(nfa1->end, 0, new_end);
    create_edge(nfa2->end, 0, new_end);
    free(nfa1); // The inputs are consumed: free only their wrappers, the states live on in the result
    free(nfa2);
    return create_nfa(new_start, new_end); // Return new NFA with new start and end states
}

// Kleene star operation on an NFA with a new start state and a new accept state
NFA *kleene_star(NFA *nfa, int new_start_id, int new_end_id)
{
    Node *new_start = create_node(new_start_id); // Create a new start state
    Node *new_end = create_node(new_end_id);     // Create a new accept state
    create_edge(new_start, 0, nfa->start);       // Epsilon transition from new start to old start
    create_edge(new_start, 0, new_end);          // Epsilon transition from new start to new end
    create_edge(nfa->end, 0, nfa->start);        // Epsilon transition from old end to old start
    create_edge(nfa->end, 0, new_end);           // Epsilon transition from old end to new end
    free(nfa);                                   // The input is consumed: free only its wrapper, the states live on in the result
    return create_nfa(new_start, new_end);       // Return new NFA with new start and end states
}

// Convert a postfix regex to an NFA
NFA *postfix_to_nfa(const char *postfix)
{
    if (postfix == NULL)
        return NULL;

    NFA *stack[MAX_STATES]; // Stack to hold intermediate NFAs
    int top = -1;           // Stack pointer
    int next_id = 0;

    // Pops and id increments are done in separate statements: C leaves the evaluation order of function
    // arguments unspecified, so f(x++, x++) or f(stack[top--], stack[top--]) is undefined behaviour.
    for (const char *p = postfix; *p != '\0'; p++)
    { // p is pointer to current char in postfix
        if (*p == '*')
        {
            int s = next_id++, e = next_id++;
            stack[top] = kleene_star(stack[top], s, e); // Replace the top NFA with its kleene star
        }
        else if (*p == '.')
        {
            NFA *b = stack[top--]; // Right operand is on top
            NFA *a = stack[top--]; // Left operand is below it
            stack[++top] = concatenate(a, b);
        }
        else if (*p == '|')
        {
            NFA *b = stack[top--];
            NFA *a = stack[top--];
            int s = next_id++, e = next_id++;
            stack[++top] = union_nfa(a, b, s, e);
        }
        else
        {
            int s = next_id++, e = next_id++;
            stack[++top] = symbol_nfa(*p, s, e); // Create a new NFA for the symbol and push it
        }
    }

    return stack[0]; // Return the final NFA at the top of the stack
}

// Function to find the ε-closure of a set of states T => every state reachable from T using only ε-edges.
// Fills closure[] (caller gives MAX_STATES slots) and returns the number of states in it!
int epsilon_closure(Node **T, int t_count, Node **closure)
{
    Node *stack[MAX_STATES]; // States whose ε-edges are yet to be followed
    int top = -1;
    int count = 0;
    for (int i = 0; i < t_count; i++) // Push all states of T onto stack and initialize ε-closure(T) to T
    {
        stack[++top] = T[i];
        closure[count++] = T[i];
    }
    while (top >= 0)
    {
        Node *t = stack[top--]; // Pop t, the top element, off stack
        for (int j = 0; j < t->n; j++)
        {
            if (t->sym[j] != 0)
                continue; // Only ε-edges (sym 0) count
            int found = 0;
            for (int k = 0; k < count; k++)
            {
                if (closure[k] == t->to[j])
                {
                    found = 1;
                    break;
                }
            }
            if (!found) // u is not in ε-closure(T): add it and push it
            {
                closure[count++] = t->to[j];
                stack[++top] = t->to[j];
            }
        }
    }
    return count;
}

// Function to collect the input alphabet of an NFA
// Fills alphabet[] (caller gives MAX_SYMBOLS slots) and returns the number of symbols in it.
int get_alphabet(NFA *nfa, char *alphabet)
{
    Node *states[MAX_STATES]; // Same traversal as print_nfa, to visit every state once
    int count = 0;
    int n_symbols = 0;
    states[count++] = nfa->start;
    for (int i = 0; i < count; i++)
    {
        Node *state = states[i];
        for (int j = 0; j < state->n; j++)
        {
            if (state->sym[j] != 0) // Add the symbol if it's not ε and not already there
            {
                int found = 0;
                for (int k = 0; k < n_symbols; k++)
                {
                    if (alphabet[k] == state->sym[j])
                    {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    alphabet[n_symbols++] = state->sym[j];
                }
            }
            // Add the next state to the list if it's not already there
            int found = 0;
            for (int k = 0; k < count; k++)
            {
                if (states[k] == state->to[j])
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                states[count++] = state->to[j];
            }
        }
    }
    return n_symbols;
}
