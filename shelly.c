#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "shelly.h"

static void print_cli(void);

static int run_command(execution_context_t *context);
static int launch_command(execution_context_t *context);
static int create_pipe(execution_context_t *context, int *pipe_fds);
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int prev_pipe_read);

static void free_context_list(execution_context_t *context_list);
static void free_token_list(token_t *token_list);

static void handle_error(int err, char *filename);


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

        if (context_list == NULL) {
            fprintf(stderr, "syntax error\n");
            continue;
        }
        
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

    while (context_list->type != CONTEXT_END_TYPE) {
        launch_command(context_list);
        context_list++;
    }

/*     for (i = 0; i < builtins_size(); ++i) {
        if (strcmp(*context->tokens, builtins[i].name) == 0) {
            return builtins[i].builtin(context->tokens);
        }      
    } */

    return 1;
}

static int launch_command(execution_context_t *context) {
    pid_t pid;
    int i, status;
    int pipe_fds[2];
    static int prev_read_end = 0;

    create_pipe(context, pipe_fds);
    pid = fork();

    if (pid == 0) {
        if(manipulate_fds(context, pipe_fds, prev_read_end) < 0) {
            return 0;
        }

        execvp(*context->tokens, context->tokens);

        fprintf(stderr, "exec error\n");
        exit(0);
    } else if (pid < 0) {
        fprintf(stderr, "fork error\n");
    } else {
        if (prev_read_end) {
            close(prev_read_end);
        }

        if ((context->flags & INTO_PIPE) == INTO_PIPE) {
            close(pipe_fds[1]);
            prev_read_end = pipe_fds[0];
        }
        
        // do {
        //     waitpid(pid, &status, WUNTRACED);
        // } while (!WIFEXITED(status) && !WIFSIGNALED(status));     
    }

    return pid;
}

static int create_pipe(execution_context_t *context, int *pipe_fds) {
    if ((context->flags & INTO_PIPE) == INTO_PIPE && pipe(pipe_fds) == -1) {
        return -1;
    } 

    return 0;
}

static int manipulate_fds(execution_context_t *context, int *pipe_fds, int prev_pipe_read) {
    int fd;
    mode_t mode;
    mode = 0644;
    
    if ((context->flags & IN ) == IN) {
        fd = open(context->input_file, O_RDONLY);
        
        if (fd == -1) {
            handle_error(errno, context->input_file);
            return -1;
        }
        dup2(fd, 0);   
    }

    if ((context->flags & OUT ) == OUT) {
        fd = open(context->output_file, (O_WRONLY | O_CREAT | O_TRUNC), mode);

        if (fd == -1) {
            handle_error(errno, context->output_file);
            return -1;
        }
        dup2(fd, 1);   
    }

    if ((context->flags & APPEND) == APPEND) {
        fd = open(context->append_file, (O_WRONLY | O_CREAT | O_APPEND), mode);
        
        if (fd == -1) {
            handle_error(errno, context->append_file);             
            return -1;
        }
        dup2(fd, 1);   
    }

    if ((context->flags & OUT_OF_PIPE) == OUT_OF_PIPE) {
        dup2(prev_pipe_read, 0);
    }

    if ((context->flags & INTO_PIPE) == INTO_PIPE) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], 1);
    }

    return 0;
}

static void handle_error(int err, char *filename) {
    switch (errno) {
        case(ENOENT):
            fprintf(stderr, "shelly: no such file or directory: %s\n", filename);
            break;
        case(EACCES):
            fprintf(stderr, "shelly: permission denied: %s\n", filename);
            break;
        case(EISDIR):
            fprintf(stderr, "shelly: permission denied: %s\n", filename);
            break;              
    }
}
