#include <stdio.h>
#include <unistd.h>

#include "builtins.h"
#include "../job-control/jobs.h"
#include "../common/command_flags.h"

static void print_redirects(job_command_t *job_command);

char *job_statuses[] = {"running", "suspended", "done"};

int job_control_builtin_jobs(char **tokens) {
    int i, j, job_id, job_stat_id, ongoing;
    job_command_t *job_command;

    job_stat_id = 0;
    ongoing = 1;

    for (job_id = 0; job_id < job_list_size; ++job_id) {
        i = 0;
        
        if (job_list[job_id].id == -1 || !job_list[job_id].is_background) {
            continue;
        }

        printf("[%d]  %s ", job_list[job_id].id + 1, " ");

        for (i = 0; i < job_list[job_id].job_cmd_counter; ++i) {
            job_command = &job_list[job_id].job_commands[i];
            ongoing = 1;
            j = 0;

            if (job_command->job_stat != RUNNING) {
                job_stat_id = (job_command->job_stat == STOPPED) ? 1 : 2;
            }
            
            (i == 0) ? printf("") : printf("       ");
            printf("%-11s", job_statuses[job_stat_id]);

            while (ongoing) {
                while (job_command->tokens[j] != NULL) {
                    printf("%s ", job_command->tokens[j]);
                    j++;
                }

                print_redirects(job_command);
                
                if (i + 1 < job_list[job_id].job_cmd_counter) {
                    i++; 
                    job_command = &job_list[job_id].job_commands[i];

                    if (job_command->job_stat == RUNNING || job_command->job_stat == STOPPED) {
                        i--;
                        ongoing = 0;
                        printf("\n");
                    }
                } else {
                    printf("\n");
                    ongoing = 0;
                }

                j = 0;
            }








        }

    }

    return 0;
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
