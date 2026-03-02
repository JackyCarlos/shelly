#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shelly.h"

void shelly_loop(void);
void print_cli(void);
char *read_line(void);
execution_context_t *parse_line(char *line);
int run_command(execution_context_t *context);
int launch_command(execution_context_t *context);

int main(int argc, char *argv[]) {
    shelly_loop();
    
    return 0;
}

void shelly_loop(void) {
    char *line;
    execution_context_t *context;
    int status;

    status = 1;

    while (status) {
        print_cli();
        
        line = read_line();
        context = parse_line(line);
        
        status = run_command(context); 
        
        free(line);
        free(context->tokens);
        free(context);
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

int run_command(execution_context_t *context) {
    int i;

    // when user just hits enter without cmd 
    if (context->tokens[0] == NULL) {
        return 1;
    }

    for (i = 0; i < builtins_size(); ++i) {
        if (strcmp(*context->tokens, builtins[i].name) == 0) {
            return builtins[i].builtin(context->tokens);
        }      
    }

    return launch_command(context);
}


int launch_command(execution_context_t *context) {
    pid_t pid, pid2;
    int i, status;

    pid = fork();

    if (pid == 0) {
        execvp(*context->tokens, context->tokens);

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

execution_context_t *parse_line(char *line) {
    char *delim, *token, **tokens, **tokens2;
    execution_context_t *execution_context;
    int size;

    delim = " \t\r";
    size = 32;
    tokens = tokens2 = (char **) malloc(sizeof(char *) * size);
    execution_context = malloc(sizeof(execution_context_t));

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

        if (strcmp(token, ">") == 0) {
            execution_context->input_file = strtok(NULL, delim);
        } else if (strcmp(token, ">>") == 0) {
            execution_context->output_file = strtok(NULL, delim);
        } else if (strcmp(token, "<") == 0) {
            execution_context->append_file = strtok(NULL, delim);
        } else {
            *tokens2++ = token;             
        }

        token = strtok(NULL, delim);
    }

    *tokens2 = NULL;

    execution_context->tokens = tokens;

    return execution_context;
}


// typedef struct {
//     char **tokens;
//     char *input_file;
//     char *output_file;
//     char *append_file;
// } execution_context_t;