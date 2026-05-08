#include "../common/command_flags.h"

typedef struct execution_context execution_context_t;

typedef struct job_command {
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
    int pgid;
    int is_background;
    
    int job_cmd_counter;
    job_command_t *job_commands;
    int job_cmds_size;
} job_t;

extern job_t *job_list;

void init_job_control(void);
int add_background_job_command(execution_context_t *context);
int job_complete(int job_id);
