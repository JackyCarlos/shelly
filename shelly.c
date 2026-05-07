#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shelly.h"
#include "parser/parser.h"
#include "builtins/builtins.h"
#include "job-control/jobs.h"
#include "executor/executor.h"

static void print_cli(void);

void ensure_shell_process_group(void);
void SIGINT_Handler(int sig);
void setup_signal_handlers(void);

pid_t global_shell_pgid = 0;

void shelly_loop(void) {
    char *line;
    token_t *token_list;
    execution_context_t *context_list;
    int status, err;

    ensure_shell_process_group();

    if (tcgetpgrp(0) != global_shell_pgid) {
        // do some sleeping and check again if shell is the foreground process
        ;
    }

    setup_signal_handlers();
    init_job_control();

    status = 1;

    while (1) {
        print_cli();
        
        line = read_line(&err);    
        if (line == NULL) {
            switch (err) {
                case READ_LINE_EOF:
                    printf("\nexit\n");
                    exit(0);
                case READ_LINE_SIGINT_INTERRUPT:
                    printf("\n");
                    continue;
                default:
                    continue;
            }
        }

        token_list = tokenizer(line);
        free(line);

        context_list = get_context(token_list);

        if (context_list == NULL) {
            free_token_list(token_list);
            fprintf(stderr, "syntax error\n");     
            continue;
        }
        
        status = executor(context_list); 
        
        free_token_list(token_list);
        free_context_list(context_list);
    }
}

void SIGINT_Handler(int sig) {
    return;
}

void setup_signal_handlers(void) {
    struct sigaction sa, sa2;
    sa.sa_handler = SIGINT_Handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa2.sa_handler = SIG_IGN;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTTOU, &sa2, NULL);
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

static void print_cli(void) {
    char *cwd, *user;
    char hostname[64];

    cwd = getcwd(NULL, MAX_PATH_LEN);
    user = getlogin();
    gethostname(hostname, sizeof(hostname));

    printf("%s@%s %s $ ", (user != NULL ? user : ""), hostname, cwd);

    free(cwd);
    fflush(stdout);
}

