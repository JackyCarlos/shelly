#include <stdlib.h>
#include <strings.h>

#include "jobs.h"
#include "../builtins/builtin.h"

job_t *job_list_acquire_slot(void);
static void copy_tokens(execution_context_t *context, job_t *job);
static void copy_io_files(execution_context_t *context, job_t *jobs);

static job_t *job_list;
static int job_list_size = 8;
static int job_list_index = -1;

void init_job_control(void) {
    int i; 

    job_list = malloc(job_list_size * sizeof(job_t));
    // add error checking

    for (i = 0; i < 8; ++i) {
        job_list[i].id = -1;
        job_list[i].job_cmd_counter = 0;
        job_list[i].job_cmds_size = 8;

        job_list[i].job_commands = malloc(job_list[i].job_cmds_size * sizeof(job_command_t));

        if (job_list[i].job_commands == NULL) { goto alloc_err; }
    }
}

job_t *job_list_acquire_slot(void) {
    int i;

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

    return &job_list[i];
}

void add_background_job_command(execution_context_t *context) {
    job_t *job; 
    char *copy;

    job = (job_list_index == -1) ? job_list_acquire_slot() : &job_list[job_list_index];

    // do we need more space for more job_command_t's? 
    if (job->job_cmd_counter == job->job_cmds_size) {
        job->job_cmds_size += 8;
        job->job_commands = realloc(job->job_cmds_size * sizeof(job_command_t));

        if (job->job_commands == NULL) { goto alloc_err; }
    }

    copy_tokens(context, jobs);
    copy_io_files(context, jobs);
    job->job_cmd_counter++;

    if (context->pipeline_end) {
        job_list_index = -1; 
    }

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_tokens(execution_context_t *context, job_t *job) {
    int i;
    job->job_commands[job->job_cmd_counter].tokens = malloc((context->argc + 1) * sizeof(char *));
    if (job->job_commands[job->job_cmd_counter].tokens == NULL) { goto alloc_err; } 

    for (i = 0; i < context->argc; ++i) {
        copy = strdup(context->tokens[i]);

        if (copy == NULL) { goto alloc_err; } 

        job->job_commands[job->job_cmd_counter].tokens[i] = copy;
    }

    job->job_commands[job->job_cmd_counter].tokens[context->argc] = NULL;
    job->job_commands[job->job_cmd_counter].argc = context->argc;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

static void copy_io_files(execution_context_t *context, job_t *job) {
    job_command_t *job_cmd;

    job_cmd = &job->job_commands[job->job_cmd_counter];
    
    job_cmd->input_file  = ((context->flags & REDIR_IN) == REDIR_IN) ? strdup(context->input_file) : NULL;
    job_cmd->output_file = ((context->flags & REDIR_OUT) == REDIR_OUT) ? strdup(context->output_file) : NULL;
    job_cmd->append_file = ((context->flags & REDIR_APPEND) == REDIR_APPEND) ? strdup(context->append_file) : NULL;
    
    if (job_cmd->input_file == NULL || job_cmd->output_file == NULL || job_cmd->append_file == NULL ) {
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
    }
}
