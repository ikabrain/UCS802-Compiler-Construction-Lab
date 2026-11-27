/*
 * regex.c - Function to init regex string and add explicit concatenation with '.' for valid regex, and function to convert to postfix regex for Thompson's construction
 * Allowed symbols:  a-z, A-Z
 * Allowed operations:  |  union          (lowest precedence)
 *                      .  concatenation  (typed implicitly: "ab" means a.b)
 *                      *  Kleene closure (highest precedence)
 *                      (  )  grouping
*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

// Function that takes in regex string and outputs concatenated string with '.' for concatenation if valid regex, else returns NULL and prints error message
char* init_regex(const char* regex) {
    int len = strlen(regex);
    // 1. Is the string of valid length?
    if (len == 0) {
        fprintf(stderr, "Error: Empty regex\n");
        return NULL;
    }
    if (len > MAX_REGEX_LENGTH) {
        fprintf(stderr, "Error: Regex too long\n");
        return NULL;
    }

    // 2. Does it have valid operators?
    int depth = 0; // how many '(' are still open
    int prev_ends_operand = 0; // whether the previous character ended '*' or '|' or ')' operands
    for (int i = 0; i < len; i++) {
        char c = regex[i];
        if (!(c == '|' || c == '*' || c == '(' || c == ')' || isalpha(c))) {
            fprintf(stderr, "Error: Invalid character '%c' in regex\n", c);
            if (c == '.') {
                fprintf(stderr, "'.' is not allowed in input regex. Use implicit concatenation.\n");
            }
            return NULL;
        }
        // 3. Are the operators in valid positions?
        if (c == '(') {
            depth++;
            prev_ends_operand = 0;
        } else if (c == ')') {
            if (depth == 0) {
                fprintf(stderr, "Error: Unmatched ')' at position %d\n", i);
                return NULL;
            }
            if (!prev_ends_operand) {
                fprintf(stderr, "Error: Nothing inside brackets before position %d\n", i);
                return NULL;
            }
            depth--;
            prev_ends_operand = 1;
        } else if (c == '*') {
            if (!prev_ends_operand) {
                fprintf(stderr, "Error: '*' at position %d has nothing to repeat\n", i);
                return NULL;
            }
        } else if (c == '|') {
            if (!prev_ends_operand) {
                fprintf(stderr, "Error: '|' at position %d is missing its left side\n", i);
                return NULL;
            }
            prev_ends_operand = 0;
        } else { // an alphabet symbol
            prev_ends_operand = 1;
        }
    }
    if (depth != 0)
    {
        fprintf(stderr, "Error: Missing ')'\n");
        return NULL;
    }
    if (!prev_ends_operand)
    {
        fprintf(stderr, "Error: Regex ends with an operator that is missing its right side\n");
        return NULL;
    }

    // 4. For valid regex, add explicit concatenation with '.'
    char* new_regex = malloc(2 * len + 1); // Allocate enough space for the new regex
    int j = 0; // Index for new_regex
    for (int i = 0; i < len; i++) {
        char c = regex[i];
        if (i > 0) {
            char p = regex[i - 1];
            int prev_ends_operand = !(p == '(' || p == '|');
            int cur_starts_operand = !(c == ')' || c == '|' || c == '*');
            if (prev_ends_operand && cur_starts_operand) new_regex[j++] = '.'; // If previous character ends an operand and current character starts an operand, add '.' between them for concatenation
        }
        new_regex[j++] = c;
    }
    new_regex[j] = '\0';
    return new_regex;
}


// Regex character precedence
static int precedence(char c) {
    switch (c) {
        case '*': return 3;
        case '.': return 2;
        case '|': return 1;
        default: return 0; // for '(' and ')'
    }
}


// Function that converts given valid infix regex to postfix regex using shunting-yard algorithm
char* infix_to_postfix(const char* regex) {
    int len = strlen(regex);
    char* postfix = malloc(len + 1); // Allocate space for postfix regex
    char* stack = malloc(len + 1); // Stack for operators
    int top = 0; // Top of the stack
    int j = 0; // Index for postfix

    for (int i = 0; i < len; i++) {
        char c = regex[i];
        if (isalpha(c)) { // If it's an alphabet symbol, add to postfix
            postfix[j++] = c;
        } else if (c == '(') { // If it's '(', push to stack
            stack[top++] = c;
        } else if (c == ')') { // If it's ')', pop from stack to postfix until '(' is found
            while (top > 0 && stack[top - 1] != '(') {
                postfix[j++] = stack[--top];
            }
            top--; // Pop the '(' from the stack
        } else { // If it's an operator, pop from stack to postfix based on precedence
            while (top > 0 && precedence(stack[top - 1]) >= precedence(c)) {
                postfix[j++] = stack[--top];
            }
            stack[top++] = c; // Push current operator to stack
        }
    }
    while (top > 0) { // Pop any remaining operators from the stack to postfix
        postfix[j++] = stack[--top];
    }
    postfix[j] = '\0'; // Null-terminate the postfix string
    free(stack); // Free the allocated memory for the stack
    return postfix; // Return the resulting postfix regex
}