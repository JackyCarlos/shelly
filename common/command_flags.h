#ifndef REDIR_FLAGS
#define REDIR_FLAGS

typedef enum {
    REDIR_IN              = 1,
    REDIR_OUT             = 1 << 1,
    REDIR_APPEND          = 1 << 2,
    REDIR_INTO_PIPE       = 1 << 3,
    REDIR_OUT_OF_PIPE     = 1 << 4
} redirect_flags;

#endif
