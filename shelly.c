#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "shelly.h"

static int executor(execution_context_t *context);
static int launch_command(execution_context_t *context, int *pgid, int *prev_pipe_read);
static int launch_builtin(execution_context_t *context, int builtin_id, int *prev_pipe_read);

static int create_pipe(execution_context_t *context, int *pipe_fds);
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void pipe_cleanup_parent(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void parent_set_pgid(int pid, int *pgid);
static void child_set_pgid(int *pgid);

static void print_cli(void);
static void handle_error(int err, char *filename);

static void free_context_list(execution_context_t *context_list);
static void free_token_list(token_t *token_list);

void shelly_loop(void) {
    char *line;
    token_t *token_list;
    execution_context_t *context_list;
    int status;

    status = 1;

    while (1) {
        print_cli();
        
        line = read_line();
        token_list = tokenizer(line);
        context_list = get_context(token_list);

        if (context_list == NULL) {
            fprintf(stderr, "syntax error\n");
            continue;
        }
        
        status = executor(context_list); 
        
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

static int executor(execution_context_t *context_list) {
    int context_count, child_count, j;
    int pgid, prev_pipe_read;
    int child_status, child_pid; 
    int builtin_id;

    pgid = prev_pipe_read = child_count = 0;
    context_count = context_counter(context_list);
    int child_pids[context_count];

    while (context_list->type != CONTEXT_END_TYPE) {
        builtin_id = is_builtin(*context_list->tokens);

        if (builtin_id != -1) {
            launch_builtin(context_list, builtin_id, &prev_pipe_read);
        } else {
            child_pids[child_count++] = launch_command(context_list, &pgid, &prev_pipe_read);
        }

        context_list++;
    }

    // do the waiting on all spawned processes
    for (j = 0; j < child_count; ++j) {
        do {
            // waitpid(-pgid, &status, WUNTRACED);
            child_pid = waitpid(-pgid, &child_status, 0);
        } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));   
        
        for (int l = 0; l < child_count; ++l) {
            if (child_pids[l] == child_pid) {
                child_pids[l] = WEXITSTATUS(child_status);
                break;
            }
        }
    }



    return child_pids[child_count - 1];
}

static int launch_builtin(execution_context_t *context, int builtin_id, int *prev_pipe_read) {
    int pipe_fds[2];
    int fd_backup[2];

    create_pipe(context, pipe_fds);
    fd_backup[0] = dup(0);      // backup STDIN
    fd_backup[1] = dup(1);      // backup STDOUT

    if(manipulate_fds(context, pipe_fds, prev_pipe_read) < 0) {
        return 1;
    }

    if (builtins[builtin_id].builtin(context->tokens) != 0) {
        return 1;
    }

    pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);

    dup2(fd_backup[0], 0);      // restore STDIN
    dup2(fd_backup[1], 1);      // restore STDOUT

    return 0;
}

static int launch_command(execution_context_t *context, int *pgid, int *prev_pipe_read) {
    pid_t pid;
    int pipe_fds[2];

    create_pipe(context, pipe_fds);
    pid = fork();

    if (pid == 0) {
        if(manipulate_fds(context, pipe_fds, prev_pipe_read) < 0) {
            exit(1);
        }

        child_set_pgid(pgid);

        execvp(*context->tokens, context->tokens);

        fprintf(stderr, "exec error\n");
        exit(1);
    } else if (pid < 0) {
        fprintf(stderr, "fork error\n");
    } else {
        // set the pgid variable as parent too in order to prevent race conditions
        parent_set_pgid(pid, pgid);

        pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);
    }

    return pid;
}

static int create_pipe(execution_context_t *context, int *pipe_fds) {
    if ((context->flags & INTO_PIPE) == INTO_PIPE && pipe(pipe_fds) == -1) {
        return -1;
    } 

    return 0;
}

static void child_set_pgid(int *pgid) {
    if (*pgid == 0) {
        setpgid(0, 0);
    } else {
        setpgid(0, *pgid);
    }
}

static void parent_set_pgid(int pid, int *pgid) {
    if (*pgid == 0) {
        *pgid = pid;
    }

    setpgid(pid, *pgid);
}

static void pipe_cleanup_parent(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
    // close read fds to previous pipe 
    if (*prev_pipe_read) {
        close(*prev_pipe_read);
    }

    if ((context->flags & INTO_PIPE) == INTO_PIPE) {
        close(pipe_fds[1]);
        *prev_pipe_read = pipe_fds[0];
    }
}

static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
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
        dup2(*prev_pipe_read, 0);
    }

    if ((context->flags & INTO_PIPE) == INTO_PIPE) {
        if (is_builtin(*context->tokens) == -1) {
            close(pipe_fds[0]);
        }
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
