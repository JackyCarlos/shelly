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

extern pid_t global_shell_pgid;
extern job_t *job_list;

void init_job_control(void);
int add_background_job_command(execution_context_t *context);
int job_control_after_launch(int job_id);
void foreground_job_wait(int job_id);

void job_control_set_pgid(int job_id, pid_t job_pgid);
void job_control_set_command_pid(int job_id, int pid);
void job_control_register_background_job(int job_id, int is_background);
void job_control_set_builtin_returnval(int job_id, int builtin_return);
