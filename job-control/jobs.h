#include "../common/command_flags.h"

typedef struct execution_context execution_context_t;

typedef enum {
    RUNNING = 1,
    SUSPENDED,
    TERMINATED
} job_status;

typedef enum {
    CMD_RUNNING = 1,
    CMD_SUSPENDED,
    CMD_TERMINATED,
    CMD_FAILURE
} job_cmd_status;

typedef struct job_command {
    char **tokens;
    int argc;

    redirect_flags flags;
    char *input_file;
    char *output_file;
    char *append_file;
 
    int pid;
    job_cmd_status job_stat;
    int return_value;
} job_command_t;

typedef struct job {
    int id;
    int pgid;
    int is_background;
    job_status status;
    
    int job_cmd_counter;
    job_command_t *job_commands;
    int job_cmds_size;
} job_t;

extern pid_t global_shell_pgid;
extern job_t *job_list;
extern int job_list_size;

void init_job_control(void);
void job_control_add_job_command(job_t *job, execution_context_t *context, int is_builtin);
int job_control_after_launch(job_t *job);
void foreground_job_wait(job_t *job);
void reap_background_jobs(void);

int job_complete(job_t *job);
int job_running(job_t *job);

job_t *job_control_get_job(execution_context_t *context);
void job_control_set_pgid(job_t *job, pid_t job_pgid);
void job_control_set_command_pid(job_t *job, int pid);
void job_control_register_job(job_t *job, int is_background);
void job_control_set_builtin_returnval(job_t *job, int builtin_return);
