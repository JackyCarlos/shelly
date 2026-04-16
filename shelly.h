#define      MAX_PATH_LEN        1024

typedef struct {
    const char name[64];
    int (*builtin)(char **tokens);
} builtin_h; 

typedef enum {
    WORD_TYPE,
    REDIRECT_OUT_TYPE,
    REDIRECT_IN_TYPE,
    REDIRECT_PIPE_TYPE,
    REDIRECT_APPEND_TYPE,
    NULL_TYPE
} token_type;

typedef enum {
    STATUS_TOKENIZER_WORDOUT,
    STATUS_TOKENIZER_WORDIN,
    STATUS_TOKENIZER_REDIRECT_OUT,
} tokenizer_status;

typedef enum {
    CONTEXT_COMMAND_TYPE,
    CONTEXT_END_TYPE
} context_type;

typedef enum {
    STATUS_CONTEXT_INIT,
    STATUS_CONTEXT_WORD,
    STATUS_CONTEXT_REDIRECT_IN,
    STATUS_CONTEXT_REDIRECT_OUT,
    STATUS_CONTEXT_REDIRECT_APPEND,
    STATUS_CONTEXT_PIPE
} context_status;

typedef enum {
    IN              = 1,
    OUT             = 2,
    APPEND          = 4,
    INTO_PIPE       = 8,
    OUT_OF_PIPE     = 16
} redirect_flags;

typedef enum {
    READ_LINE_OK = 0,
    READ_LINE_EOF,
    READ_LINE_SIGINT_INTERRUPT,
    READ_LINE_ERROR
} readline_return_values;

typedef struct {
    token_type type;
    char *str;
    int index;
} token_t;

typedef struct {
    context_type type;

    char **tokens;
    int tokens_index;

    redirect_flags flags;

    char *input_file;
    char *output_file;
    char *append_file;
} execution_context_t;


extern builtin_h builtins[];
int builtins_size(void);
int is_builtin(char *command);

char *read_line(int *err);
token_t *tokenizer(char *line); 

execution_context_t *get_context(token_t *);
int context_counter(execution_context_t *context_list);

void shelly_loop(void);