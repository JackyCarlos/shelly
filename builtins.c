#include <stdio.h>
#include <unistd.h>
#include "shelly.h"

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin[]) (char ** ) = {
    &shelly_cd,
    &shelly_help,
    &shelly_exit
};

int shelly_num_builtins(void) {
    return sizeof(builtin_str) / sizeof(char *);
}

/* Builtin function implementations */
int shelly_cd(char **args)
{
    if (args[1] == NULL)
        fprintf(stderr, "shelly: expected argument to \"cd\"\n");
    else 
        if (chdir(args[1]) != 0)
            perror("shelly");
    
    return 0;
}

int shelly_help(char **args)
{
    int i;
    printf("shelly by Robert Eikmanns\n");
    printf("The following builtins are supported: \n");

    for (i = 0; i < shelly_num_builtins(); i++)
        printf("   %s\n", builtin_str[i]);
    
    printf("use the man command for Information on other programs");
    return 1; 
}

int shelly_exit(char **args)
{
    return 0;
}