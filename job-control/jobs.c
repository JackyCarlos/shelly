#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"
#include "../parser/parser.h"

static job_t *job_list_acquire_slot(void);
static void copy_tokens(execution_context_t *context, job_command_t *job);
static void copy_io_files(execution_context_t *context, job_command_t *jobs);
static void init_jobs(int start_index);
static int job_complete(int job_id);
static void cleanup_job(int job_id);

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

        job_list[j].job_commands = malloc(job_list[i].job_cmds_size * sizeof(job_command_t));

        if (job_list[j].job_commands == NULL) { goto alloc_err; }
    }

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);    
}

static job_t *job_list_acquire_slot(void) {
    int i;

    i = 0;

    while (i != job_list_size) {
        if (job_list[i].id == -1) {
            job_list_index = i;

            return &job_list[i];
        }

        i++;
    }

    job_list_size += 8;
    job_list = realloc(job_list, job_list_size * sizeof(job_t));
    job_list_index = i;

    init_jobs(i);

    return &job_list[i];
}

int job_control_get_jobid(execution_context_t *context) {
    job_t *job; 

    job = (job_list_index == -1) ? job_list_acquire_slot() : &job_list[job_list_index];
    job->id = job_list_index;
    
    return job->id;
}

int job_control_add_job_command(int job_id, execution_context_t *context) {
    job_t *job;
    job_command_t *job_command;

    job = &job_list[job_id];

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
    job_command->job_stat = RUNNING;
    
    if (context->pipeline_end) {
        job_list_index = -1; 
    }

    return job->id;

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

static int job_complete(int job_id) {
    int i;

    for (i = 0; i < job_list[job_id].job_cmd_counter; ++i) {
        if (job_list[job_id].job_commands[i].return_value == -1) {
            return 0;
        } 
    }

    return 1;
}

void foreground_job_wait(int job_id) {
    int child_pid;
    int child_status;
    job_command_t *job_command;

    child_status = 0;
    
    while (!job_complete(job_id)) {
        child_pid = waitpid(-job_list[job_id].pgid, &child_status, WUNTRACED);

        for (int j = 0; j < job_list[job_id].job_cmd_counter; ++j) {
            job_command = &job_list[job_id].job_commands[j];

            if (job_command->pid == child_pid) {
                job_command->return_value = WEXITSTATUS(child_status);
                job_command->job_stat = TERMINATED;
            }
        }
    }   


    if (!job_list[job_id].is_background && WIFSIGNALED(child_status)) {
        printf("\n");
    }
}

int job_control_after_launch(int job_id) {
    int return_value;
    
    if (job_id == -1 || job_list[job_id].is_background) {
        return -1; 
    }

    tcsetpgrp(0, job_list[job_id].pgid);

    foreground_job_wait(job_id);

    if (job_complete(job_id)) {
        return_value = job_list[job_id].job_commands[job_list[job_id].job_cmd_counter - 1].return_value;
        cleanup_job(job_id);
    }

    tcsetpgrp(0, global_shell_pgid);

    return return_value;
}

void job_control_set_pgid(int job_id, pid_t job_pgid) {
    job_list[job_id].pgid = job_pgid; 
}

void job_control_set_command_pid(int job_id, int pid) {
    int job_cmd_index = job_list[job_id].job_cmd_counter - 1;
    job_list[job_id].job_commands[job_cmd_index].pid = pid;   
}

void job_control_set_builtin_returnval(int job_id, int builtin_return) {
    job_list[job_id].job_commands[job_list[job_id].job_cmd_counter - 1].return_value = builtin_return;
}

void job_control_register_background_job(int job_id, int is_background) {
    if (is_background) {
        printf("[%d] ", job_id + 1);

        for (int i = 0; i < job_list[job_id].job_cmd_counter; ++i) {
            printf("%d ", job_list[job_id].job_commands[i].pid);
        }

        printf("\n");
    }        
}

static void cleanup_job(int job_id) {
    job_t *job;
    job_command_t *job_command;
    int i, j;

    job = &job_list[job_id];
    job->id = -1;


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
    }
    
    job->job_cmd_counter = 0;
}
