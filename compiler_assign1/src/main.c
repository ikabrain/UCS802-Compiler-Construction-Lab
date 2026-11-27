#include <stdio.h>
#include <stdlib.h>
#include "regex.h"
#include "nfa.h"

int main()
{
    char *regex = NULL; // %ms allocates the buffer itself, so no malloc here
    printf("Enter a regex string (allowed symbols: a-z, A-Z; allowed operations: |, *, implicit concatenation): ");
    if (scanf("%ms", &regex) != 1) {
        fprintf(stderr, "Error: No input\n");
        return 1;
    }
    char* init = init_regex(regex);
    if (init) {
        printf("Initialized regex: %s\n", init);
        char* postfix = infix_to_postfix(init);
        if (postfix) {
            printf("Postfix regex: %s\n", postfix);
            NFA *nfa = postfix_to_nfa(postfix);
            free(postfix);
            print_nfa(nfa);
            free_nfa(nfa);
        }
        free(init);
    }
    free(regex);
    return 0;
}
