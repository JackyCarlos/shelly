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
token_t *get_tokens(char *line); 
token_t *tokenizer(char *line);

int main(int argc, char *argv[]) {
    // shelly_loop();

    token_t *token_list;
    char *line;

    line = read_line();
    printf("line:::: %s\n", line);
    token_list = tokenizer(line); // get_tokens(line);

    if (token_list == NULL) {
        fprintf(stderr, "parsing error\n");
        return 0;
    }

    while (token_list->type != NULL_TYPE) {
        printf("token: %s, type:%d\n", token_list->str, token_list->type);
        token_list++;
    }
    
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
            line = line2 = (char *) realloc(line, buf_size);
            line2 += (buf_size - 64);
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



token_t *tokenizer(char *line) {
    token_t *token_list;
    int token_array_size, token_buf_size;
    int i, j, str_index;
    tokenizer_status status;

    token_array_size = 32;
    token_list = (token_t *) malloc(sizeof(token_t) * token_array_size);

    i = -1;
    status = STATUS_WORDOUT;

    while (*line != '\0') {
        if (i >= token_array_size - 2) {
            token_array_size += 32;

            token_list = realloc(token_list, sizeof(token_t) * token_array_size);

            if (token_list == NULL) {
                goto err;
            }
        }

        switch (*line) {
            case '<':
                i++;
                token_list[i].type = REDIRECT_IN;
                token_list[i].str = NULL;
                status = STATUS_WORDOUT;
                break;

            case '|':
                i++;
                token_list[i].type = REDIRECT_PIPE;
                token_list[i].str = NULL;
                status = STATUS_WORDOUT;
                break;

            case '>':
                if (status == STATUS_REDIRECT_OUT) {
                    token_list[i].type = REDIRECT_APPEND;
                    status = STATUS_WORDOUT;
                } else {
                    i++;
                    token_list[i].type = REDIRECT_OUT;
                    token_list[i].str = NULL;
                    status = STATUS_REDIRECT_OUT;
                } 
                break;

            case ' ':
                status = STATUS_WORDOUT;
                break;

            case '\r':
                status = STATUS_WORDOUT;
                break;

            case '\t':
                status = STATUS_WORDOUT;
                break;

            default:
                if (status == STATUS_WORDOUT || status == STATUS_REDIRECT_OUT) {
                    i++;
                    token_buf_size = 32;

                    token_list[i].str = (char *) malloc(token_buf_size);
                    if (token_list[i].str == NULL) {
                        goto err;
                    }

                    token_list[i].type = WORD_TYPE;
                    token_list[i].index = 0;
                }

                if (token_list[i].index >= token_buf_size - 1) {
                    token_buf_size += 32;
                    token_list[i].str = (char *) realloc(token_list[i].str, token_buf_size);

                    if (token_list[i].str == NULL) {
                        goto err;
                    }
                }

                str_index = token_list[i].index;
                token_list[i].str[str_index] = *line;
                token_list[i].index += 1;
                str_index = token_list[i].index;
                token_list[i].str[str_index] = '\0';

                status = STATUS_WORDIN; 
        };

        line++;
    }

    token_list[++i].type = NULL_TYPE;
    return token_list;

    err:
        fprintf(stderr, "memory allocation error\n");
        
        for (j = 0; j < i; ++j) {       
            free(token_list[j].str);
        }

        free(token_list);
        return NULL;
}
