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
static int in_path(char *token, char *full_path);

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
    char full_path[256];

    for (; *tokens != NULL; tokens++) {
        if (is_builtin(*tokens) != -1) {
            printf("%s is a shell builtin\n", *tokens);

        } else if (in_path(*tokens, full_path)) {  
            printf("%s is %s\n", *tokens, full_path);

        } else {
            printf("%s not found\n", *tokens);
        }
    } 

    return 0;
}

static int in_path(char *token, char *full_path) {
    char *base_path, *path_var, path_var2[256];
    
    path_var = getenv("PATH");
    strcpy(path_var2, path_var);
    base_path = strtok(path_var, ":");

    while (base_path != NULL) {
        strcpy(full_path, base_path);
        strcat(full_path, "/");
        strcat(full_path, token);

        if (access(full_path, X_OK) == 0) {
            return 1;
        }

        base_path = strtok(NULL, ":");
    }

    return 0;
}
