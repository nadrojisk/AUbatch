#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define CMD_BUF_SIZE 10 /* The size of the command queue */
#define MAX_CMD_LEN 512 /* The longest scheduler length */

enum scheduling_policies
{
    FCFS,
    SJF,
    PRIORITY,
} policy;

typedef struct
{
    char cmd[MAX_CMD_LEN];
    time_t arrival_time;
    int cpu_burst;
    int cpu_remaining_burst;
    int priority;
    int interruptions;
    int first_time_on_cpu;

} process_t;

typedef struct
{
    char cmd[MAX_CMD_LEN];
    time_t arrival_time;
    int cpu_burst;
    int first_time_on_cpu;
    int priority;
    int interruptions;
    int finish_time;
    int turnaround_time;
    int waiting_time;
    int response_time;

} finished_process_t;

// Custom types
typedef process_t *process_p;
typedef finished_process_t *finished_process_p;
typedef unsigned int u_int;

// Scheduler and dispatch prototypes
void test_scheduler(char *benchmark, int num_of_jobs, int arrival_rate, int priority_levels, int min_CPU_time, int max_CPU_time); /* To simulate batch job submission and scheduling */
void scheduler(int argc, char **argv);                                                                                            /* To simulate job submissions and scheduling */
void *dispatcher(void *ptr);                                                                                                      /* To simulate job execution */

// sorting prototypes
void sort_buffer(process_p *process_buffer);          /* sorts process buffer depending on scheduler */
int sjf_scheduler(const void *a, const void *b);      /* sorts buffer by remaining cpu burst */
int fcfs_scheduler(const void *a, const void *b);     /* sorts buffer by arrival time */
int priority_scheduler(const void *a, const void *b); /* sorts buffer by priority */

// process functions
process_p get_process(char **argv);       /* returns new process_p based on input args */
int run_process(int burst);               /* sleeps for burst seconds */
void complete_process(process_p process); /* copys process to completed process buffer */
void submit_job(const char *cmd);         /* submits job to be ran */

void report_metrics(); /* loops through completed process buffer and prints metrics */

// utility functions
char *convert_time(time_t time);   /* convers from epoch time to human readable string */
void remove_newline(char *buffer); /* pulls newline off of string read from user input*/
char *get_policy_string();         /* returns a human readable string of the current scheduling policy */
int calculate_wait();              /* calculate wait time for the process that was just added */

/* Global shared variables */
u_int buf_head;      /* points to the next free slot in process_buffer */
u_int buf_tail;      /* points to the next process to be loaded onto the cpu */
u_int count;         /* the number of waiting processes */
u_int finished_head; /* points to the next free slot in the finished process buffer */
u_int batch;

process_p running_process;                        /* running process */
process_p process_buffer[CMD_BUF_SIZE];           /* buffer of waiting processes */
finished_process_p finished_process_buffer[8192]; /* buffer of finished processes*/

pthread_mutex_t cmd_queue_lock;   /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full;  /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */
