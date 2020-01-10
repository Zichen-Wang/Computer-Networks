#include <string.h>


void parse(char *argv[], char buffer[]) {
    char *token;
    int argc;

    // reset the arg counter
    argc = 0;

    // get the first token
    token = strtok(buffer, " ");

    while (token != NULL) {
        // if the current token is not NULL, store this token
        argv[argc++] = token;

        // get the next token
        token = strtok(NULL, " ");
    }

    // make sure argv ends with NULL
    argv[argc] = NULL;
}
