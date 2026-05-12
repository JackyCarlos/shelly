#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "../shelly.h"
#include "../parser/parser.h"
#include "../builtins/builtins.h"
#include "../job-control/jobs.h"

static int launch_command(execution_context_t *context, pid_t *pgid, int *prev_pipe_read);
static int launch_builtin(execution_context_t *context, int builtin_id, int *prev_pipe_read);

static int create_pipe(execution_context_t *context, int *pipe_fds);
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void pipe_cleanup_parent(execution_context_t *context, int *pipe_fds, int *prev_pipe_read);
static void parent_set_pgid(int pid, pid_t *pgid);
static void child_set_pgid(pid_t *pgid);
static void handle_error(int err, char *filename);

job_t *executor(execution_context_t *context_list) {
    int prev_pipe_read;
    int builtin_id, is_builtin_cmd;
    pid_t job_pgid;
    int child_pid;
    int builtin_return;
    job_t *job;

    prev_pipe_read = 0;
    job_pgid = 0;

    if (context_list->type == CTX_END_TYPE) { 
        return NULL; 
    }

    while (context_list->type != CTX_END_TYPE) {
        job = job_control_get_job(context_list);
        job_control_add_job_command(job, context_list);

        is_builtin_cmd = is_builtin(*context_list->tokens, &builtin_id);

        if (!is_builtin_cmd) {
            child_pid = launch_command(context_list, &job_pgid, &prev_pipe_read);

            job_control_set_pgid(job, job_pgid);
            job_control_set_command_pid(job, child_pid);
        } else {
            builtin_return = launch_builtin(context_list, builtin_id, &prev_pipe_read);
            job_control_set_builtin_returnval(job, builtin_return);      
        }

        if (context_list->pipeline_end) {
            job_control_register_background_job(job, context_list->is_background);
            job_pgid = 0;
        }
    
        context_list++;
    }

    return job;
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
        if (is_builtin(*context->tokens, NULL) == -1) {
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
