#ifndef REGEX_H
#define REGEX_H

#define MAX_REGEX_LENGTH 255 // Maximum length of the regex string
char* init_regex(const char* regex);
char* infix_to_postfix(const char* regex);

#endif