#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"
#include "../parser/parser.h"

static int job_list_acquire_slot(void);
static void copy_tokens(execution_context_t *context, job_command_t *job);
static void copy_io_files(execution_context_t *context, job_command_t *jobs);
static void init_jobs(int start_index);
static int job_complete(job_t *job);
static int job_running(job_t *job);
static void cleanup_job(job_t *job);
static void job_control_find_by_pid(int child_pid, job_t **job_out, job_command_t **job_command_out);

job_t *job_list;
int job_list_size = 8;
static int job_list_index = -1;

void init_job_control(void) {
    job_list = malloc(job_list_size * sizeof(job_t));

    if (job_list == NULL) { goto alloc_err; }

    init_jobs(0);

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);    
}

static void init_jobs(int start_index) {
    int i, j;

    for (i = 0; i < 8; ++i) {
        j = start_index + i;

        job_list[j].id = -1;
        job_list[j].job_cmd_counter = 0;
        job_list[j].job_cmds_size = 8;
        job_list[j].pgid = -1;

        job_list[j].job_commands = malloc(job_list[i].job_cmds_size * sizeof(job_command_t));

        if (job_list[j].job_commands == NULL) { goto alloc_err; }
    }

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);    
}

static int job_list_acquire_slot(void) {
    int i;

    i = 0;

    while (i != job_list_size) {
        if (job_list[i].id == -1) {
            job_list_index = i;

            return i;
        }

        i++;
    }

    job_list_size += 8;
    job_list = realloc(job_list, job_list_size * sizeof(job_t));
    job_list_index = i;

    init_jobs(i);

    return i;
}

job_t *job_control_get_job(execution_context_t *context) {
    int job_id;
    job_t *job;

    job_id = (job_list_index == -1) ? job_list_acquire_slot() : job_list_index;
    job_list_index = (context->pipeline_end) ? -1 : job_id;

    job = &job_list[job_id];
    job->id = job_id;
    job->is_background = context->is_background;
    
    return job;
}

void job_control_add_job_command(job_t *job, execution_context_t *context, int is_builtin) {
    job_command_t *job_command;

    // do we need more space for more job_command_t's? 
    if (job->job_cmd_counter == job->job_cmds_size) {
        job->job_cmds_size += 8;
        job->job_commands = realloc(job->job_commands, job->job_cmds_size * sizeof(job_command_t));

        if (job->job_commands == NULL) { goto alloc_err; }
    }

    job_command = &job->job_commands[job->job_cmd_counter];
    job->job_cmd_counter++;
    
    copy_tokens(context, job_command);
    copy_io_files(context, job_command);
    job_command->return_value = -1;
    job_command->job_stat = (is_builtin) ? TERMINATED : RUNNING; 
    
    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_tokens(execution_context_t *context, job_command_t *job_command) {
    int i;
    char *copy;

    job_command->tokens = malloc((context->argc + 1) * sizeof(char *));
    if (job_command->tokens == NULL) { goto alloc_err; } 

    for (i = 0; i < context->argc; ++i) {
        copy = strdup(context->tokens[i]);

        if (copy == NULL) { goto alloc_err; } 

        job_command->tokens[i] = copy;
    }

    job_command->tokens[context->argc] = NULL;
    job_command->argc = context->argc;

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_io_files(execution_context_t *context, job_command_t *job_command) {
    char *temp, *temp2, *temp3;

    job_command->flags = context->flags;

    if ((context->flags & REDIR_IN) == REDIR_IN) {
        temp = strdup(context->input_file);
        if (temp == NULL) { goto alloc_err; }
    }

    if ((context->flags & REDIR_OUT) == REDIR_OUT) {
        temp2 = strdup(context->output_file);
        if (temp2 == NULL) { goto alloc_err; }
    }

    if ((context->flags & REDIR_APPEND) == REDIR_APPEND) {
        temp3 = strdup(context->append_file);
        if (temp3 == NULL) { goto alloc_err; }
    }

    job_command->input_file  = temp;
    job_command->output_file = temp2;
    job_command->append_file = temp3;

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static int job_complete(job_t *job) {
    int i;

    for (i = 0; i < job->job_cmd_counter; ++i) {
        if (job->job_commands[i].job_stat != TERMINATED) {
            return 0;
        } 
    }
 
    return 1;
}

static int job_running(job_t *job) {
    int i;

    for (i = 0; i < job->job_cmd_counter; ++i) {
        if (job->job_commands[i].job_stat == RUNNING) {
            return 1;
        } 
    }
 
    return 0;
}

int job_control_after_launch(job_t *job) {
    int return_value;
    
    if (job == NULL) {
        return -1; 
    }

    if (job->pgid == -1) {
        return_value = job->job_commands[job->job_cmd_counter - 1].return_value;
        cleanup_job(job);
        return return_value;
    }

    if (job->is_background) {
        return -1;
    }

    tcsetpgrp(0, job->pgid);

    foreground_job_wait(job);

    if (job_complete(job)) {
        return_value = job->job_commands[job->job_cmd_counter - 1].return_value;
        cleanup_job(job);
    }

    tcsetpgrp(0, global_shell_pgid);

    return return_value;
}

void foreground_job_wait(job_t *job) {
    int child_pid;
    int child_status;
    job_command_t *job_command;

    child_status = 0;
    
    while (job_running(job)) {
        child_pid = waitpid(-job->pgid, &child_status, WUNTRACED);

        for (int j = 0; j < job->job_cmd_counter; ++j) {
            job_command = &job->job_commands[j];

            if (job_command->pid == child_pid) {

                if (WIFSTOPPED(child_status)) {
                    job_command->job_stat = SUSPENDED;
                    job->is_background = 1;
                } else {

                    job_command->return_value = WEXITSTATUS(child_status);

                    if (job_command->return_value == 127) {
                        job_command->job_stat = FAILURE;
                    } else {
                        job_command->job_stat = TERMINATED;
                    }
                }


            }
        }
    }   

    if (WIFSIGNALED(child_status)) {
        printf("\n");
    }
}

void reap_background_jobs(void) {
    int terminated_child, child_status;
    job_t *job;
    job_command_t *job_command;

    terminated_child = waitpid(-1, &child_status, WNOHANG);

    while (terminated_child > 0) {
        job_control_find_by_pid(terminated_child, &job, &job_command);

        job_command->return_value = WEXITSTATUS(child_status);
        if (job_command->return_value == 127) {
            job_command->job_stat = FAILURE;
        } else {
            job_command->job_stat = TERMINATED;
        }

        terminated_child = waitpid(-1, &child_status, WNOHANG);

        if (job_complete(job)) {
            printf("\r[%d]    done\r\n", job->id + 1);
            cleanup_job(job);
        }
    }
}

static void job_control_find_by_pid(int child_pid, job_t **job_out, job_command_t **job_command_out) {
    int i, j;
    job_t *job;
    job_command_t *job_command;

    for (i = 0; i < job_list_size; ++i) {
        job = &job_list[i];

        for (j = 0; j < job->job_cmd_counter; ++j) {
            job_command = &job->job_commands[j];

            if (job_command->pid == child_pid) {
                *job_out = job;
                *job_command_out = job_command;
                return;
            }
        }
    }
}

void job_control_set_pgid(job_t *job, pid_t job_pgid) {
    job->pgid = job_pgid; 
}

void job_control_set_command_pid(job_t *job, int pid) {
    int job_cmd_index = job->job_cmd_counter - 1;
    job->job_commands[job_cmd_index].pid = pid;   
}

void job_control_set_builtin_returnval(job_t *job, int builtin_return) {
    job->job_commands[job->job_cmd_counter - 1].return_value = builtin_return;
}

void job_control_register_background_job(job_t *job, int is_background) {
    if (is_background) {
        printf("[%d] ", job->id + 1);

        for (int i = 0; i < job->job_cmd_counter; ++i) {
            printf("%d ", job->job_commands[i].pid);
        }

        printf("\n");
    }        
}

static void cleanup_job(job_t *job) {
    job_command_t *job_command;
    int i, j;

    job->id = job->pgid = -1;

    for (i = 0; i < job->job_cmd_counter; ++i) {
        job_command = &job->job_commands[i];
  
        if ((job_command->flags & REDIR_IN) == REDIR_IN) {
            free(job_command->input_file);
        }

        if ((job_command->flags & REDIR_OUT) == REDIR_OUT) {
            free(job_command->output_file);
        }

        if ((job_command->flags & REDIR_APPEND) == REDIR_APPEND) {
            free(job_command->append_file);
        }

        for (j = 0; j < job_command->argc; ++j) {
            free(job_command->tokens[j]);
        }

        free(job_command->tokens);

        job_command->return_value = -1;
        job_command->pid = 0;
    }
    
    job->job_cmd_counter = 0;
}
