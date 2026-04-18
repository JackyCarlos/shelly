#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "parser.h"

#define     INPUT_CHUNK_SIZE    64

typedef enum {
    STATUS_TOKENIZER_WORDOUT,
    STATUS_TOKENIZER_WORDIN,
    STATUS_TOKENIZER_REDIRECT_OUT,
} tokenizer_status;

char *read_line(int *err) {
    char *buffer;
    int buf_size, len;
    int bytes_read;

    buf_size = INPUT_CHUNK_SIZE;
    len = 0;
    *err = -1;
    buffer = (char *) malloc(sizeof(char) * buf_size);

    if (buffer == NULL) { goto alloc_err; }

    while (1) {
        bytes_read = read(0, buffer + len, buf_size - len);

        if (bytes_read == 0) {
            *err = READ_LINE_EOF;

            goto input_err;
        } else if (bytes_read == -1) {
            *err = (errno == EINTR ? READ_LINE_SIGINT_INTERRUPT : READ_LINE_ERROR);
            
            goto input_err;
        } else {
            len += bytes_read;

            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                *err = READ_LINE_OK;

                return buffer;
            }

            if (len == buf_size) {
                buf_size += INPUT_CHUNK_SIZE;

                buffer = realloc(buffer, buf_size);
                if (buffer == NULL) { goto alloc_err; }
           }
        }         
    }

    return buffer;

    input_err:        
        free(buffer);
        return NULL;

    alloc_err:
        fprintf(stderr, "memory allocation error. Terminating shelly ..\n");
        exit(0);
}

token_t *tokenizer(char *line) {
    token_t *token_list;
    int token_array_size, token_buf_size;
    int i, j, str_index;
    tokenizer_status status;

    token_array_size = 32;
    token_list = (token_t *) malloc(sizeof(token_t) * token_array_size);

    if (token_list == NULL) { goto alloc_err; }

    i = -1;
    status = STATUS_TOKENIZER_WORDOUT;

    while (*line != '\0') {
        if (i == token_array_size - 2) {
            token_array_size += 32;

            token_list = realloc(token_list, sizeof(token_t) * token_array_size);

            if (token_list == NULL) { goto alloc_err; }
        }

        switch (*line) {
            case '<':
                i++;
                token_list[i].type = TOK_REDIRECT_IN_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '|':
                i++;
                token_list[i].type = TOK_REDIRECT_PIPE_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '>':
                if (status == STATUS_TOKENIZER_REDIRECT_OUT) {
                    token_list[i].type = TOK_REDIRECT_APPEND_TYPE;
                    status = STATUS_TOKENIZER_WORDOUT;
                } else {
                    i++;
                    token_list[i].type = TOK_REDIRECT_OUT_TYPE;
                    token_list[i].str = NULL;
                    status = STATUS_TOKENIZER_REDIRECT_OUT;
                } 
                break;
            case '&':
                i++;
                token_list[i].type = TOK_AMPS_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;

                break;

            case ' ':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '\r':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '\t':
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            default:
                if (status == STATUS_TOKENIZER_WORDOUT || status == STATUS_TOKENIZER_REDIRECT_OUT) {
                    i++;
                    token_buf_size = 32;

                    token_list[i].str = (char *) malloc(token_buf_size);
                    if (token_list[i].str == NULL) { goto alloc_err; }

                    token_list[i].type = TOK_WORD_TYPE;
                    token_list[i].index = 0;
                }

                if (token_list[i].index >= token_buf_size - 1) {
                    token_buf_size += 32;

                    token_list[i].str = (char *) realloc(token_list[i].str, token_buf_size);
                    if (token_list[i].str == NULL) { goto alloc_err; }
                }

                str_index = token_list[i].index++;
                token_list[i].str[str_index] = *line;
                str_index++;
                token_list[i].str[str_index] = '\0';

                status = STATUS_TOKENIZER_WORDIN; 
        };

        line++;
    }

    token_list[i + 1].type = TOK_NULL_TYPE;
    return token_list;

    alloc_err:
        fprintf(stderr, "memory allocation error\n");
        exit(0);
}

void free_token_list(token_t *token_list) {
    token_t *token;
    for (token = token_list; token->type != TOK_NULL_TYPE; token++) {
        free(token->str);
    }
    
    free(token_list);
}
