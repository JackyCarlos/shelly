#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>

#include "input.h"
#include "../external/linenoise/linenoise.h"

void SIGINT_Handler(int sig);
void SIGCHLD_handler(int sig);

volatile sig_atomic_t got_sigint = 0;
volatile sig_atomic_t got_sigchld = 0;

void SIGINT_Handler(int sig) {
    got_sigint = 1;
}

void SIGCHLD_handler(int sig) {
    got_sigchld = 1;
}

void setup_signal_handlers(void) {
    struct sigaction sa, sa2, sa3;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIGINT_Handler;
    sa.sa_flags = 0;

    sigemptyset(&sa2.sa_mask);
    sa2.sa_handler = SIG_IGN;
    sa2.sa_flags = 0;
    
    sigemptyset(&sa3.sa_mask);
    sa3.sa_handler = SIGCHLD_handler;
    sa3.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTTOU, &sa2, NULL);
    sigaction(SIGCHLD, &sa3, NULL);
}

char *shelly_linenoise(char *prompt, int *err, t_sigchld_hook on_sigchld) {
    struct linenoiseState ls;
    char buf[4096];
    char *line;

    linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), prompt);

    while (1) {
        fd_set rfds;

        if (got_sigchld) {
            got_sigchld = 0;

            // run job reaping and cleanup via hook
        }

        FD_ZERO(&rfds);
        FD_SET(ls.ifd, &rfds);

        int ret = select(ls.ifd + 1, &rfds, NULL, NULL, NULL);

        if (ret == -1) {
            *err = READ_LINE_ERROR;

            if (errno == EINTR) {
                if (got_sigchld) {
                    got_sigchld = 0;

                    // run job reaping and cleanup via hook
                    continue;
                }

                if (got_sigint) {
                    got_sigint = 0;
                    *err = READ_LINE_SIGINT_INTERRUPT;
                } 
            }

            linenoiseEditStop(&ls);
            return NULL;
        }

        if (FD_ISSET(ls.ifd, &rfds)) {
            errno = 0;
            line = linenoiseEditFeed(&ls);

            if (line == linenoiseEditMore) {
                continue;
            }

            linenoiseEditStop(&ls);

            if (line == NULL) {
                if (errno == EAGAIN) {
                    *err = READ_LINE_SIGINT_INTERRUPT;
                    return NULL;
                }

                *err = READ_LINE_EOF;
                return NULL;
            }

            *err = READ_LINE_OK;
            return line;
        }
    }
}
