#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "shelly.h"
#include "parser/parser.h"
#include "builtins/builtins.h"

static int executor(execution_context_t *context);
static int launch_command(execution_context_t *context, pid_t *pgid, int *prev_pipe_read);
static int launch_builtin(execution_context_t *context, int builtin_id, int *prev_pipe_read);

static int create_pipe(execution_context_t *context, int *pipe_fds);
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void pipe_cleanup_parent(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void parent_set_pgid(int pid, pid_t *pgid);
static void child_set_pgid(pid_t *pgid);

static void print_cli(void);
static void handle_error(int err, char *filename);

void ensure_shell_process_group(void);
void SIGINT_Handler(int sig);
void setup_signal_handlers(void);

static pid_t global_shell_pgid = 0;

void shelly_loop(void) {
    char *line;
    token_t *token_list;
    execution_context_t *context_list;
    int status, err;

    ensure_shell_process_group();

    if (tcgetpgrp(0) != global_shell_pgid) {
        // do some sleeping and check again if shell is the foreground process
        ;
    }

    setup_signal_handlers();

    status = 1;

    while (1) {
        print_cli();
        
        line = read_line(&err);    
        if (line == NULL) {
            switch (err) {
                case READ_LINE_EOF:
                    printf("\nexit\n");
                    exit(0);
                case READ_LINE_SIGINT_INTERRUPT:
                    printf("\n");
                    continue;
                default:
                    continue;
            }
        }

        token_list = tokenizer(line);
        free(line);

        context_list = get_context(token_list);

        if (context_list == NULL) {
            free_token_list(token_list);
            fprintf(stderr, "syntax error\n");     
            continue;
        }
        
        status = executor(context_list); 
        
        free_token_list(token_list);
        free_context_list(context_list);
    }
}

void SIGINT_Handler(int sig) {
    return;
}

void setup_signal_handlers(void) {
    struct sigaction sa, sa2;
    sa.sa_handler = SIGINT_Handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa2.sa_handler = SIG_IGN;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTTOU, &sa2, NULL);
}

void ensure_shell_process_group(void) {
    pid_t pgid;

    pgid = getpgid(0);

    if (pgid == -1) {
        setpgid(0, 0);
        global_shell_pgid = getpid();
    } else {
        global_shell_pgid = pgid;
    }
}

static void print_cli(void) {
    char *cwd, *user;
    char hostname[64];

    cwd = getcwd(NULL, MAX_PATH_LEN);
    user = getlogin();
    gethostname(hostname, sizeof(hostname));

    printf("%s@%s %s $ ", (user != NULL ? user : ""), hostname, cwd);

    free(cwd);
    fflush(stdout);
}

static int executor(execution_context_t *context_list) {
    int context_count, child_count, return_count, j;
    int prev_pipe_read;
    int child_status, child_pid; 
    int builtin_id;
    pid_t job_pgid;

    prev_pipe_read = child_count = return_count = child_status = 0;
    job_pgid = 0;

    context_count = context_counter(context_list);
    int return_values[context_count];

    while (context_list->type != CTX_END_TYPE) {
        builtin_id = is_builtin(*context_list->tokens);

        if (builtin_id != -1) {
            return_values[return_count++] = launch_builtin(context_list, builtin_id, &prev_pipe_read);
        } else {
            return_values[return_count++] = launch_command(context_list, &job_pgid, &prev_pipe_read);
            child_count++;
        }

        context_list++;
    }

    if (job_pgid) {
        tcsetpgrp(0, job_pgid);
    }

    // do the waiting on all spawned processes in the foreground process group
    for (j = 0; j < child_count; ++j) {
        do {
            // waitpid(-pgid, &status, WUNTRACED);
            child_pid = waitpid(-job_pgid, &child_status, 0);

        } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
        
        for (int l = 0; l < return_count; ++l) {
            if (return_values[l] == child_pid) {
                return_values[l] = WEXITSTATUS(child_status);
                break;
            }
        }
    }

    if (WIFSIGNALED(child_status)) {
        printf("\n");
    }

    // set foreground process group back to shell pid
    tcsetpgrp(0, global_shell_pgid);

    return return_values[return_count - 1];
}

static int launch_builtin(execution_context_t *context, int builtin_id, int *prev_pipe_read) {
    int pipe_fds[2];
    int stdio_fd_backup[3];
    int builtin_return;

    create_pipe(context, pipe_fds);

    stdio_fd_backup[0] = dup(0);        // backup STDIN
    stdio_fd_backup[1] = dup(1);        // backup STDOUT
    stdio_fd_backup[2] = dup(2);        // backup STDERR

    if(manipulate_fds(context, pipe_fds, prev_pipe_read) < 0) {
        return 1;
    }

    builtin_return = builtins[builtin_id].builtin(context->tokens);
    
    dup2(stdio_fd_backup[0], 0);        // restore STDIN
    dup2(stdio_fd_backup[1], 1);        // restore STDOUT
    dup2(stdio_fd_backup[2], 2);        // restore STDERR

    if (builtin_return != 0) {
        pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);

        return 1;
    }

    pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);

    return 0;
}

static int launch_command(execution_context_t *context, pid_t *job_pgid, int *prev_pipe_read) {
    pid_t pid;
    int pipe_fds[2];

    create_pipe(context, pipe_fds);
    pid = fork();

    if (pid == 0) {
        if(manipulate_fds(context, pipe_fds, prev_pipe_read) < 0) {
            exit(1);
        }

        child_set_pgid(job_pgid);

        execvp(*context->tokens, context->tokens);

        fprintf(stderr, "exec error\n");
        exit(1);
    } else if (pid < 0) {
        fprintf(stderr, "fork error\n");
    } else {
        // set the pgid variable as parent too in order to prevent race conditions
        parent_set_pgid(pid, job_pgid);

        pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);
    }

    return pid;
}

static int create_pipe(execution_context_t *context, int *pipe_fds) {
    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE && pipe(pipe_fds) == -1) {
        return -1;
    } 

    return 0;
}

static void child_set_pgid(pid_t *pgid) {
    if (*pgid == 0) {
        setpgid(0, 0);
    } else {
        setpgid(0, *pgid);
    }
}

static void parent_set_pgid(int pid, pid_t *pgid) {
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

    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
        close(pipe_fds[1]);
        *prev_pipe_read = pipe_fds[0];
    }
}

static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
    int fd;
    mode_t mode;
    mode = 0644;
    
    if ((context->flags & REDIR_IN ) == REDIR_IN) {
        fd = open(context->input_file, O_RDONLY);
        
        if (fd == -1) {
            handle_error(errno, context->input_file);
            return -1;
        }
        dup2(fd, 0);   
    }

    if ((context->flags & REDIR_OUT ) == REDIR_OUT) {
        fd = open(context->output_file, (O_WRONLY | O_CREAT | O_TRUNC), mode);

        if (fd == -1) {
            handle_error(errno, context->output_file);
            return -1;
        }
        dup2(fd, 1);   
    }

    if ((context->flags & REDIR_APPEND) == REDIR_APPEND) {
        fd = open(context->append_file, (O_WRONLY | O_CREAT | O_APPEND), mode);
        
        if (fd == -1) {
            handle_error(errno, context->append_file);             
            return -1;
        }
        dup2(fd, 1);   
    }

    if ((context->flags & REDIR_OUT_OF_PIPE) == REDIR_OUT_OF_PIPE) {
        dup2(*prev_pipe_read, 0);
    }

    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
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
