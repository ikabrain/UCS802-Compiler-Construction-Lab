/*
 * grammar.c - Function to read a CFG, augment it with S' -> S, and find the FIRST and FOLLOW sets of its symbols
 * Non-terminals:  an uppercase letter, optionally followed by 's      (E, T, E')
 * Terminals:      a run of lowercase letters and digits               (id, num)
 *                 or any other single character                       (+, *, (, ))
 * Special:        '|' separates alternatives, '#' on its own is ε, '$' is the end marker
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grammar.h"

// Function to read the name of the next symbol at s into name (if it fits in MAX_NAME); returns its length, 0 at the end of s
int read_symbol(const char *s, char *name)
{
    int len = 0;
    if (isupper((unsigned char)s[0]))
    {
        len = 1;
        while (s[len] == '\'')
            len++; // Primes belong to the non-terminal: E'
    }
    else if (islower((unsigned char)s[0]) || isdigit((unsigned char)s[0]))
    {
        while (islower((unsigned char)s[len]) || isdigit((unsigned char)s[len]))
            len++; // A run of lowercase letters and digits is one terminal: id
    }
    else if (s[0] != '\0')
    {
        len = 1; // Any other character is a terminal on its own: +
    }
    if (len < MAX_NAME)
    {
        memcpy(name, s, len);
        name[len] = '\0';
    }
    return len;
}

// Function to find a symbol by name; returns its id, or -1 if it's not in the grammar
int find_symbol(Grammar *g, const char *name)
{
    for (int X = 0; X < g->n_symbols; X++)
    {
        if (strcmp(g->names[X], name) == 0)
        {
            return X;
        }
    }
    return -1;
}

// Function to add a symbol if it's new; returns its id, or -1 if there is no room for it
int add_symbol(Grammar *g, const char *name, int is_terminal)
{
    int X = find_symbol(g, name);
    if (X != -1)
        return X;
    if (g->n_symbols >= MAX_SYMBOLS)
        return -1;
    strcpy(g->names[g->n_symbols], name);
    g->is_terminal[g->n_symbols] = is_terminal;
    return g->n_symbols++;
}

// Function to report an error in production line, free the grammar and return NULL
Grammar *grammar_error(Grammar *g, const char *message, int line)
{
    fprintf(stderr, "Error: %s in production %d\n", message, line);
    free(g);
    return NULL;
}

// Function to add every terminal in from[] to to[]; returns 1 if to[] got anything new, 0 if not
int add_set(int *to, int *from, int n)
{
    int added = 0;
    for (int a = 0; a < n; a++)
    {
        if (from[a] && !to[a])
        {
            to[a] = 1;
            added = 1;
        }
    }
    return added;
}

// Function to find FIRST and nullable for every symbol, repeating until no set grows
void compute_first(Grammar *g)
{
    for (int X = 0; X < g->n_symbols; X++)
    {
        if (g->is_terminal[X])
            g->first[X][X] = 1; // FIRST of a terminal is the terminal itself
    }
    int changed = 1;
    while (changed)
    {
        changed = 0;
        for (int i = 0; i < g->n_prods; i++)
        {
            Production *p = &g->prods[i];
            int j = 0;
            for (; j < p->n; j++) // Add FIRST of each symbol on the right, until one can't derive ε
            {
                changed |= add_set(g->first[p->lhs], g->first[p->rhs[j]], g->n_symbols);
                if (!g->nullable[p->rhs[j]])
                    break;
            }
            if (j == p->n && !g->nullable[p->lhs]) // Every symbol on the right derives ε, or the right side is ε
            {
                g->nullable[p->lhs] = 1;
                changed = 1;
            }
        }
    }
}

// Function to find FOLLOW for every non-terminal, repeating until no set grows
void compute_follow(Grammar *g)
{
    g->follow[g->start][g->end] = 1; // $ follows the start symbol
    int changed = 1;
    while (changed)
    {
        changed = 0;
        for (int i = 0; i < g->n_prods; i++)
        {
            Production *p = &g->prods[i];
            for (int j = 0; j < p->n; j++) // For each non-terminal B in A -> αBβ
            {
                int B = p->rhs[j];
                if (g->is_terminal[B])
                    continue;
                int k = j + 1;
                for (; k < p->n; k++) // FIRST(β) is in FOLLOW(B), up to the first symbol that can't derive ε
                {
                    changed |= add_set(g->follow[B], g->first[p->rhs[k]], g->n_symbols);
                    if (!g->nullable[p->rhs[k]])
                        break;
                }
                if (k == p->n) // β derives ε, so FOLLOW(A) is in FOLLOW(B)
                    changed |= add_set(g->follow[B], g->follow[p->lhs], g->n_symbols);
            }
        }
    }
}

// Function that takes in the production lines of a CFG and returns the augmented grammar with FIRST and FOLLOW
// if every line is valid, else returns NULL and prints an error message
Grammar *init_grammar(char **lines, int n)
{
    if (lines == NULL || n < 1)
        return NULL;

    Grammar *g = (Grammar *)calloc(1, sizeof(Grammar)); // calloc, so every set starts empty
    g->n_prods = 1;                                     // prods[0] is kept for S' -> S, added once the start symbol is known
    char name[MAX_NAME];
    for (int i = 0; i < n; i++)
    {
        const char *line = lines[i];
        // 1. Is the line of valid length?
        if (strlen(line) > MAX_INPUT)
            return grammar_error(g, "Line too long", i + 1);

        // 2. Does it start with a non-terminal followed by "->"?
        int len = read_symbol(line, name);
        if (!isupper((unsigned char)line[0]) || len >= MAX_NAME)
            return grammar_error(g, "Left side must be one non-terminal like E or E'", i + 1);
        if (strncmp(line + len, "->", 2) != 0)
            return grammar_error(g, "Missing '->' after the left side", i + 1);
        int lhs = add_symbol(g, name, 0);
        if (lhs == -1)
            return grammar_error(g, "Too many symbols", i + 1);

        // 3. Split the right side on '|' into one production per alternative
        const char *p = line + len + 2; // p is pointer to current char in the right side
        while (1)
        {
            if (g->n_prods >= MAX_PRODUCTIONS)
                return grammar_error(g, "Too many productions", i + 1);
            Production *prod = &g->prods[g->n_prods];
            prod->lhs = lhs;
            prod->n = 0;
            if (*p == '|' || *p == '\0')
                return grammar_error(g, "Empty alternative (write # for ε)", i + 1);
            if (*p == '#') // ε: the alternative must be '#' alone
            {
                p++;
                if (*p != '|' && *p != '\0')
                    return grammar_error(g, "'#' (ε) must be an alternative on its own", i + 1);
            }
            while (*p != '|' && *p != '\0') // Read the symbols of this alternative
            {
                if (*p == '#')
                    return grammar_error(g, "'#' (ε) must be an alternative on its own", i + 1);
                if (*p == '$')
                    return grammar_error(g, "'$' is reserved for the end marker", i + 1);
                if (*p == '\'')
                    return grammar_error(g, "''' must follow an uppercase non-terminal", i + 1);
                len = read_symbol(p, name);
                if (len >= MAX_NAME)
                    return grammar_error(g, "Symbol name too long", i + 1);
                if (prod->n >= MAX_RHS)
                    return grammar_error(g, "Right side too long", i + 1);
                int X = add_symbol(g, name, !isupper((unsigned char)*p));
                if (X == -1)
                    return grammar_error(g, "Too many symbols", i + 1);
                prod->rhs[prod->n++] = X;
                p += len;
            }
            g->n_prods++;
            if (*p == '\0')
                break;
            p++; // Skip '|' to the next alternative
        }
    }

    // 4. Does every non-terminal have a production?
    for (int X = 0; X < g->n_symbols; X++)
    {
        if (g->is_terminal[X])
            continue;
        int found = 0;
        for (int i = 1; i < g->n_prods; i++)
        {
            if (g->prods[i].lhs == X)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "Error: Non-terminal '%s' has no production\n", g->names[X]);
            free(g);
            return NULL;
        }
    }

    // 5. Add the end marker $, and augment the grammar with S' -> S, where S' is the start symbol plus enough 's to be new
    g->end = add_symbol(g, "$", 1);
    strcpy(name, g->names[g->prods[1].lhs]);
    do
    {
        if (strlen(name) + 1 >= MAX_NAME)
            return grammar_error(g, "No room to name the augmented start symbol", 1);
        strcat(name, "'");
    } while (find_symbol(g, name) != -1);
    g->start = add_symbol(g, name, 0);
    if (g->end == -1 || g->start == -1)
        return grammar_error(g, "Too many symbols", n);
    g->prods[0].lhs = g->start;
    g->prods[0].rhs[0] = g->prods[1].lhs;
    g->prods[0].n = 1;

    compute_first(g);
    compute_follow(g);
    return g;
}

// Function to split a string into the ids of its terminals and append $; returns the number of tokens,
// or -1 if the string has a symbol that is not a terminal of the grammar. Fills tokens[] (caller gives MAX_INPUT + 1 slots)
int tokenize(Grammar *g, const char *str, int *tokens)
{
    if (strlen(str) > MAX_INPUT)
    {
        fprintf(stderr, "Error: String too long\n");
        return -1;
    }
    char name[MAX_NAME];
    int count = 0;
    for (const char *p = str; *p != '\0';) // p is pointer to current char in str
    {
        int len = read_symbol(p, name);
        int X = (len < MAX_NAME) ? find_symbol(g, name) : -1;
        if (X == -1 || !g->is_terminal[X] || X == g->end)
        {
            fprintf(stderr, "Error: '%.*s' is not a terminal of the grammar\n", len, p);
            return -1;
        }
        tokens[count++] = X;
        p += len;
    }
    tokens[count++] = g->end; // End the input with $
    return count;
}

// Function to print production i as A -> X Y Z, with a dot before position dot of the right side (-1 for no dot)
void print_production(Grammar *g, int i, int dot)
{
    Production *p = &g->prods[i];
    printf("%s ->", g->names[p->lhs]);
    for (int j = 0; j < p->n; j++)
    {
        printf(j == dot ? " . %s" : " %s", g->names[p->rhs[j]]);
    }
    if (dot == p->n)
        printf(" .");
    else if (p->n == 0)
        printf(" ε");
}

// Function to print the augmented grammar with numbered productions, and its terminals and non-terminals
void print_grammar(Grammar *g)
{
    if (g == NULL)
    {
        printf("Grammar is NULL\n");
        return;
    }
    for (int i = 0; i < g->n_prods; i++)
    {
        printf("%d) ", i);
        print_production(g, i, -1);
        printf("\n");
    }
    printf("\n");

    for (int terminal = 1; terminal >= 0; terminal--)
    {
        printf(terminal ? "Terminals: {" : "Non-terminals: {");
        int count = 0;
        for (int X = 0; X < g->n_symbols; X++)
        {
            if (g->is_terminal[X] == terminal)
                printf(count++ == 0 ? "%s" : ", %s", g->names[X]);
        }
        printf("}\n");
    }
    printf("\n");
}

// Function to print a set of terminals as {a, b, c}, with ε at the end if with_epsilon is 1
void print_set(Grammar *g, int *set, int with_epsilon)
{
    printf("{");
    int count = 0;
    for (int a = 0; a < g->n_symbols; a++)
    {
        if (set[a])
            printf(count++ == 0 ? "%s" : ", %s", g->names[a]);
    }
    if (with_epsilon)
        printf(count == 0 ? "ε" : ", ε");
    printf("}");
}

// Function to print FIRST and FOLLOW of every non-terminal except S'
void print_first_follow(Grammar *g)
{
    if (g == NULL)
    {
        printf("Grammar is NULL\n");
        return;
    }
    for (int X = 0; X < g->n_symbols; X++)
    {
        if (g->is_terminal[X] || X == g->start)
            continue;
        printf("FIRST(%s) = ", g->names[X]);
        print_set(g, g->first[X], g->nullable[X]);
        printf("\n");
    }
    printf("\n");

    for (int X = 0; X < g->n_symbols; X++)
    {
        if (g->is_terminal[X] || X == g->start)
            continue;
        printf("FOLLOW(%s) = ", g->names[X]);
        print_set(g, g->follow[X], 0);
        printf("\n");
    }
    printf("\n");
}
