#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "shelly.h"

#define MAX_PATH

static int builtin_exit(char **tokens);
static int builtin_change_directory(char **tokens);
static int builtin_type(char **tokens);

builtin_h builtins[] = {
    { "exit", builtin_exit },
    { "cd", builtin_change_directory },
    { "type", builtin_type }
};

int builtins_size() {
    return sizeof(builtins) / sizeof(builtin_h);
}

int is_builtin(char *command) {
    int i;

    for (i = 0; i < builtins_size(); ++i) {
        if (strcmp(command, builtins[i].name) == 0) {
            return i;
        }      
    } 

    return -1;
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

    return 0;
}

int builtin_type(char **tokens) {
    tokens++;

    for (; *tokens != NULL; tokens++) {
        if (is_builtin(*tokens) != -1) {
            printf("%s is a shell builtin\n", *tokens);

        } else if (access(*tokens, F_OK) == 0) {
            char fullPath[256];
            fullPath[0] = '\0';

            realpath(*tokens, fullPath);    
            printf("%s is %s\n", *tokens, fullPath);

        } else {
            printf("%s not found\n", *tokens);
        }
    } 

    return 0;
}
