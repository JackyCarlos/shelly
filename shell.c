#include "shelly.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    // Load config files. if any. 

    // Run command loop
    shelly_loop();

    // perform any shutdown/cleanup
    return 0; 
}

void shelly_loop(void)
{
    char *line;  
    char **args;
    int status = 1;

    do {
        printf("$ ");
        line = shelly_read_line();
        //args = shelly_split_line(line);
        //status = shelly_execute(args);
        printf("top line: %s\n", line);
    } while (status);
}

char *shelly_read_line(void)
{
    int bufsize = SHELLY_BUF;
    char *buffer = (char *) malloc(sizeof(char) * bufsize);
    char *buf = buffer;
    int c; 

    if (!buffer) {
        fprintf(stderr, "shelly: allocation error\n");
        exit(1);
    }

    while ((c = getchar()) != EOF && c != '\n') {
        *buf++ = c;

        if(buf - buffer == bufsize) {
            bufsize += SHELLY_BUF;
            buffer = realloc(buffer, bufsize);

            if (!buffer) {
                fprintf(stderr, "shelly: allocation error\n");
                exit(1);
            }
        }
    }

    *buf = '\0';
    return buffer;
}
