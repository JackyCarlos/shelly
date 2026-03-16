#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shelly.h"

static void print_cli(void);

static int run_command(execution_context_t *context);
static int launch_command(execution_context_t *context);

static void free_context_list(execution_context_t *context_list);
static void free_token_list(token_t *token_list);


void shelly_loop(void) {
    char *line;
    token_t *token_list;
    execution_context_t *context_list;
    int status;

    status = 1;

    while (status) {
        print_cli();
        
        line = read_line();
        token_list = tokenizer(line);
        context_list = get_context(token_list);
        
        status = run_command(context_list); 
        
        free(line);
        free_token_list(token_list);
        free_context_list(context_list);
    }
}

static void free_context_list(execution_context_t *context_list) {
    execution_context_t *context;
    for (context = context_list; context->type != CONTEXT_END_TYPE; context++) {
        free(context->tokens);
    }

    free(context_list);
}

static void free_token_list(token_t *token_list) {
    token_t *token;
    for (token = token_list; token->type != NULL_TYPE; token++) {
        free(token->str);
    }
    
    free(token_list);
}

static void print_cli(void) {
    char *cwd, *user;
    char hostname[64];

    cwd = getcwd(NULL, MAX_PATH_LEN);
    user = getlogin();
    gethostname(hostname, sizeof(hostname));

    printf("%s@%s %s $ ", (user != NULL ? user : ""), hostname, cwd);

    free(cwd);
}

static int run_command(execution_context_t *context_list) {
    int i;

    // when user just hits enter without cmd 
    if (context_list->type == CONTEXT_END_TYPE) {
        printf("test");
        return 1;
    }

/*     for (i = 0; i < builtins_size(); ++i) {
        if (strcmp(*context->tokens, builtins[i].name) == 0) {
            return builtins[i].builtin(context->tokens);
        }      
    } */

    return launch_command(context_list);
}

static int launch_command(execution_context_t *context) {
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

