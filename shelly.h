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

typedef enum {
    WORD_TYPE,
    REDIRECT_OUT,
    REDIRECT_IN,
    REDIRECT_PIPE,
    REDIRECT_APPEND,
    NULL_TYPE
} token_type;

typedef enum {
    STATUS_WORDOUT,
    STATUS_WORDIN,
    STATUS_REDIRECT_OUT,
} tokenizer_status;

typedef struct {
    token_type type;
    char *str;
    int index;
} token_t;


extern builtin_h builtins[];
int builtins_size(void);

#define      PATH_MAX        1024
