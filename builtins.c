#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "shelly.h"

static int builtin_exit(char **tokens);
static int builtin_change_directory(char **tokens);

builtin_h builtins[] = {
    { "exit", builtin_exit },
    { "cd", builtin_change_directory}
};

int builtins_size() {
    return sizeof(builtins) / sizeof(builtin_h);
}

int builtin_exit(char **tokens) {
    exit(0);
}

int builtin_change_directory(char **tokens) {
    if (tokens[2] != NULL) {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }

    if (chdir(tokens[1]) == -1) {
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "cd: no such file or directory: %s\n", tokens[1]); break;
            case EACCES:
                fprintf(stderr, "cd: permission denied: %s\n", tokens[1]); break;
        }
    } 
    return 1;
}

