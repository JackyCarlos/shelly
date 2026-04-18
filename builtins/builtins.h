typedef struct {
    const char name[64];
    int (*builtin)(char **tokens);
} builtin_t; 

extern const builtin_t builtins[];

int is_builtin(char *command);
