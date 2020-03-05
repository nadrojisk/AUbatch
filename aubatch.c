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
#include <limits.h>

typedef unsigned int u_int;

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

pthread_mutex_t cmd_queue_lock;   /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full;  /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int count;
u_int finished_head;

int from_file;

process_p process_buffer[CMD_BUF_SIZE];
finished_process_p finished_process_buffer[8192];

int main()
{
    srand(time(NULL));
    pthread_t scheduling_thread, dispatching_thread; /* Two concurrent threads */
    char *message1 = "Scheduling Thread";
    char *message2 = "Dispatching Thread";
    int iret1, iret2;

    policy = SJF; // policy for scheduler
    from_file = 1;

    /* Initialize count, two buffer pointers */
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

    if (iret1)
        printf("scheduling_thread returns: %d\n", iret1);
    if (iret2)
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
    printf("%s \n", (char *)ptr);

    /* Enter multiple commands in the queue to be scheduled */
    for (int i = 0; i < NUM_OF_CMD; i++)
    {

        /* lock the shared command queue */
        pthread_mutex_lock(&cmd_queue_lock);
        printf("In scheduler: count = %d\n", count);

        while (count == CMD_BUF_SIZE)
        {
            pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
        }

        // pthread_mutex_unlock(&cmd_queue_lock);  //uncomment if you want the dispatcher to run while scheduler is loading

        printf("Please submit a batch processing job:\n");
        printf(">");

        if (from_file)
            process_buffer[buf_head] = get_process_from_file("static.txt", i);
        else
            process_buffer[buf_head] = get_process();

        // pthread_mutex_lock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading

        printf("In scheduler: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]->cmd);

        count++;

        /* Move buf_head forward, this is a circular queue */
        buf_head++;
        buf_head %= CMD_BUF_SIZE;

        sort_buffer(process_buffer);

        /* Unlock the shared command queue */

        pthread_cond_signal(&cmd_buf_not_empty);
        pthread_mutex_unlock(&cmd_queue_lock);
    }
    return (void *)NULL;
}

process_p get_process()
{
    process_p process = malloc(sizeof(process_t));
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
    return process;
}

void skip_ahead(FILE *file, int index)
{
    int c = EOF;
    int linecount = 0;
    do
    {
        if (c == '\n')
            linecount++;
        if (linecount == index)
            break;
    } while ((c = fgetc(file)) != EOF);
}

process_p get_process_from_file(char *filename, int index)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Error %s could not be found", filename);
        exit(1);
    }

    skip_ahead(fp, index);

    char data[MAX_CMD_LEN];
    fscanf(fp, "%[^\n]s", data);

    char *tok = strtok(data, " ");

    char tokens[64][64] = {{0}};

    process_p process = malloc(sizeof(process_t));
    int i = 0;
    while (tok != NULL)
    {
        strcpy(tokens[i], tok);
        tok = strtok(NULL, " ");
        i++;
    }

    strcpy(process->cmd, tokens[0]);
    process->cpu_burst = atoi(tokens[1]);
    process->priority = atoi(tokens[2]);
    process->cpu_remaining_burst = process->cpu_burst;
    process->interruptions = 0;
    process->arrival_time = time(NULL);

    return process;
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

    printf("%s \n", (char *)ptr);

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

        printf("In dispatcher: process_buffer[%d] = %s\n", buf_tail, process_buffer[buf_tail]->cmd);
        process_buffer[buf_tail]->first_time_on_cpu = time(NULL);

        process_p process = process_buffer[buf_tail];

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
        free(process);

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlock the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);
    }
    return (void *)NULL;
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
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;

    int max_waiting_time = INT_MIN;
    int min_waiting_time = INT_MAX;
    int max_response_time = INT_MIN;
    int min_response_time = INT_MAX;
    int max_turnaround_time = INT_MIN;
    int min_turnaround_time = INT_MAX;

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
    int i = 0;
    for (; i < finished_head; i++)
    {
        finished_process = finished_process_buffer[i];

        printf("Metrics for %s:\n", finished_process->cmd);
        printf("\tCPU Burst:           %d\n", finished_process->cpu_burst);
        printf("\tInterruptions:       %d\n", finished_process->interruptions);
        printf("\tPriority:            %d\n", finished_process->priority);

        printf("\tArrival Time:        %s", convert_time(finished_process->arrival_time));
        printf("\tFirst Time on CPU:   %s", convert_time(finished_process->first_time_on_cpu));
        printf("\tFinish Time:         %s", convert_time(finished_process->finish_time));

        printf("\tTurnaround Time:     %d\n", finished_process->turnaround_time);
        printf("\tWaiting Time:        %d\n", finished_process->waiting_time);
        printf("\tResponse Time:       %d\n", finished_process->response_time);
        printf("\n");

        if (finished_process->waiting_time < min_waiting_time)
            min_waiting_time = finished_process->waiting_time;
        if (finished_process->turnaround_time < min_turnaround_time)
            min_turnaround_time = finished_process->turnaround_time;
        if (finished_process->response_time < min_response_time)
            min_response_time = finished_process->response_time;

        if (finished_process->waiting_time > max_waiting_time)
            max_waiting_time = finished_process->waiting_time;
        if (finished_process->turnaround_time > max_response_time)
            max_turnaround_time = finished_process->turnaround_time;
        if (finished_process->response_time > max_response_time)
            max_response_time = finished_process->response_time;

        total_response_time += finished_process->response_time;
        total_waiting_time += finished_process->waiting_time;
        total_turnaround_time += finished_process->turnaround_time;
    }

    printf("Overall Metrics for Batch:\n");
    printf("\tAverage Turnaround Time: %f\n", total_turnaround_time / (float)i);
    printf("\tAverage Waiting Time:    %f\n", total_waiting_time / (float)i);
    printf("\tAverage Response Time:   %f\n\n", total_response_time / (float)i);

    printf("\tMax Turnaround Time:     %d\n", max_turnaround_time);
    printf("\tMin Turnaround Time:     %d\n\n", min_turnaround_time);

    printf("\tMax Waiting Time:        %d\n", max_waiting_time);
    printf("\tMin Waiting Time:        %d\n\n", min_waiting_time);

    printf("\tMax Response Time:       %d\n", max_response_time);
    printf("\tMin Response Time:       %d\n\n", min_response_time);
}

void sort_buffer(process_p *process_buffer)
{
    void *sort;
    switch (policy)
    {
    case FCFS:
        sort = fcfs_scheduler;
        break;
    case SJF:
        sort = sjf_scheduler;
        break;
    case PRIORITY:
        sort = priority_scheduler;
    }
    qsort(process_buffer, buf_head, sizeof(process_p), sort);
}

int sjf_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->cpu_remaining_burst - process_b->cpu_remaining_burst);
}

int fcfs_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->arrival_time - process_b->arrival_time);
}

int priority_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->priority - process_b->priority);
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

char *convert_time(time_t time)
{
    return asctime(localtime(&time));
}

void print_process(process_p *process_buffer)
{
    for (int i = 0; i < buf_head; i++)
    {
        printf("Process: %s, remaining burst: %d\n", process_buffer[i]->cmd, process_buffer[i]->cpu_remaining_burst);
    }
}
