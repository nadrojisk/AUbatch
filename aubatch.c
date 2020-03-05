/* 
 * COMP7500/7506
 * Project 3: AUbatch 
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * This sample source code demonstrates the development of 
 * a batch-job scheduler using pthread.
 *
 * Compilation Instruction: 
 * gcc aubatch.c -o aubatch -lpthread
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef unsigned int u_int;

#define CMD_BUF_SIZE 10 /* The size of the command queueu */
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
/* 
 * When a job is submitted, the job must be compiled before it
 * is running by the dispatcher thread (see also dispatcher()).
 */
void print_message(char *ptr);
void *scheduler(void *ptr);  /* To simulate job submissions and scheduling */
void *dispatcher(void *ptr); /* To simulate job execution */
process_p first_come_first_serve_scheduler(process_p *process_buffer);
process_p shortest_job_first_scheduler(process_p *process_buffer);

void sort_buffer(process_p *process_buffer);
int sjf_sort(const void *a, const void *b);
int fcfs_sort(const void *a, const void *b);
int priority_sort(const void *a, const void *b);

process_p *copy_process_buffer(process_p *process_buffer);
process_p copy_process(process_p process);
process_p get_process(process_p *process_buffer, int index);
void remove_newline(char *buffer);
int run_process(int burst);

void complete_process(process_p process);
void report_metrics();
char *print_time(time_t time);

pthread_mutex_t cmd_queue_lock; /* Lock for critical sections */

pthread_cond_t cmd_buf_not_full;  /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int count;
u_int finished_head;

process_p process_buffer[CMD_BUF_SIZE];
finished_process_p finished_process_buffer[8192];

int main()
{
    srand(time(NULL));
    pthread_t scheduling_thread, dispatching_thread; /* Two concurrent threads */
    char *message1 = "Scheduling Thread";
    char *message2 = "Dispatching Thread";
    int iret1, iret2;

    policy = FCFS;

    /* Initilize count, two buffer pionters */
    count = 0;
    buf_head = 0;
    buf_tail = 0;
    finished_head = 0;

    /* Create two independent threads:command and dispatchers */
    iret1 = pthread_create(&scheduling_thread, NULL, scheduler, (void *)message1);
    iret2 = pthread_create(&dispatching_thread, NULL, dispatcher, (void *)message2);

    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(scheduling_thread, NULL);
    pthread_join(dispatching_thread, NULL);

    printf("scheduling_thread returns: %d\n", iret1);
    printf("dispatching_thread returns: %d\n", iret1);

    report_metrics();
    exit(0);
}

/* 
 * This function simulates a terminal where users may 
 * submit jobs into a batch processing queue.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *scheduler(void *ptr)
{

    char *cmd;
    u_int i;
    char num_str[8];
    size_t command_size;

    print_message((char *)ptr);

    /* Enter multiple commands in the queue to be scheduled */
    for (i = 0; i < NUM_OF_CMD; i++)
    {

        /* lock the shared command queue */
        pthread_mutex_lock(&cmd_queue_lock);
        printf("In scheduler: count = %d\n", count);

        while (count == CMD_BUF_SIZE)
        {
            pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
        }

        /* Dynamically create a buffer slot to hold a scheduler */

        // pthread_mutex_unlock(&cmd_queue_lock);
        process_p process = malloc(sizeof(process_t));

        printf("Please submit a batch processing job:\n");
        printf(">");

        char cmd_string[MAX_CMD_LEN];
        //fgets(&cmd_string, MAX_CMD_LEN, stdin); // why is this incorrect? i get a incompatible pointer type warning?
        fgets(cmd_string, MAX_CMD_LEN, stdin);
        remove_newline(cmd_string);

        // load process structure
        strcpy(process->cmd, cmd_string);
        process->arrival_time = time(NULL);
        process->cpu_burst = rand() % 9 + 1; // TODO
        process->cpu_remaining_burst = process->cpu_burst;
        process->priority = rand() % 5; // TODO
        process->interruptions = 0;

        process_buffer[buf_head] = process;

        // pthread_mutex_lock(&cmd_queue_lock);

        printf("In scheduler: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]->cmd);

        count++;

        /* Move buf_head forward, this is a circular queue */
        buf_head++;
        buf_head %= CMD_BUF_SIZE;

        sort_buffer(process_buffer);

        /* Unlock the shared command queue */

        // pthread_mutex_unlock(&cmd_queue_lock);
        // pthread_cond_signal(&cmd_buf_not_empty);

        pthread_cond_signal(&cmd_buf_not_empty);
        pthread_mutex_unlock(&cmd_queue_lock);
    }
}

/*
 * This function simulates a server running jobs in a batch mode.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *dispatcher(void *ptr)
{

    u_int i;

    print_message((char *)ptr);

    for (i = 0; i < NUM_OF_CMD; i++)
    {

        /* lock and unlock for the shared process queue */
        pthread_mutex_lock(&cmd_queue_lock);
        printf("In dispatcher: count = %d\n", count);

        while (count == 0)
        {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }

        /* Run the command scheduled in the queue */
        count--;

        process_p process = get_process(process_buffer, buf_tail);

        char *cmd = process->cmd;
        char *argv[] = {NULL};
        execv(cmd, argv);

        // TODO check if process is done, if not, put back on the buffer
        /* Free the dynamically allocated memory for the buffer */

        // fcfs example, can run and does not worry about getting booted off
        int burst = run_process(process->cpu_remaining_burst);
        // process->finish_time = time(NULL);
        process->cpu_remaining_burst -= burst;

        complete_process(process);

        /* Move buf_tail forward, this is a circular queue */
        buf_tail++;
        buf_tail %= CMD_BUF_SIZE;

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlok the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);
    }
}

void complete_process(process_p process)
{
    finished_process_p finished_process = malloc(sizeof(finished_process_t));
    finished_process->finish_time = time(NULL);
    strcpy(finished_process->cmd, process->cmd);
    finished_process->arrival_time = process->arrival_time;
    finished_process->cpu_burst = process->cpu_burst;
    finished_process->interruptions = process->interruptions;
    finished_process->priority = process->priority;
    finished_process->first_time_on_cpu = process->first_time_on_cpu;
    finished_process->turnaround_time = finished_process->finish_time - finished_process->arrival_time;
    finished_process->waiting_time = finished_process->turnaround_time - finished_process->cpu_burst;
    finished_process->response_time = finished_process->first_time_on_cpu - finished_process->arrival_time;

    finished_process_buffer[finished_head] = finished_process;
    finished_head++;
}

void report_metrics()
{
    char str_policy[32];
    switch (policy)
    {
    case FCFS:
        strcpy(str_policy, "First Come First Serve");
        break;
    case SJF:
        strcpy(str_policy, "Shortest Job First");
        break;
    case PRIORITY:
        strcpy(str_policy, "Priority");
        break;
    }

    printf("\n=== Reporting Metrics for %s ===\n\n", str_policy);
    finished_process_p finished_process;
    for (int i = 0; i < finished_head; i++)
    {
        finished_process = finished_process_buffer[i];

        printf("Metrics for %s:\n", finished_process->cmd);
        printf("\tCPU Burst:         %d\n", finished_process->cpu_burst);
        printf("\tInterruptions:     %d\n", finished_process->interruptions);
        printf("\tPriority:          %d\n", finished_process->priority);

        printf("\tArrival Time:      %s", print_time(finished_process->arrival_time));
        printf("\tFirst Time on CPU: %s", print_time(finished_process->first_time_on_cpu));
        printf("\tFinish Time:       %s", print_time(finished_process->finish_time));

        printf("\tTurnaround Time:   %d\n", finished_process->turnaround_time);
        printf("\tWaiting Time:      %d\n", finished_process->waiting_time);
        printf("\tResponse Time:     %d\n", finished_process->response_time);
        printf("\n");
    }
}
void print_message(char *ptr)
{
    printf("%s \n", ptr);
}

process_p get_process(process_p *process_buffer, int index)
{
    printf("In dispatcher: process_buffer[%d] = %s\n", index, process_buffer[index]->cmd);
    process_buffer[index]->first_time_on_cpu = time(NULL);
    return process_buffer[index];
}

void print(process_p *process_buffer)
{
    for (int i = 0; i < buf_head; i++)
    {
        printf("Process: %s, remaining burst: %d\n", process_buffer[i]->cmd, process_buffer[i]->cpu_remaining_burst);
    }
}

void sort_buffer(process_p *process_buffer)
{
    void *sort;
    switch (policy)
    {
    case FCFS:
        sort = fcfs_sort;
        break;
    case SJF:
        sort = sjf_sort;
        break;
    case PRIORITY:
        sort = priority_sort;
    }
    qsort(process_buffer, buf_head, sizeof(process_p), sort);
}

int sjf_sort(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->cpu_remaining_burst - process_b->cpu_remaining_burst);
}

int fcfs_sort(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->arrival_time - process_b->arrival_time);
}

int priority_sort(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->priority - process_b->priority);
}

process_p *copy_process_buffer(process_p *process_buffer)
{
    process_p *new_process_buffer = malloc(sizeof(new_process_buffer) * buf_head);
    for (int i = 0; i < buf_head; i++)
    {
        process_p tmp = process_buffer[i];
        process_p new_process = copy_process(tmp);
        new_process_buffer[i] = new_process;
    }
    return new_process_buffer;
}

process_p copy_process(process_p process)
{
    process_p new_process = malloc(sizeof(new_process));
    strcpy(new_process->cmd, process->cmd);
    new_process->arrival_time = process->arrival_time;
    new_process->cpu_burst = process->cpu_burst;
    new_process->cpu_remaining_burst = process->cpu_remaining_burst;
    new_process->interruptions = process->interruptions;
    new_process->priority = process->priority;

    return new_process;
}

int run_process(int burst)
{
    sleep(burst);
    return burst;
}

void remove_newline(char *buffer)
{
    int string_length = strlen(buffer);
    if (buffer[string_length - 1] == '\n')
    {
        buffer[string_length - 1] = '\0';
    }
}

char *print_time(time_t time)
{
    return asctime(localtime(&time));
}
