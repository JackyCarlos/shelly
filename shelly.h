#define      MAX_PATH_LEN        1024

typedef struct {
    const char name[64];
    int (*builtin)(char **tokens);
} builtin_h; 

extern builtin_h builtins[];
int builtins_size(void);
int is_builtin(char *command);

void shelly_loop(void);
