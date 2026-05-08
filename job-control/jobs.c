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

job_t *job_list;
static int job_list_size = 8;
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

int add_background_job_command(execution_context_t *context) {
    job_t *job; 
    job_command_t *job_command;

    job = (job_list_index == -1) ? job_list_acquire_slot() : &job_list[job_list_index];
    job->id = job_list_index;

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

int job_complete(int job_id) {
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
    
    while (!job_complete(job_id)) {
        child_pid = waitpid(-job_list[job_id].pgid, &child_status, WUNTRACED);

        for (int j = 0; j < job_list[job_id].job_cmd_counter; ++j) {
            if (job_list[job_id].job_commands[j].pid == child_pid) {
                job_list[job_id].job_commands[j].return_value = WEXITSTATUS(child_status);
            }
        }
    }   


    if (!job_list[job_id].is_background && WIFSIGNALED(child_status)) {
        printf("\n");
    }
}

void job_control_after_launch(int job_id) {
    if (job_id == -1 || job_list[job_id].is_background) {
        return; 
    }

    tcsetpgrp(0, job_list[job_id].pgid);

    foreground_job_wait(job_id);

    tcsetpgrp(0, global_shell_pgid);
}

