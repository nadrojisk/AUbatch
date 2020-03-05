#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define CMD_BUF_SIZE 10 /* The size of the command queue */
#define NUM_OF_CMD 5    /* The number of submitted jobs   */
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

typedef process_t *process_p;
typedef finished_process_t *finished_process_p;

void *scheduler(void *ptr);  /* To simulate job submissions and scheduling */
void *dispatcher(void *ptr); /* To simulate job execution */

void sort_buffer(process_p *process_buffer);          /* sorts process buffer depending on scheduler */
int sjf_scheduler(const void *a, const void *b);      /* sorts buffer by remaining cpu burst */
int fcfs_scheduler(const void *a, const void *b);     /* sorts buffer by arrival time */
int priority_scheduler(const void *a, const void *b); /* sorts buffer by priority */

process_p get_process();
process_p get_process_from_file(char *filename, int index);
int run_process(int burst);               /* sleeps for burst seconds */
void complete_process(process_p process); /* copys process to completed process buffer */

void report_metrics(); /* loops through completed process buffer and prints metrics */

char *convert_time(time_t time);   /* convers from epoch time to human readable string */
void remove_newline(char *buffer); /* pulls newline off of string read from user input*/
