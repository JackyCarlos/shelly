#define SHELLY_BUF 1024

void shelly_loop(void);
char *shelly_read_line(void);
char **shelly_split_line(char *);
int shelly_execute(char **);