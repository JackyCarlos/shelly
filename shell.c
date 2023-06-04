#include "shelly.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    // Load config files. if any. 

    // Run command loop
    shelly_loop();

    // perform any shutdown/cleanup
    return 0; 
}

void shelly_loop(void)
{
    char *line;  
    char **args;
    int status = 1;

    do {
        printf("$ ");
        line = shelly_read_line();
        args = shelly_split_line(line);
        status = shelly_execute(args);
        
        // for(; *args != NULL; args++) {
        //     printf("Argument: %s\n", *args);
        // }

    } while (status);
}

char *shelly_read_line(void)
{
    int bufsize = SHELLY_BUF;
    char *buffer = (char *) malloc(sizeof(char) * bufsize);
    char *buf = buffer;
    int c; 

    if (!buffer) {
        fprintf(stderr, "shelly: allocation error\n");
        exit(1);
    }

    while ((c = getchar()) != EOF && c != '\n') {
        *buf++ = c;

        if(buf - buffer == bufsize) {
            bufsize += SHELLY_BUF;
            buffer = realloc(buffer, bufsize);

            if (!buffer) {
                fprintf(stderr, "shelly: allocation error\n");
                exit(1);
            }
        }
    }

    *buf = '\0';
    return buffer;
}

char **shelly_split_line(char *line)
{
    int bufsize = SHELLY_TOK_BUFSIZE;
    char **tokens = (char **) malloc(sizeof(char *) * bufsize);
    char **tok = tokens;
    char *token; 

    if (!tokens) {
        fprintf(stderr, "shelly: allocation error\n");
        exit(1);
    }

    token = strtok(line, SHELLY_TOK_DELIM);
    while (token != NULL) {
        *tok++ = token;

        if (tok - tokens == bufsize) {
            bufsize += SHELLY_TOK_BUFSIZE;
            tokens = realloc(tokens, sizeof(char *) * bufsize);

            if (!tokens) {
                fprintf(stderr, "shelly: allocation error\n");
                exit(1);
            }
        }

        token = strtok(NULL, SHELLY_TOK_DELIM);
    }

    *tok = NULL;

    return tokens;
}

int shelly_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (!pid) {
        // Child process
        if (execvp(*args, args) == -1) 
            perror("shelly");
        exit(1);
    } else if (pid < 0) {
        // Error forking
        perror("shelly");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}
