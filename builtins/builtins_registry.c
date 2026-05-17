#include "builtins.h"
#include "string.h"

/* core builtins */
int builtin_exit(char **tokens);
int builtin_change_directory(char **tokens);
int builtin_type(char **tokens);

/* job control builtins */
int job_control_builtin_jobs(char **tokens);
int job_control_builtin_fg(char **tokens);
int job_control_builtin_bg(char **tokens);

const builtin_t builtins[] = {
    { "exit", builtin_exit },
    { "cd", builtin_change_directory},
    { "type", builtin_type }, 
    { "jobs", job_control_builtin_jobs }, 
    { "fg", job_control_builtin_fg }, 
    { "bg",  job_control_builtin_bg }
};

const int builtins_size = sizeof(builtins) / sizeof(builtin_t);

int is_builtin(char *command, int *builtin_id) {
    int i;

    for (i = 0; i < builtins_size; ++i) {
        if (strcmp(command, builtins[i].name) == 0) {
            if (builtin_id != NULL) *builtin_id = i;
            
            return 1;
        }      
    } 

    return 0;
}
