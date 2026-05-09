#include <stdio.h>
#include <unistd.h>

#include "builtins.h"
#include "../job-control/jobs.h"

int job_control_builtin_jobs(char **tokens) {
    printf("jobs:\n");

    return 0;
}
