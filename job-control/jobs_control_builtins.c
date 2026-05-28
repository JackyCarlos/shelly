#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "../common/command_flags.h"
#include "../builtins/builtins.h"
#include "jobs.h"
#include "job-display/job-display.h"

static int get_jobid(char **, char *);
static void job_mark_running(job_t *job);

int job_control_builtin_fg(char **tokens) {
    int job_id;
    job_t *job;

    job_id = get_jobid(tokens, "fg");

    if (job_id == -1) {
        return -1;
    }

    job = &job_list[job_id];
    job->is_background = 0;

    job_display_print_fg(job);

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

    if (kill(-job->pgid, SIGCONT) == -1) {
        fprintf(stderr, "error continuing job %d\n", job_id);
    }

    job_display_print_bg(job);
    job_mark_running(job);

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

    job->status = RUNNING;
}

int job_control_builtin_jobs(char **tokens) {
    int job_id;

    for (job_id = 0; job_id < job_list_size; ++job_id) {
        if (job_list[job_id].id == -1 || job_list[job_id].pgid == -1 || !job_list[job_id].is_background) {
            continue;
        }

        job_display_print_job(&job_list[job_id]);
    }

    return 0;
}
