#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shelly.h"
#include "parser/parser.h"
#include "builtins/builtins.h"
#include "job-control/jobs.h"
#include "executor/executor.h"
#include "input/input.h"

static char *cli_line(void);

void ensure_shell_process_group(void);
void SIGINT_Handler(int sig);
void SIGCHLD_handler(int sig);
void setup_signal_handlers(void);

pid_t global_shell_pgid = 0;

volatile sig_atomic_t sig_sigint = 0;
volatile sig_atomic_t sig_sigchld = 0;

void shelly_loop(void) {
    char *cli_line_string, *line;
    token_t *token_list;
    execution_context_t *context_list;
    int err;
    job_t *started_job;

    ensure_shell_process_group();

    if (tcgetpgrp(0) != global_shell_pgid) {
        // do some sleeping and check again if shell is the foreground process
        ;
    }

    setup_signal_handlers();
    init_job_control();

    while (1) {
        cli_line_string = cli_line();
        line = shelly_linenoise(cli_line_string, &err, reap_background_jobs);

        if (line == NULL) {
            switch (err) {
                case READ_LINE_EOF:
                    printf("exit\n");
                    exit(0);
                case READ_LINE_SIGINT_INTERRUPT:
                    continue;
                default:
                    continue;
            }
        }

        token_list = tokenizer(line);
        free(line);
        free(cli_line_string);

        context_list = get_context(token_list);

        if (context_list == NULL) {
            free_token_list(token_list);
            fprintf(stderr, "syntax error\n");     
            continue;
        }
        
        started_job = executor(context_list);
        free_token_list(token_list);
        free_context_list(context_list);

        job_control_after_launch(started_job);   // returns exit code of last command
    }
}

void ensure_shell_process_group(void) {
    pid_t pgid;

    pgid = getpgid(0);

    if (pgid == -1) {
        setpgid(0, 0);
        global_shell_pgid = getpid();
    } else {
        global_shell_pgid = pgid;
    }
}

static char *cli_line(void) {
    char *cli_line_string, *cwd, *user;
    char hostname[64];

    cli_line_string = malloc(sizeof(char) * 128);

    cwd = getcwd(NULL, MAX_PATH_LEN);
    user = getlogin();
    gethostname(hostname, sizeof(hostname));

    sprintf(cli_line_string, "\033[0;31m%s@%s\033[0m \033[0;34m%s\033[0m $ ", (user != NULL ? user : ""), hostname, cwd);

    free(cwd);
    
    return cli_line_string;
}
