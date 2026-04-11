#include <stdlib.h>
#include <stdio.h>

#include "shelly.h"

char *read_line(void) {
    char *line, *line2;
    int buf_size, c;

    buf_size = 64;
    line = line2 = (char *) malloc(sizeof(char) * buf_size);

    if (line == NULL) {
        fprintf(stderr, "memory allocation error. Terminating .. \n");
        exit(0);
    }

    c = getchar();
    while (c != EOF && c != '\n') {
        if (line2 - line == buf_size) {
            buf_size += 64;
            line = line2 = (char *) realloc(line, buf_size);
            line2 += (buf_size - 64);
        }

        *line2++ = c;
        c = getchar();
    }

    if (c == EOF) {
        printf("\nexit\n");
        exit(0);
    }

    *line2 = '\0';

    return line;
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
