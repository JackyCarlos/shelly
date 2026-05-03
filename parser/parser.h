/*
 ** ownership & lifetime rules for parser structures
 ** 
 ** read_line():
 **   - allocates a line of text from terminal input buffer
 **
 ** tokenizer(line):
 **   - allocates a list of tokens
 **   - word tokens contain heap-allocated strings
 **   - ownership is transferred to the caller
 **
 ** get_context(token_list):
 **   - builds execution_context_t structures based on token_list
 **   - DOES NOT copy token strings
 **   - context structures contain pointers to token->str
 **   - does NOT take ownership of token_list
 **
 ** lifetime constraints:
 **   - token_list must remain valid as long as context_list is in use
 **   - context_list contains borrowed references to token strings
 **
 ** error handling:
 **   - in case of memory alloc errors shelly terminates
 **   - on failure, read_line() frees internally allocated memory
 **   - on failure, get_context() frees all internally allocated memory
 **   - token_list remains the caller's responsibility and must be freed by caller
 **
 ** caller responsibilities:
 **   - always free token_list after context_list is no longer needed
 **   - after calling executor() token_list and context_list must be freed by calling 
 **     free_context_list() and free_token_list() respectivley
 **     
 */

typedef enum {
    TOK_WORD_TYPE,
    TOK_REDIRECT_OUT_TYPE,
    TOK_REDIRECT_IN_TYPE,
    TOK_REDIRECT_PIPE_TYPE,
    TOK_REDIRECT_APPEND_TYPE,
    TOK_AMPS_TYPE,
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
    int argc;

    redirect_flags flags;
    char *input_file;
    char *output_file;
    char *append_file;

    int is_background;
    int pipeline_end;
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

void free_context_list(execution_context_t *context_list);
void free_token_list(token_t *token_list);
