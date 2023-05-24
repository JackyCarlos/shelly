#define SHELLY_BUF 128

void shelly_loop(void);
char *shelly_read_line(void);
char **shelly_split_line(char *);
int shelly_execute(char **);