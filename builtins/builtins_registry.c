#include "builtins.h"
#include "string.h"

/* core builtins */
int builtin_exit(char **tokens);
int builtin_change_directory(char **tokens);
int builtin_type(char **tokens);

/* job control builtins */

const builtin_t builtins[] = {
    { "exit", builtin_exit },
    { "cd", builtin_change_directory},
    { "type", builtin_type }
};

const int builtins_size = sizeof(builtins) / sizeof(builtin_t);

int is_builtin(char *command) {
    int i;

    for (i = 0; i < builtins_size; ++i) {
        if (strcmp(command, builtins[i].name) == 0) {
            return i;
        }      
    } 

    return -1;
}
