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

void shelly_loop(void) {
    char *line;
    execution_context_t *context;
    int status;

    status = 1;

    while (status) {
        print_cli();
        
        line = read_line();
        //context = parse_line(line);
        
        status = run_command(context); 
        
        free(line);
        free(context->tokens);
        free(context);
    }
}

void print_cli(void) {
    char *cwd, *user;
    char hostname[64];

    cwd = getcwd(NULL, MAX_PATH_LEN);
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

execution_context_t *get_context(token_t *token_list) {
    execution_context_t *contexts;
    int i;
    context_status status;
    int tokens_index;   // index in the token array of a single context 
    int tokens_size;    // size of the char * array of a single context

    // replace 32 with symbolic constant 
    contexts = (execution_context_t *) malloc(sizeof(execution_context_t) * 32);
    for (int j; j < 32; ++j) {
        contexts[j].tokens_index = 0;
    }
    
    i = 0;

    contexts[0].status = CONTEXT_END_TYPE;
    status = STATUS_CONTEXT_INIT;

    while (token_list->type != NULL_TYPE) {

        switch (token_list->type) {
            case REDIRECT_OUT_TYPE:
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_OUT;
                break;

            case REDIRECT_IN_TYPE:
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_IN;
                break;

            case REDIRECT_PIPE_TYPE: 
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }
                contexts[i].flags |= INTO_PIPE;
                i++;
                status = STATUS_CONTEXT_PIPE;
                break;

            case REDIRECT_APPEND_TYPE: 
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_APPEND;
                break;

            default:
                if (status == STATUS_CONTEXT_REDIRECT_OUT) {
                    contexts[i].output_file = token_list->str;
                    contexts[i].flags |= OUT;

                } else if (status == STATUS_CONTEXT_REDIRECT_IN) {
                    contexts[i].input_file = token_list->str;
                    contexts[i].flags |= IN;

                } else if (status == STATUS_CONTEXT_REDIRECT_APPEND) {
                    contexts[i].append_file = token_list->str;
                    contexts[i].flags |= APPEND;

                } else if (status == STATUS_CONTEXT_PIPE) {
                    contexts[i].flags |= OUT_OF_PIPE;

                } else {
                    tokens_index = contexts[i].tokens_index;

                    if (tokens_index == 0) {
                        contexts[i].status = CONTEXT_COMMAND_TYPE;
                        contexts[i + 1].status = CONTEXT_END_TYPE;

                        tokens_size = 8;
                        contexts[i].tokens = malloc(sizeof(char *) * tokens_size);
                    } // left to do check for tokens_index - 1 == tokens_size and reallocate if necessary

                    contexts[i].tokens[tokens_index++] = token_list->str;
                    contexts[i].tokens[tokens_index] = NULL;
                    contexts[i].tokens_index++;
                }

                status = STATUS_CONTEXT_WORD;
        }
        
        token_list++;
    } 
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

token_t *tokenizer(char *line) {
    token_t *token_list;
    int token_array_size, token_buf_size;
    int i, j, str_index;
    tokenizer_status status;

    token_array_size = 32;
    token_list = (token_t *) malloc(sizeof(token_t) * token_array_size);

    i = -1;
    status = STATUS_TOKENIZER_WORDOUT;

    while (*line != '\0') {
        if (i == token_array_size - 2) {
            token_array_size += 32;

            token_list = realloc(token_list, sizeof(token_t) * token_array_size);

            if (token_list == NULL) {
                goto err;
            }
        }

        switch (*line) {
            case '<':
                i++;
                token_list[i].type = REDIRECT_IN_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '|':
                i++;
                token_list[i].type = REDIRECT_PIPE_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '>':
                if (status == STATUS_TOKENIZER_REDIRECT_OUT) {
                    token_list[i].type = REDIRECT_APPEND_TYPE;
                    status = STATUS_TOKENIZER_WORDOUT;
                } else {
                    i++;
                    token_list[i].type = REDIRECT_OUT_TYPE;
                    token_list[i].str = NULL;
                    status = STATUS_TOKENIZER_REDIRECT_OUT;
                } 
                break;

            case ' ':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '\r':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '\t':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            default:
                if (status == STATUS_TOKENIZER_WORDOUT || status == STATUS_TOKENIZER_REDIRECT_OUT) {
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

                str_index = token_list[i].index++;
                token_list[i].str[str_index] = *line;
                str_index++;
                token_list[i].str[str_index] = '\0';

                status = STATUS_TOKENIZER_WORDIN; 
        };

        line++;
    }

    token_list[i + 1].type = NULL_TYPE;
    return token_list;

    err:
        fprintf(stderr, "memory allocation error\n");
        
        for (j = 0; j < i; ++j) {       
            free(token_list[j].str);
        }

        free(token_list);
        return NULL;
}
