#include <stdio.h>
#include <unistd.h>

#include "builtins.h"
#include "../job-control/jobs.h"
#include "../common/command_flags.h"

static void print_job(job_t *job);
static int print_command_group(job_t *job, int start_index);
static void print_command(job_command_t *cmd);
static int job_status_id(job_status status);
static void print_redirects(job_command_t *job_command);
static int job_status_id(job_status status);

char *job_statuses[] = {"running", "suspended", "done"};

int job_control_builtin_jobs(char **tokens) {
    int job_id;

    for (job_id = 0; job_id < job_list_size; ++job_id) {
        if (job_list[job_id].id == -1 || !job_list[job_id].is_background) {
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
    job_status current_status = job->job_commands[i].job_stat;
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

static int job_status_id(job_status status) {
    if (status == RUNNING)
        return 0;
    if (status == STOPPED)
        return 1;
    return 2;
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
