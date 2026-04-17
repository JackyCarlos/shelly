typedef enum {
    TOK_WORD_TYPE,
    TOK_REDIRECT_OUT_TYPE,
    TOK_REDIRECT_IN_TYPE,
    TOK_REDIRECT_PIPE_TYPE,
    TOK_REDIRECT_APPEND_TYPE,
    TOK_NULL_TYPE
} token_type;

typedef enum {
    CTX_COMMAND_TYPE,
    CTX_END_TYPE
} context_type;

typedef struct {
    token_type type;
    char *str;
    int index;
} token_t;

typedef enum {
    REDIR_IN              = 1,
    REDIR_OUT             = 2,
    REDIR_APPEND          = 4,
    REDIR_INTO_PIPE       = 8,
    REDIR_OUT_OF_PIPE     = 16
} redirect_flags;

typedef struct {
    context_type type;

    char **tokens;
    int tokens_index;

    redirect_flags flags;

    char *input_file;
    char *output_file;
    char *append_file;
} execution_context_t;

typedef enum {
    READ_LINE_OK = 0,
    READ_LINE_EOF,
    READ_LINE_SIGINT_INTERRUPT,
    READ_LINE_ERROR
} readline_return_values;

char *read_line(int *err);
token_t *tokenizer(char *line); 

execution_context_t *get_context(token_t *);
int context_counter(execution_context_t *context_list);
