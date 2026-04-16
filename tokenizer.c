#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "shelly.h"

typedef enum {
    READ_LINE_OK = 0,
    READ_LINE_EOF,
    READ_LINE_SIGINT_INTERRUPT,
    READ_LINE_ERROR
} readline_return_values;

char *read_line(int *err) {
    char *buffer;
    int buf_size, len;
    int bytes_read;

    buf_size = 8;
    len = 0;
    *err = -1;
    buffer = (char *) malloc(sizeof(char) * buf_size);

    if (buffer == NULL) {
        fprintf(stderr, "memory allocation error. Terminating .. \n");
        exit(0);
    }

    while (1) {
        bytes_read = read(0, buffer + len, buf_size - len);

        if (bytes_read == 0) {
            free(buffer);
            *err = READ_LINE_EOF;

            return NULL;
        } else if (bytes_read == -1) {
            free(buffer);
            *err = (errno == EINTR ? READ_LINE_SIGINT_INTERRUPT : READ_LINE_ERROR);
            
            return NULL;
        } else {
            len += bytes_read;

            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                *err = READ_LINE_OK;

                return buffer;
            }

            if (len == buf_size) {
                buf_size += 8;

                buffer = realloc(buffer, buf_size);

                // memory error checking left to do
           }
        }         
    }

    return buffer;
}

token_t *tokenizer(char *line) {
    token_t *token_list;
    int token_array_size, token_buf_size;
    int i, j, str_index;
    tokenizer_status status;

    token_array_size = 32;
    token_list = (token_t *) malloc(sizeof(token_t) * token_array_size);

    i = -1;
    status = STATUS_TOKENIZER_WORDOUT;

    while (*line != '\0') {
        if (i == token_array_size - 2) {
            token_array_size += 32;

            token_list = realloc(token_list, sizeof(token_t) * token_array_size);

            if (token_list == NULL) {
                goto err;
            }
        }

        switch (*line) {
            case '<':
                i++;
                token_list[i].type = REDIRECT_IN_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '|':
                i++;
                token_list[i].type = REDIRECT_PIPE_TYPE;
                token_list[i].str = NULL;
                status = STATUS_TOKENIZER_WORDOUT;
                break;

            case '>':
                if (status == STATUS_TOKENIZER_REDIRECT_OUT) {
                    token_list[i].type = REDIRECT_APPEND_TYPE;
                    status = STATUS_TOKENIZER_WORDOUT;
                } else {
                    i++;
                    token_list[i].type = REDIRECT_OUT_TYPE;
                    token_list[i].str = NULL;
                    status = STATUS_TOKENIZER_REDIRECT_OUT;
                } 
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
                    if (token_list[i].str == NULL) {
                        goto err;
                    }

                    token_list[i].type = WORD_TYPE;
                    token_list[i].index = 0;
                }

                if (token_list[i].index >= token_buf_size - 1) {
                    token_buf_size += 32;
                    token_list[i].str = (char *) realloc(token_list[i].str, token_buf_size);

                    if (token_list[i].str == NULL) {
                        goto err;
                    }
                }

                str_index = token_list[i].index++;
                token_list[i].str[str_index] = *line;
                str_index++;
                token_list[i].str[str_index] = '\0';

                status = STATUS_TOKENIZER_WORDIN; 
        };

        line++;
    }

    token_list[i + 1].type = NULL_TYPE;
    return token_list;

    err:
        fprintf(stderr, "memory allocation error\n");
        
        for (j = 0; j < i; ++j) {       
            free(token_list[j].str);
        }

        free(token_list);
        return NULL;
}
