#define     SHELLY_BUF              128
#define     SHELLY_TOK_BUFSIZE      128
#define     SHELLY_TOK_DELIM        " \t\r\n\a"

void shelly_loop(void);
char *shelly_read_line(void);
char **shelly_split_line(char *);
int shelly_execute(char **);
