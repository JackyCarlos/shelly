#include <stdio.h>

#include "../jobs.h"
#include "job-display.h"

static int print_command_group(job_t *job, int start_index, job_display_mode_t mode);
static void print_command(job_command_t *cmd);
static void print_redirects(job_command_t *cmd);
static char *status_label(job_cmd_status status, job_display_mode_t mode);

void job_display_print_job(job_t *job) {
    int i = 0;

    while (i < job->job_cmd_counter) {
        i = print_command_group(job, i, JOB_DISPLAY_NORMAL);
    }
}

void job_display_print_fg(job_t *job) {
    int i = 0;

    while (i < job->job_cmd_counter) {
        i = print_command_group(job, i, JOB_DISPLAY_FG);
    }
}

void job_display_print_bg(job_t *job) {
    int i = 0;

    while (i < job->job_cmd_counter) {
        i = print_command_group(job, i, JOB_DISPLAY_BG);
    }
}

void job_display_print_ctrlz(job_t *job) {
    int i = 0;

    printf("\n");

    while (i < job->job_cmd_counter) {
        i = print_command_group(job, i, JOB_DISPLAY_CTRLZ);  
    } 
}

void job_display_print_background_start(job_t *job) {
    int i;

    printf("[%d] ", job->id + 1);

    for (i = 0; i < job->job_cmd_counter; ++i) {
        printf("%d ", job->job_commands[i].pid);
    }

    printf("\n");
}

static int print_command_group(job_t *job, int start_index, job_display_mode_t mode) {
    int i = start_index;
    job_cmd_status current_status;

    if (mode != JOB_DISPLAY_CTRLZ) {
        if (start_index == 0) {
            printf("[%d]    ", job->id + 1);
        } else {
            printf("       ");
        }
    } else {
        printf("shelly: ");
    }

    current_status = job->job_commands[i].job_stat;

    printf("%-11s", status_label(current_status, mode));

    print_command(&job->job_commands[i]);
    i++;

    while (i < job->job_cmd_counter && job->job_commands[i].job_stat == current_status) {
        print_command(&job->job_commands[i]);
        i++;
    }

    printf("\n");
    return i;
}

static char *status_label(job_cmd_status status, job_display_mode_t mode) {    
    if (mode == JOB_DISPLAY_BG) {
        if (status == SUSPENDED) {
            return "continued";
        }
    }

    if (mode == JOB_DISPLAY_FG) {
        if (status == SUSPENDED) {
            return "continued";
        }
        if (status == RUNNING) {
            return "running";
        }
    }

    switch (status) {
        case RUNNING:
            return "running";
        case SUSPENDED:
            return "suspended";
        case TERMINATED:
            return "done";
        default:
            return "exit 127";
    }
}

static void print_command(job_command_t *cmd) {
    int i = 0;

    while (cmd->tokens[i] != NULL) {
        printf("%s ", cmd->tokens[i]);
        i++;
    }

    print_redirects(cmd);
}

static void print_redirects(job_command_t *cmd) {
    if ((cmd->flags & REDIR_IN) == REDIR_IN) {
        printf("< %s ", cmd->input_file);
    }     

    if ((cmd->flags & REDIR_OUT) == REDIR_OUT) {
        printf("> %s ", cmd->output_file);
    }
        
    if ((cmd->flags & REDIR_APPEND) == REDIR_APPEND) {
        printf(">> %s ", cmd->append_file);
    }
        
    if ((cmd->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
        printf("| ");
    }      
}
