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

#include "aubatch.h"

pthread_mutex_t cmd_queue_lock;   /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full;  /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */

u_int count;
u_int finished_head;

int from_file;

finished_process_p finished_process_buffer[8192];

/* 
 * This function simulates a terminal where users may 
 * submit jobs into a batch processing queue.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */

void scheduler(int argc, char **argv)
{
    /* lock the shared command queue */
    pthread_mutex_lock(&cmd_queue_lock);
#ifdef VERBOSE
    printf("In scheduler: count = %d\n", count);
#endif
    while (count == CMD_BUF_SIZE)
    {
        pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
    }

    pthread_mutex_unlock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading
    if (argc == 4)
        process_buffer[buf_head] = get_process(argv);
    else
        process_buffer[buf_head] = get_process_from_file(argv, buf_head);

    pthread_mutex_lock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading
#ifdef VERBOSE
    printf("In scheduler: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]->cmd);
#endif
    count++;

    /* Move buf_head forward, this is a circular queue */
    buf_head++;
    buf_head %= CMD_BUF_SIZE;

    sort_buffer(process_buffer);

    /* Unlock the shared command queue */

    pthread_cond_signal(&cmd_buf_not_empty);
    pthread_mutex_unlock(&cmd_queue_lock);
}

process_p get_process(char **argv)
{
    process_p process = malloc(sizeof(process_t));
    remove_newline(argv[3]);

    // load process structure
    strcpy(process->cmd, argv[1]);
    process->arrival_time = time(NULL);
    process->cpu_burst = atoi(argv[2]); // TODO
    process->cpu_remaining_burst = process->cpu_burst;
    process->priority = atoi(argv[3]); // TODO
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

process_p get_process_from_file(char **filename, int index)
{
    FILE *fp = fopen(filename[0], "r");
    if (fp == NULL)
    {
        printf("Error %s could not be found", filename[0]);
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
#ifdef VERBOSE
        printf("In dispatcher: count = %d\n", count);
#endif
        while (count == 0)
        {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }

        /* Run the command scheduled in the queue */
        count--;
#ifdef VERBOSE
        printf("In dispatcher: process_buffer[%d] = %s\n", buf_tail, process_buffer[buf_tail]->cmd);
#endif
        process_buffer[buf_tail]->first_time_on_cpu = time(NULL);

        process_p process = process_buffer[buf_tail];

        char *cmd = process->cmd;
        char *argv[] = {NULL};
        execv(cmd, argv);

        /* Move buf_tail forward, this is a circular queue */
        buf_tail++;
        buf_tail %= CMD_BUF_SIZE;

        /* Free the dynamically allocated memory for the buffer */
        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlock the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);

        complete_process(process);
    }
    return (void *)NULL;
}

void complete_process(process_p process)
{
    int burst = run_process(process->cpu_remaining_burst);
    process->cpu_remaining_burst -= burst;

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

    free(process);
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
