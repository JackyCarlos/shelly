#include "../parser/parser.h"

typedef struct job_command {
    int status; // 

    char **tokens;
    int argc;

    redirect_flags flags;
    char *input_file;
    char *output_file;
    char *append_file;

    int pid;
    int return_value;

} job_command_t;

typedef struct job {
    int id;
    int job_cmd_counter;

    job_command_t *job_commands;
    int job_cmds_size;
} job_t;



void init_job_control(void);
void add_background_job_command(execution_context_t *context);
