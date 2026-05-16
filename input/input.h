typedef enum {
    READ_LINE_OK = 0,
    READ_LINE_EOF,
    READ_LINE_SIGINT_INTERRUPT,
    READ_LINE_ERROR
} readline_return_values;

typedef void (*t_sigchld_hook)(void);

void setup_signal_handlers(void);
char *shelly_linenoise(char *prompt, int *err, t_sigchld_hook on_sigchld);

