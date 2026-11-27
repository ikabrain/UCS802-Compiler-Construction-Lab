/*
 * parser.c - Function to parse a string with the SLR ACTION and GOTO table, using the shift-reduce algorithm
 */
#include <stdio.h>

#include "parser.h"

#define STACK_WIDTH 32 // Width of the Stack column when printing the parse
#define INPUT_WIDTH 24 // Width of the Input column when printing the parse

// Function to print one step of the parse as the Stack and the remaining Input, padded into columns
void print_step(Grammar *g, int *stack, int top, int *tokens, int n_tokens, int ip)
{
    int len = 0;
    for (int k = 0; k <= top; k++) // States are at even positions of the stack, grammar symbols at odd ones
    {
        if (k % 2 == 0)
            len += printf(k == 0 ? "%d" : " %d", stack[k]);
        else
            len += printf(" %s", g->names[stack[k]]);
    }
    printf("%*s", len < STACK_WIDTH ? STACK_WIDTH - len : 1, "");

    len = 0;
    for (int k = ip; k < n_tokens; k++)
    {
        len += printf("%s", g->names[tokens[k]]);
    }
    printf("%*s", len < INPUT_WIDTH ? INPUT_WIDTH - len : 1, "");
}

// Function to parse a string with the SLR table, printing each step; returns 1 if the string is accepted, 0 if not
int parse(Grammar *g, Table *t, const char *str)
{
    if (g == NULL || t == NULL || str == NULL)
        return 0;

    int tokens[MAX_INPUT + 1]; // Terminal ids of str, ending with $
    int n_tokens = tokenize(g, str, tokens);
    if (n_tokens == -1)
        return 0;

    int stack[MAX_STACK]; // S0 X1 S1 ... Xm Sm: states and grammar symbols
    int top = 0;
    stack[0] = 0; // Push 0 on stack
    int ip = 0;   // Index of the current token
    printf("%-*s%-*s%s\n", STACK_WIDTH, "Stack", INPUT_WIDTH, "Input", "Action");
    while (1)
    {
        int state = stack[top]; // State = top of stack
        Action action = t->action[state][tokens[ip]];
        print_step(g, stack, top, tokens, n_tokens, ip);
        if (action.type == 's') // Shift: push the token and state i on the stack, then read the next token
        {
            if (top + 2 >= MAX_STACK)
            {
                printf("error: stack is full\n");
                return 0;
            }
            stack[++top] = tokens[ip++];
            stack[++top] = action.n;
            printf("shift %d\n", action.n);
        }
        else if (action.type == 'r') // Reduce by A -> β: pop 2|β| items, then push A and GOTO[state, A]
        {
            Production *p = &g->prods[action.n];
            top -= 2 * p->n;
            state = stack[top]; // Restore the state from before the reduction
            if (top + 2 >= MAX_STACK)
            {
                printf("error: stack is full\n");
                return 0;
            }
            stack[++top] = p->lhs;
            stack[++top] = t->go[state][p->lhs]; // State after the reduction
            printf("reduce ");
            print_production(g, action.n, -1);
            printf("\n");
        }
        else if (action.type == 'a') // Accept: the sentence is parsed
        {
            printf("accept\n");
            return 1;
        }
        else // Blank entry: error in the sentence
        {
            printf("error\n");
            return 0;
        }
    }
}
