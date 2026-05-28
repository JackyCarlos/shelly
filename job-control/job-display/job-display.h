/* job-display.h */

typedef struct job job_t;

typedef enum job_display_mode {
    JOB_DISPLAY_NORMAL,
    JOB_DISPLAY_FG,
    JOB_DISPLAY_BG,
    JOB_DISPLAY_NOTIFICATION
} job_display_mode_t;

void job_display_print_job(job_t *job);
void job_display_print_fg(job_t *job);
void job_display_print_bg(job_t *job);
void job_display_print_background_start(job_t *job);
void job_display_print_status_notification(job_t *job);
