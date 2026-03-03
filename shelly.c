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
    pid_t pid;
    int i, status;

    pid = fork();

    if (pid == 0) {
        if (context->output_file) {
            FILE *fptr;

            fptr = fopen(context->output_file, "w");
            dup2(fileno(fptr), 1);
        }

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
            execution_context->output_file = strtok(NULL, delim);
        } else if (strcmp(token, ">>") == 0) {
            execution_context->append_file = strtok(NULL, delim);
        } else if (strcmp(token, "<") == 0) {
            execution_context->input_file = strtok(NULL, delim);
        } else {
            *tokens2++ = token;             
        }

        token = strtok(NULL, delim);
    }

    *tokens2 = NULL;
    execution_context->tokens = tokens;

    return execution_context;
}

enum tokenizer_status {
    START,
    WORD,
    REDIRECT_IN,
    REDIRECT_PIPE,
    REDIRECT_OUT,
    REDIRECT_APPEND
};

token_t *get_tokens(char *line) {
    token_t *token_list;
    int size, i, c;
    tokenizer_status status;

    size = 32;
    token_list = (token_t *) malloc(sizeof(token_t) * size);



    i = 0;
    status = START;




    int in_word = -1;


    
    c = getchar();
    while (c != EOF) {
        if (i >= size - 1) {
            size += 32;

            token_list = realloc(token_list, sizeof(token_t) * size);
        }

        switch (c) {
            case '<':
                if (status == START) {
                    return NULL;
                } else if (status == WORD) {
                    // add null byte
                    token_list[i].str[token_list[i].index] = '\0';

                    i++;
                    token_list[i].str[0] = '<';
                    status = REDIRECT_IN;
                    in_word = 1;
                } else if (status == REDIRECT_IN || status == REDIRECT_OUT || status == REDIRECT_PIPE || status == REDIRECT_APPEND) {
                    return NULL;
                } break;

            case '|':
                if (status == START) {
                    return NULL;
                } else if (status == WORD) {
                    // add null byte
                    token_list[i].str[token_list[i].index] = '\0';

                    i++;
                    token_list[i].str[0] = '|';
                    status = REDIRECT_PIPE;
                    in_word = 1;
                } else if (status == REDIRECT_IN || status == REDIRECT_OUT || status == REDIRECT_PIPE || status == REDIRECT_APPEND) {
                    return NULL;
                } break;

            case '>':
                if (status == START) {
                    return NULL;
                } else if (status == WORD) {
                    // add null byte
                    token_list[i].str[token_list[i].index] = '\0';

                    i++;
                    token_list[i].str[0] = '>';
                    status = REDIRECT_PIPE;
                    in_word = 1;
                } else if (status == REDIRECT_IN || status == REDIRECT_PIPE || status == REDIRECT_APPEND) {
                    return NULL;
                } else if (status == REDIRECT_OUT && in_word) {
                    token_list[i].str[1] = '>';
                    status = REDIRECT_APPEND;
                } else if (status == REDIRECT_OUT && !in_word) {

                } break;   

            case ' ':
                if (status != START && in_word == 1) {
                    in_word = 0;
                }
                break;

            default:
                if (status != WORD && status != START || !in_word) {
                    i++; 
                } 
                status = WORD;
                in_word = 1;
                token_list[i].str[token_list[i].index++] = c;
        };


        c = getchar();
    }




}



// typedef struct {
//     char **tokens;
//     char *input_file;
//     char *output_file;
//     char *append_file;
// } execution_context_t;