typedef struct {
    const char name[64];
    int (*builtin)(char **tokens);
} builtin_h; 

typedef struct {
    char **tokens;
    char *input_file;
    char *output_file;
    char *append_file;
} execution_context_t;

extern builtin_h builtins[];
int builtins_size(void);

#define      PATH_MAX        1024
