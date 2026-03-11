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
    REDIRECT_OUT_TYPE,
    REDIRECT_IN_TYPE,
    REDIRECT_PIPE_TYPE,
    REDIRECT_APPEND_TYPE,
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

#define      MAX_PATH_LEN        1024
