#define     SHELLY_BUF              128
#define     SHELLY_TOK_BUFSIZE      128
#define     SHELLY_TOK_DELIM        " \t\r\n\a"

void shelly_loop(void);
char *shelly_read_line(void);
char **shelly_split_line(char *);
int shelly_launch(char **);
int shelly_execute(char **);

// prototypes for shelly builtins
int shelly_cd(char **args);
int shelly_help(char **args);
int shelly_exit(char **args);

// prototypes for shelly helper function
int shelly_num_builtins(void);

extern char *builtin_str[];
extern int (*builtin_func[]) (char **);