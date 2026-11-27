#ifndef ITEMS_H
#define ITEMS_H

#include "grammar.h"

#define MAX_ITEMS (MAX_PRODUCTIONS * (MAX_RHS + 1)) // Every possible LR(0) item, so one set can never overflow
#define MAX_SETS 256                                // Sets in the canonical collection

// Structure to represent one LR(0) item A -> α.β; dot is the number of right side symbols before the dot
typedef struct Item
{
    int prod, dot; // prod is the production number, dot is the position of the dot in its right side
} Item;

// Structure to represent one set of LR(0) items; a state of the DFA that recognizes handles
typedef struct ItemSet
{
    Item items[MAX_ITEMS]; // Kernel items first, then the items added by closure
    int go[MAX_SYMBOLS];   // go[X] is the id of the set goto(I, X), -1 if it is empty
    int n, id;             // n is the number of items, id is the set id
} ItemSet;

typedef struct Collection
{
    ItemSet *sets[MAX_SETS]; // sets[0] is closure({S' -> .S})
    int count;               // Number of sets
} Collection;


void free_collection(Collection *c);
void print_collection(Grammar *g, Collection *c);
Collection *build_collection(Grammar *g);

#endif
