#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shelly(void);
char *read_line(void);
char **split_line(char *line);

int main(int argc, char *argv[]) {
    char *line;
    char **tokens;

    while (1) {
        line = read_line();
        tokens = split_line(line);
        
        for (int i = 0; tokens[i] != NULL; ++i) {
            printf("%d. argument is %s\n", i, tokens[i]);
        }
    }
    
    return 0;
}

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
            line = (char *) realloc(line, buf_size);
        }

        *line2++ = c;
        c = getchar();
    }

    *line2 = '\0';

    return line;
}

char **split_line(char *line) {
    char *delim, *token, **tokens, **tokens2;
    int size;

    delim = " \t\r";
    size = 32;
    tokens = tokens2 = (char **) malloc(sizeof(char *) * size);

    if (tokens == NULL) {
        fprintf(stderr, "memory allocation error. Terminating .. \n");
        exit(0);        
    }



    token = strtok(line, delim);

    while (token != NULL) {
        if (tokens2 - tokens == size) {
            size += 32;
            tokens = (char **) realloc(tokens, sizeof(char *) * size);
        }

        *tokens2++ = token; 

        token = strtok(NULL, delim);
    }

    *tokens2 = NULL;

    return tokens;
}

