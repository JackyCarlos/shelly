#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "../parser/parser.h"

void SIGINT_Handler(int sig);
void SIGCHLD_handler(int sig);

static volatile sig_atomic_t got_sigint = 0;
static volatile sig_atomic_t got_sigchld = 0;

static char *finished_line = NULL;
static int line_ready = 0;
static int eof_seen = 0;

void SIGINT_Handler(int sig) {
    got_sigint = sig;
}

void SIGCHLD_handler(int sig) {
    got_sigchld = sig;
}

void setup_signal_handlers(void) {
    struct sigaction sa, sa2, sa3;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIGINT_Handler;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sa2.sa_handler = SIG_IGN;
    sa2.sa_flags = 0;
    
    sigemptyset(&sa3.sa_mask);
    sa3.sa_handler = SIGCHLD_handler;
    sa3.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTTOU, &sa2, NULL);
    sigaction(SIGCHLD, &sa3, NULL);
}

static void readline_callback(char *line) {
    if (line == NULL) {
        eof_seen = 1;
        line_ready = 1;
        return;
    }

    finished_line = line;
    line_ready = 1;
}

char *shelly_readline(char *prompt, int *err) {
    finished_line = NULL;
    line_ready = 0;
    eof_seen = 0;

    fprintf(stderr, "INSTALL READLINE\n");
    rl_callback_handler_install(prompt, readline_callback);

    while (!line_ready) {
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, NULL);

        if (ret == -1) {
            if (errno == EINTR) {
                if (got_sigint) {
                    got_sigint = 0;

                    rl_replace_line("", 0);
                    rl_on_new_line();

                    rl_callback_handler_remove();
                    *err = READ_LINE_SIGINT_INTERRUPT;

                    return NULL;
                } else if (got_sigchld) {
                    ;
                }

                continue;
            }

            perror("select");
            rl_callback_handler_remove();

            return NULL;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            rl_callback_read_char();
        }

        if (got_sigint) {
            got_sigint = 0;

            rl_replace_line("", 0);
            rl_on_new_line();

            rl_callback_handler_remove();

            *err = READ_LINE_SIGINT_INTERRUPT;
            return NULL;
        }
    }

    if (eof_seen) {
        *err = READ_LINE_EOF;
        return NULL;
    }

    rl_callback_handler_remove();

    *err = READ_LINE_OK;
    rl_deprep_terminal();

    return finished_line;
}
