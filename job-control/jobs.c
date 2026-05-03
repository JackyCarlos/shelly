#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jobs.h"
#include "../builtins/builtins.h"
#include "../parser/parser.h"

job_t *job_list_acquire_slot(void);
static void copy_tokens(execution_context_t *context, job_t *job);
static void copy_io_files(execution_context_t *context, job_t *jobs);

static job_t *job_list;
static int job_list_size = 8;
static int job_list_index = -1;

void init_job_control(void) {
    int i;

    job_list = malloc(job_list_size * sizeof(job_t));

    if (job_list == NULL) { goto alloc_err; }

    for (i = 0; i < 8; ++i) {
        job_list[i].id = -1;
        job_list[i].job_cmd_counter = 0;
        job_list[i].job_cmds_size = 8;

        job_list[i].job_commands = malloc(job_list[i].job_cmds_size * sizeof(job_command_t));

        if (job_list[i].job_commands == NULL) { goto alloc_err; }
    }

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);    
}

job_t *job_list_acquire_slot(void) {
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

    // initialize job slots with default values. create separate function for this 

    return &job_list[i];
}

void add_background_job_command(execution_context_t *context) {
    job_t *job; 

    job = (job_list_index == -1) ? job_list_acquire_slot() : &job_list[job_list_index];
    job->id = job_list_index;

    // do we need more space for more job_command_t's? 
    if (job->job_cmd_counter == job->job_cmds_size) {
        job->job_cmds_size += 8;
        job->job_commands = realloc(job->job_commands, job->job_cmds_size * sizeof(job_command_t));

        if (job->job_commands == NULL) { goto alloc_err; }
    }

    copy_tokens(context, job);
    copy_io_files(context, job);
    job->job_cmd_counter++;

    if (context->pipeline_end) {
        job_list_index = -1; 
    }

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_tokens(execution_context_t *context, job_t *job) {
    int i;
    char *copy;

    job->job_commands[job->job_cmd_counter].tokens = malloc((context->argc + 1) * sizeof(char *));
    if (job->job_commands[job->job_cmd_counter].tokens == NULL) { goto alloc_err; } 

    for (i = 0; i < context->argc; ++i) {
        copy = strdup(context->tokens[i]);

        if (copy == NULL) { goto alloc_err; } 

        job->job_commands[job->job_cmd_counter].tokens[i] = copy;
    }

    job->job_commands[job->job_cmd_counter].tokens[context->argc] = NULL;
    job->job_commands[job->job_cmd_counter].argc = context->argc;

    return;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_io_files(execution_context_t *context, job_t *job) {
    job_command_t *job_cmd;
    char *temp, *temp2, *temp3;

    job_cmd = &job->job_commands[job->job_cmd_counter];

    if ((context->flags & REDIR_IN) == REDIR_IN) {
        temp = strdup(context->input_file);
    }

    if ((context->flags & REDIR_OUT) == REDIR_OUT) {
        temp2 = strdup(context->output_file);
    }

    if ((context->flags & REDIR_APPEND) == REDIR_APPEND) {
        temp3 = strdup(context->append_file);
    }
    
    if (temp == NULL || temp2 == NULL || temp3 == NULL) {
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
    }

    job_cmd->input_file  = temp;
    job_cmd->output_file = temp2;
    job_cmd->append_file = temp3;
}
