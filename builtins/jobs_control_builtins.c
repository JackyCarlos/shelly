#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "builtins.h"
#include "../job-control/jobs.h"
#include "../common/command_flags.h"

static int get_jobid(char **, char *);
static void job_mark_running(job_t *job);
static void print_job(job_t *job);
static int print_command_group(job_t *job, int start_index);
static void print_command(job_command_t *cmd);
static int job_status_id(job_cmd_status status);
static void print_redirects(job_command_t *job_command);
static int job_status_id(job_cmd_status status);

char *job_statuses[] = {"running", "suspended", "done", "exit 127"};

int job_control_builtin_fg(char **tokens) {
    int job_id;
    job_t *job;

    job_id = get_jobid(tokens, "fg");

    if (job_id == -1) {
        return -1;
    }

    job = &job_list[job_id];
    job->is_background = 0;

    if (job->status == SUSPENDED) {
        job_mark_running(job);

        if (kill(-job->pgid, SIGCONT) == -1) {
            fprintf(stderr, "error continuing job %d\n", job_id);
        }
    } 

    return job_control_after_launch(job);
}

int job_control_builtin_bg(char **tokens) {
    int job_id;
    job_t *job;

    job_id = get_jobid(tokens, "bg");

    if (job_id == -1) {
        return -1;
    }

    job = &job_list[job_id];

    if (job->status == RUNNING && job->is_background == 1) {
        fprintf(stderr, "bg: job already in background\n");
        return -1;
    }

    job_mark_running(job);

    if (kill(-job->pgid, SIGCONT) == -1) {
        fprintf(stderr, "error continuing job %d\n", job_id);
    }

    job->status = RUNNING;

    return 0;
}

static int get_jobid(char **tokens, char *job_builtin) {
    int job_id;
    char *end_ptr;
    
    if (tokens[1] == NULL || tokens[2] != NULL || *tokens[1] != '%') {
        printf("%s: usage: %s %%<id>\n", job_builtin, job_builtin);
        return -1;
    }

    job_id = strtol(tokens[1] + 1, &end_ptr, 10);
    if (*end_ptr != '\0') {
        printf("%s: usage: %s %%<id>\n", job_builtin, job_builtin);
        return -1;
    }

    job_id--;

    if (job_id >= job_list_size || job_id < 0 ||
            job_list[job_id].id == -1 || !job_list[job_id].is_background) {

        printf("%s: %%%d: no such job\n", job_builtin, job_id + 1);
        return -1;
    }

    return job_id;
}

static void job_mark_running(job_t *job) {
    for (int i = 0; i < job->job_cmd_counter; ++i) {
        if (job->job_commands[i].job_stat == CMD_SUSPENDED) {
            job->job_commands[i].job_stat = CMD_RUNNING;
        }
    }    
}


int job_control_builtin_jobs(char **tokens) {
    int job_id;

    for (job_id = 0; job_id < job_list_size; ++job_id) {
        if (job_list[job_id].id == -1 || job_list[job_id].pgid == -1 || !job_list[job_id].is_background) {
            continue;
        }

        print_job(&job_list[job_id]);
    }

    return 0;
}

static void print_job(job_t *job) {
    int i;

    i = 0;
    while (i < job->job_cmd_counter) {
        i = print_command_group(job, i);
    }
}

static int print_command_group(job_t *job, int start_index) {
    int i, status_array_index;

    i = start_index;

    if (start_index == 0)
        printf("[%d]    ", job->id + 1);
    else
        printf("       ");

    status_array_index = job_status_id(job->job_commands[i].job_stat);

    printf("%-11s", job_statuses[status_array_index]);

    print_command(&job->job_commands[i]);
    job_cmd_status current_status = job->job_commands[i].job_stat;
    i++;

    while (i < job->job_cmd_counter && job->job_commands[i].job_stat == current_status) {
        print_command(&job->job_commands[i]);
        i++;
    }

    printf("\n");

    return i;
}

static void print_command(job_command_t *cmd) {
    int i;

    i = 0;
    while (cmd->tokens[i] != NULL) {
        printf("%s ", cmd->tokens[i]);
        i++;
    }

    print_redirects(cmd);
}

static int job_status_id(job_cmd_status status) {
    if (status == RUNNING)
        return 0;
    if (status == SUSPENDED)
        return 1;
    if (status == TERMINATED)
        return 2;
    return 3;
}

static void print_redirects(job_command_t *job_command) {
    if ((job_command->flags & REDIR_IN ) == REDIR_IN) {
        printf("< %s ", job_command->input_file);
    }

    if ((job_command->flags & REDIR_OUT ) == REDIR_OUT) {
        printf("> %s ", job_command->output_file);
    }

    if ((job_command->flags & REDIR_APPEND) == REDIR_APPEND) {
        printf(">> %s ", job_command->append_file);
    }

    if ((job_command->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
        printf("| ");
    }
}
