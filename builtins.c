#include <stdio.h>
#include <stdlib.h>

#include "shelly.h"

static int builtin_exit(char **tokens);

builtin_h builtins[] = {
    {"exit", builtin_exit}
};

int builtins_size() {
    return sizeof(builtins) / sizeof(builtin_h);
}

int builtin_exit(char **tokens) {
    exit(0);
}

