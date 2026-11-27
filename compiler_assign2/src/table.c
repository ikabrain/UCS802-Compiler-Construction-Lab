/*
 * table.c - Function to build the SLR ACTION and GOTO table from the canonical LR(0) collection and FOLLOW sets
 */
#include <stdio.h>
#include <stdlib.h>

#include "table.h"

#define CELL_WIDTH 6 // Width of one column when printing the table

// Function to write an ACTION entry as text into buf (caller gives 16 chars): s5, r2, acc, or "" for a blank entry
void format_action(Action action, char *buf)
{
    if (action.type == 's')
        snprintf(buf, 16, "s%d", action.n);
    else if (action.type == 'r')
        snprintf(buf, 16, "r%d", action.n);
    else if (action.type == 'a')
        snprintf(buf, 16, "acc");
    else
        buf[0] = '\0';
}

// Function to set ACTION[state, a]; a different action already there is a conflict, which is reported and counted
void set_action(Grammar *g, Table *t, int state, int a, char type, int n)
{
    Action *entry = &t->action[state][a];
    if (entry->type == type && entry->n == n)
        return; // Already set by another item
    if (entry->type != 0)
    {
        char existing[16], incoming[16];
        Action action = {type, n};
        format_action(*entry, existing);
        format_action(action, incoming);
        fprintf(stderr, "Error: Conflict in state %d on '%s': %s / %s\n", state, g->names[a], existing, incoming);
        t->conflicts++;
        return; // Keep the first action
    }
    entry->type = type;
    entry->n = n;
}

// Function to print the table as rows of states, with ACTION columns for terminals and GOTO columns for non-terminals
void print_table(Grammar *g, Table *t)
{
    if (t == NULL)
    {
        printf("Table is NULL\n");
        return;
    }
    int n_terminals = 0;
    for (int X = 0; X < g->n_symbols; X++)
    {
        if (g->is_terminal[X])
            n_terminals++;
    }
    printf("%-*s| %-*s| %s\n", CELL_WIDTH, "", n_terminals * CELL_WIDTH, "ACTION", "GOTO");
    printf("%-*s| ", CELL_WIDTH, "State");
    for (int a = 0; a < g->n_symbols; a++)
    {
        if (g->is_terminal[a])
            printf("%-*s", CELL_WIDTH, g->names[a]);
    }
    printf("| ");
    for (int A = 0; A < g->n_symbols; A++)
    {
        if (!g->is_terminal[A] && A != g->start)
            printf("%-*s", CELL_WIDTH, g->names[A]);
    }
    printf("\n");

    char cell[16];
    for (int i = 0; i < t->n_states; i++)
    {
        printf("%-*d| ", CELL_WIDTH, i);
        for (int a = 0; a < g->n_symbols; a++)
        {
            if (!g->is_terminal[a])
                continue;
            format_action(t->action[i][a], cell);
            printf("%-*s", CELL_WIDTH, cell);
        }
        printf("| ");
        for (int A = 0; A < g->n_symbols; A++)
        {
            if (g->is_terminal[A] || A == g->start)
                continue;
            if (t->go[i][A] != -1)
                printf("%-*d", CELL_WIDTH, t->go[i][A]);
            else
                printf("%-*s", CELL_WIDTH, "");
        }
        printf("\n");
    }
    printf("\n");
}

// Build the SLR ACTION and GOTO table; conflicts are reported and counted in t->conflicts
Table *build_table(Grammar *g, Collection *c)
{
    if (g == NULL || c == NULL)
        return NULL;

    Table *t = (Table *)calloc(1, sizeof(Table)); // calloc, so every ACTION entry starts blank
    t->n_states = c->count;
    for (int i = 0; i < c->count; i++)
    {
        ItemSet *set = c->sets[i];
        for (int X = 0; X < g->n_symbols; X++) // GOTO: goto(Ii, A) = Ij means GOTO[i, A] = j
        {
            t->go[i][X] = g->is_terminal[X] ? -1 : set->go[X];
        }
        for (int j = 0; j < set->n; j++)
        {
            int prod = set->items[j].prod, dot = set->items[j].dot;
            Production *p = &g->prods[prod];
            if (dot < p->n && g->is_terminal[p->rhs[dot]]) // A -> α.aβ in Ii and goto(Ii, a) = Ij: shift j
            {
                set_action(g, t, i, p->rhs[dot], 's', set->go[p->rhs[dot]]);
            }
            else if (dot == p->n && prod == 0) // S' -> S. in Ii: accept on $
            {
                set_action(g, t, i, g->end, 'a', 0);
            }
            else if (dot == p->n) // A -> α. in Ii: reduce A -> α on every a in FOLLOW(A)
            {
                for (int a = 0; a < g->n_symbols; a++)
                {
                    if (g->follow[p->lhs][a])
                        set_action(g, t, i, a, 'r', prod);
                }
            }
        }
    }
    return t;
}
