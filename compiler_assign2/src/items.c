/*
 * items.c - Function to build the canonical collection of LR(0) item sets for an augmented grammar
 */
#include <stdio.h>
#include <stdlib.h>

#include "items.h"

// Function to check whether an item is in a list of items; returns 1 if it is, 0 if not
int has_item(Item *items, int n, Item item)
{
    for (int i = 0; i < n; i++)
    {
        if (items[i].prod == item.prod && items[i].dot == item.dot)
        {
            return 1;
        }
    }
    return 0;
}

// Function to find closure(I): I, plus B -> .γ for every item A -> α.Bβ in it, repeated for the items it adds.
// Fills result[] (caller gives MAX_ITEMS slots) and returns the number of items in it.
int closure(Grammar *g, Item *I, int n, Item *result)
{
    int count = 0;
    for (int i = 0; i < n; i++) // Put every item in I into closure(I)
    {
        result[count++] = I[i];
    }
    for (int i = 0; i < count; i++) // New items go at the end, so this loop visits them too
    {
        Production *p = &g->prods[result[i].prod];
        if (result[i].dot == p->n || g->is_terminal[p->rhs[result[i].dot]])
            continue; // The dot is at the end or before a terminal
        int B = p->rhs[result[i].dot];
        for (int j = 0; j < g->n_prods; j++) // Add B -> .γ for every production of B
        {
            Item item = {j, 0};
            if (g->prods[j].lhs == B && !has_item(result, count, item))
            {
                result[count++] = item;
            }
        }
    }
    return count;
}

// Function to find goto(I, X): the closure of A -> αX.β for every item A -> α.Xβ in I.
// Fills result[] (caller gives MAX_ITEMS slots) and returns the number of items in it, 0 if it is empty.
int goto_items(Grammar *g, Item *I, int n, int X, Item *result)
{
    Item moved[MAX_ITEMS]; // Items of I with the dot moved past X
    int m = 0;
    for (int i = 0; i < n; i++)
    {
        Production *p = &g->prods[I[i].prod];
        if (I[i].dot < p->n && p->rhs[I[i].dot] == X)
        {
            Item item = {I[i].prod, I[i].dot + 1};
            moved[m++] = item;
        }
    }
    return closure(g, moved, m, result);
}

// Function to create a new item set from a list of items
ItemSet *create_set(Item *items, int n, int id, int n_symbols)
{
    ItemSet *set = (ItemSet *)malloc(sizeof(ItemSet));
    for (int i = 0; i < n; i++)
    {
        set->items[i] = items[i];
    }
    for (int X = 0; X < n_symbols; X++)
    {
        set->go[X] = -1; // No transition yet
    }
    set->n = n;
    set->id = id;
    return set;
}

// Function to find the set with the same items, in any order; returns its id, or -1 if it's new
int find_set(Collection *c, Item *items, int n)
{
    for (int i = 0; i < c->count; i++)
    {
        ItemSet *set = c->sets[i];
        if (set->n != n)
            continue; // Sets of different sizes can't be the same
        int same = 1;
        for (int j = 0; j < n; j++)
        {
            if (!has_item(set->items, set->n, items[j]))
            {
                same = 0;
                break;
            }
        }
        if (same)
        {
            return set->id;
        }
    }
    return -1;
}

// Function to free the memory allocated for a collection with all its item sets
void free_collection(Collection *c)
{
    if (c == NULL)
        return;

    for (int i = 0; i < c->count; i++)
    {
        free(c->sets[i]);
    }
    free(c); // Free the collection structure itself
}

// Function to print every item set as I0: followed by its items, and transitions as Ii --X--> Ij
void print_collection(Grammar *g, Collection *c)
{
    if (c == NULL)
    {
        printf("Collection is NULL\n");
        return;
    }
    for (int i = 0; i < c->count; i++)
    {
        ItemSet *set = c->sets[i];
        printf("I%d:\n", set->id);
        for (int j = 0; j < set->n; j++)
        {
            printf("    ");
            print_production(g, set->items[j].prod, set->items[j].dot);
            printf("\n");
        }
    }
    printf("\n");

    for (int i = 0; i < c->count; i++)
    {
        ItemSet *set = c->sets[i];
        for (int X = 0; X < g->n_symbols; X++)
        {
            if (set->go[X] != -1)
            {
                printf("I%d --%s--> I%d\n", set->id, g->names[X], set->go[X]);
            }
        }
    }
    printf("\n");
}

// Build the canonical LR(0) collection C of the augmented grammar, the same way subset construction builds a DFA
Collection *build_collection(Grammar *g)
{
    if (g == NULL)
        return NULL;

    Collection *c = (Collection *)malloc(sizeof(Collection));
    c->count = 0;

    Item items[MAX_ITEMS];
    Item start = {0, 0};                  // S' -> .S
    int n = closure(g, &start, 1, items); // C is { closure({S' -> .S}) }
    c->sets[c->count++] = create_set(items, n, 0, g->n_symbols);

    // Sets are added in order, so every set before i has its gotos and every set from i on still needs them
    for (int i = 0; i < c->count; i++)
    {
        ItemSet *set = c->sets[i];
        for (int X = 0; X < g->n_symbols; X++) // For each grammar symbol X
        {
            n = goto_items(g, set->items, set->n, X, items);
            if (n == 0)
                continue; // goto(I, X) is empty
            int id = find_set(c, items, n);
            if (id == -1) // goto(I, X) is not in C: add it
            {
                if (c->count >= MAX_SETS)
                {
                    fprintf(stderr, "Error: Cannot create more than %d item sets.\n", MAX_SETS);
                    free_collection(c); // A partial collection would give a wrong table
                    return NULL;
                }
                id = c->count;
                c->sets[c->count++] = create_set(items, n, id, g->n_symbols);
            }
            set->go[X] = id;
        }
    }
    return c;
}
