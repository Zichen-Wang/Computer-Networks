// simple shell example using fork() and execlp()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
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

int main(void) {
    pid_t k;

    char buf[100];
    int len;

    char *argv[100];

    int status;

 // int i;

    while (1) {

        // print prompt
        fprintf(stdout,"[%d]$ ",getpid());

        // read command from stdin
        fgets(buf, 100, stdin);
        len = strlen(buf);
        if (len == 1)               // only return key pressed
            continue;

        buf[len - 1] = '\0';

        parse(argv, buf);

        /*
        i = 0;
        while (argv[i]) {
            printf("%s (%ld)\n", argv[i], strlen(argv[i]));
            i++;
        }
        printf("tot %d\n", i);
        */

        k = fork();

        if (k == 0) {
            // child code
            if (execvp((const char *)argv[0], argv) == -1) {
                // if execution failed, terminate child
                exit(1);
            }
        } else {
            // parent code
            waitpid(k, &status, 0);
        }
    }
}
