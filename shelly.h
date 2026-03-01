typedef struct {
    const char name[64];
    int (*builtin)(char **tokens);
} builtin_h; 

extern builtin_h builtins[];
int builtins_size(void);

#define      PATH_MAX        1024
