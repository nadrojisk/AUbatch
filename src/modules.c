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

#include "modules.h"

/* Global shared variables */

process_p running_process;

void test_scheduler(char *benchmark, int num_of_jobs, int arrival_rate, int priority_levels, int min_CPU_time, int max_CPU_time)
{
    // TODO check if queue is empty first
    /* lock the shared command queue */
    pthread_mutex_lock(&cmd_queue_lock);

    // printf("In scheduler: count = %d\n", count);

    while (count == CMD_BUF_SIZE)
    {
        pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
    }

    pthread_mutex_unlock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading

    for (int i = 0; i < num_of_jobs; i++)
    {
        int priority = (rand() % (priority_levels + 1)) + 1;
        int cpu_burst = (rand() % (max_CPU_time + 1)) + min_CPU_time;
        process_p process = malloc(sizeof(process_t));
        strcpy(process->cmd, "microbatch.out");
        process->arrival_time = time(NULL);
        process->cpu_burst = cpu_burst;
        process->cpu_remaining_burst = cpu_burst;
        process->priority = priority;
        process->interruptions = 0;
        process->first_time_on_cpu = 0;
        process_buffer[buf_head] = process;
        count++;

        /* Move buf_head forward, this is a circular queue */
        buf_head++;
        buf_head %= CMD_BUF_SIZE;

        if (arrival_rate) // if there is an arrival rate, notify dispatcher immediately and then sleep for arrival_rate
        {
            pthread_mutex_lock(&cmd_queue_lock);

            sort_buffer(process_buffer);

            /* Unlock the shared command queue */

            pthread_cond_signal(&cmd_buf_not_empty);
            pthread_mutex_unlock(&cmd_queue_lock);
            sleep(arrival_rate);
        }
    }
    if (!arrival_rate) // if arrival rate is 0, load all the jobs and then notify dispatcher
    {
        pthread_mutex_lock(&cmd_queue_lock);

        sort_buffer(process_buffer);

        /* Unlock the shared command queue */

        pthread_cond_signal(&cmd_buf_not_empty);
        pthread_mutex_unlock(&cmd_queue_lock);
    }
}
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

    // printf("In scheduler: count = %d\n", count);

    while (count == CMD_BUF_SIZE)
    {
        pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
    }

    pthread_mutex_unlock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading
    process_buffer[buf_head] = get_process(argv);

    pthread_mutex_lock(&cmd_queue_lock); //uncomment if you want the dispatcher to run while scheduler is loading

    // printf("In scheduler: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]->cmd);

    count++;

    /* Move buf_head forward, this is a circular queue */
    buf_head++;
    buf_head %= CMD_BUF_SIZE;

    submit_job(process_buffer[buf_head - 1]->cmd);

    sort_buffer(process_buffer);

    /* Unlock the shared command queue */

    pthread_cond_signal(&cmd_buf_not_empty);
    pthread_mutex_unlock(&cmd_queue_lock);
}

char *get_policy_string()
{

    switch (policy)
    {
    case FCFS:
        return "FCFS";

    case SJF:
        return "SJF";

    case PRIORITY:
        return "Priority";

    default:
        return "Unknown";
    }
}
void submit_job(const char *cmd)
{
    const char *str_policy = get_policy_string();
    printf("Job %s was submitted.\n", cmd);
    printf("Total number of jobs in the queue: %d\n", buf_head - buf_tail);
    printf("Expected waiting time: %d\n",
           calculate_wait());
    printf("Scheduling Policy: %s.\n", str_policy);
}

int calculate_wait()
{
    int wait = 0;
    for (int i = buf_tail; i < buf_head; i++)
    {
        wait += process_buffer[i]->cpu_remaining_burst;
    }
    if (running_process != NULL)
        wait += running_process->cpu_remaining_burst;
    return wait;
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
    process->first_time_on_cpu = 0;
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

/*
 * This function simulates a server running jobs in a batch mode.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *dispatcher(void *ptr)
{

    while (1)
    {

        /* lock and unlock for the shared process queue */
        pthread_mutex_lock(&cmd_queue_lock);

        // printf("In dispatcher: count = %d\n", count);

        while (count == 0)
        {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }
        running_process = process_buffer[buf_tail];

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlock the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);

        complete_process(running_process);
        /* Run the command scheduled in the queue */
        count--;

        // printf("In dispatcher: process_buffer[%d] = %s\n", buf_tail, process_buffer[buf_tail]->cmd);

        /* Move buf_tail forward, this is a circular queue */
        buf_tail++;
        buf_tail %= CMD_BUF_SIZE;
    }
    return (void *)NULL;
}

void complete_process(process_p process)
{
    char cmd[MAX_CMD_LEN * 2];
    sprintf(cmd, "./src/%s %d", process->cmd, process->cpu_remaining_burst);

    if (process->first_time_on_cpu == 0)
        process->first_time_on_cpu = time(NULL);

    system(cmd);

    // int burst = run_process(process->cpu_remaining_burst);
    process->cpu_remaining_burst = 0;

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

    // free(process);
}

void report_metrics()
{
    if (!finished_head)
    {
        printf("No jobs completed!\n");
        return;
    }
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_cpu_burst = 0;

    int max_waiting_time = INT_MIN;
    int min_waiting_time = INT_MAX;
    int max_response_time = INT_MIN;
    int min_response_time = INT_MAX;
    int max_turnaround_time = INT_MIN;
    int min_turnaround_time = INT_MAX;
    int max_cpu_burst = INT_MIN;
    int min_cpu_burst = INT_MAX;

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

        printf("Metrics for job %s:\n", finished_process->cmd);
        printf("\tCPU Burst:           %d seconds\n", finished_process->cpu_burst);
        printf("\tInterruptions:       %d times\n", finished_process->interruptions);
        printf("\tPriority:            %d\n", finished_process->priority);

        printf("\tArrival Time:        %s", convert_time(finished_process->arrival_time));
        printf("\tFirst Time on CPU:   %s", convert_time(finished_process->first_time_on_cpu));
        printf("\tFinish Time:         %s", convert_time(finished_process->finish_time));

        printf("\tTurnaround Time:     %d seconds\n", finished_process->turnaround_time);
        printf("\tWaiting Time:        %d seconds\n", finished_process->waiting_time);
        printf("\tResponse Time:       %d seconds\n", finished_process->response_time);
        printf("\n");

        if (finished_process->waiting_time < min_waiting_time)
            min_waiting_time = finished_process->waiting_time;
        if (finished_process->turnaround_time < min_turnaround_time)
            min_turnaround_time = finished_process->turnaround_time;
        if (finished_process->response_time < min_response_time)
            min_response_time = finished_process->response_time;
        if (finished_process->cpu_burst < min_cpu_burst)
            min_cpu_burst = finished_process->cpu_burst;

        if (finished_process->waiting_time > max_waiting_time)
            max_waiting_time = finished_process->waiting_time;
        if (finished_process->turnaround_time > max_response_time)
            max_turnaround_time = finished_process->turnaround_time;
        if (finished_process->response_time > max_response_time)
            max_response_time = finished_process->response_time;
        if (finished_process->cpu_burst > max_cpu_burst)
            max_cpu_burst = finished_process->cpu_burst;

        total_response_time += finished_process->response_time;
        total_waiting_time += finished_process->waiting_time;
        total_turnaround_time += finished_process->turnaround_time;
        total_cpu_burst += finished_process->cpu_burst;
    }

    printf("Overall Metrics for Batch:\n");
    printf("\tTotal Number of Jobs Completed: %d\n", finished_head);
    printf("\tTotal Number of Jobs Submitted: %d\n", finished_head + (buf_head - buf_tail));
    printf("\tAverage Turnaround Time:        %.3f seconds\n", total_turnaround_time / (float)i);
    printf("\tAverage Waiting Time:           %.3f seconds\n", total_waiting_time / (float)i);
    printf("\tAverage Response Time:          %.3f seconds\n", total_response_time / (float)i);
    printf("\tAverage CPU Burst:              %.3f seconds\n", total_cpu_burst / (float)i);
    printf("\tTotal CPU Burst:                %d seconds\n", total_cpu_burst);
    printf("\tThroughput:                     %.3f No./second\n", 1 / (total_turnaround_time / (float)i));

    printf("\tMax Turnaround Time:            %d seconds\n", max_turnaround_time);
    printf("\tMin Turnaround Time:            %d seconds\n\n", min_turnaround_time);

    printf("\tMax Waiting Time:               %d seconds\n", max_waiting_time);
    printf("\tMin Waiting Time:               %d seconds\n\n", min_waiting_time);

    printf("\tMax Response Time:              %d seconds\n", max_response_time);
    printf("\tMin Response Time:              %d seconds\n\n", min_response_time);

    printf("\tMax CPU Burst:                  %d seconds\n", max_cpu_burst);
    printf("\tMin CPU Burst:                  %d seconds\n\n", min_cpu_burst);
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
    //todo REMOVE PROCESSES BEING RAN

    qsort(&process_buffer[buf_tail], buf_head - buf_tail, sizeof(process_p), sort);
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
