#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shelly.h"

void shelly_loop(void);
void print_cli(void);
char *read_line(void);
char **split_line(char *line);
int run_command(char **tokens);
int launch_command(char **tokens);

int main(int argc, char *argv[]) {
    shelly_loop();
    
    return 0;
}

void shelly_loop(void) {
    char *line;
    char **tokens;
    int status;

    status = 1;

    while (status) {
        print_cli();
        
        line = read_line();
        tokens = split_line(line);
        
        status = run_command(tokens); 
        
        free(line);
        free(tokens);
    }
}

void print_cli(void) {
    char *cwd, *user;
    char hostname[64];

    cwd = getcwd(NULL, PATH_MAX);
    user = getlogin();
    gethostname(hostname, sizeof(hostname));

    printf("%s@%s %s $ ", (user != NULL ? user : ""), hostname, cwd);

    free(cwd);
}

int run_command(char **tokens) {
    int i;

    // when user just hits enter without cmd 
    if (*tokens == NULL) {
        return 1;
    }

    for (i = 0; i < builtins_size(); ++i) {
        if (strcmp(*tokens, builtins[i].name) == 0) {
            return builtins[i].builtin(tokens);
        }      
    }

    return launch_command(tokens);
}


int launch_command(char **tokens) {
    pid_t pid, pid2;
    int i, status;

    pid = fork();

    if (pid == 0) {
        execvp(*tokens, tokens);

        fprintf(stderr, "exec error\n");
        exit(0);
    } else if (pid < 0) {
        fprintf(stderr, "fork error\n");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));     
    }

    return 1;
}


char *read_line(void) {
    char *line, *line2;
    int buf_size, c;

    buf_size = 64;
    line = line2 = (char *) malloc(sizeof(char) * buf_size);

    if (line == NULL) {
        fprintf(stderr, "memory allocation error. Terminating .. \n");
        exit(0);
    }

    c = getchar();
    while (c != EOF && c != '\n') {
        if (line2 - line == buf_size) {
            buf_size += 64;
            line = (char *) realloc(line, buf_size);
        }

        *line2++ = c;
        c = getchar();
    }

    *line2 = '\0';

    return line;
}

char **split_line(char *line) {
    char *delim, *token, **tokens, **tokens2;
    int size;

    delim = " \t\r";
    size = 32;
    tokens = tokens2 = (char **) malloc(sizeof(char *) * size);

    if (tokens == NULL) {
        fprintf(stderr, "memory allocation error. Terminating .. \n");
        exit(0);        
    }

    token = strtok(line, delim);

    while (token != NULL) {
        if (tokens2 - tokens == size) {
            size += 32;
            tokens = (char **) realloc(tokens, sizeof(char *) * size);

            if (tokens == NULL) {
                fprintf(stderr, "memory allocation error. Terminating .. \n");
                exit(0);        
            }
        }
        *tokens2++ = token; 

        token = strtok(NULL, delim);
    }

    *tokens2 = NULL;

    return tokens;
}
