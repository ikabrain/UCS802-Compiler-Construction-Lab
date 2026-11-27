#include <stdio.h>
#include <stdlib.h>
#include "grammar.h"
#include "items.h"
#include "table.h"
#include "parser.h"

int main()
{
    int n = 0;
    printf("Enter number of productions: ");
    if (scanf("%d", &n) != 1 || n < 1 || n >= MAX_PRODUCTIONS)
    {
        fprintf(stderr, "Error: Number of productions must be from 1 to %d\n", MAX_PRODUCTIONS - 1);
        return 1;
    }
    char *lines[MAX_PRODUCTIONS] = {NULL}; // %ms allocates each line itself, so no malloc here
    for (int i = 0; i < n; i++)
    {
        printf("Enter production %d (like E->E+T|T, # for ε): ", i + 1);
        if (scanf("%ms", &lines[i]) != 1)
        {
            fprintf(stderr, "Error: No input\n");
            for (int j = 0; j < n; j++)
                free(lines[j]); // Lines not read yet are NULL, and free(NULL) does nothing
            return 1;
        }
    }
    Grammar *g = init_grammar(lines, n);
    for (int i = 0; i < n; i++)
        free(lines[i]);
    if (g)
    {
        printf("\n----------- GRAMMAR -----------\n");
        print_grammar(g);
        printf("----------- FIRST & FOLLOW -----------\n");
        print_first_follow(g);
        Collection *c = build_collection(g);
        printf("----------- LR(0) ITEMS -----------\n");
        print_collection(g, c);
        Table *t = build_table(g, c);
        printf("----------- SLR TABLE -----------\n");
        print_table(g, t);
        if (t && t->conflicts)
        {
            printf("Grammar is not SLR(1).\n");
        }
        else if (t)
        {
            char *str = NULL; // %ms allocates the buffer itself, so no malloc here
            printf("Enter a string to parse (like id+id*id): ");
            if (scanf("%ms", &str) == 1)
            {
                printf("\n----------- PARSING -----------\n");
                printf("%s\n", parse(g, t, str) ? "Accept" : "Reject");
                free(str);
            }
        }
        free(t);
        free_collection(c);
        free(g);
    }
    return 0;
}
