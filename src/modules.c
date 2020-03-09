/* 
 * COMP7500/7506
 * Project 3: modules 
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * Provides implemenation for the scheduling module and the dispatching module
 *
 * Compilation Instruction: 
 * gcc -o aubatch.out aubatch.c commandline.c modules.c -lpthread -Wall
 *
 */

#include "modules.h"

/*
 * This function takes in arguments from command line when users select test
 * 
 * Simulates a benchmark against a certain scheduling algorithm which is picked in command line
 * Can take in a number of jobs at once and can load them all at once
 * 
 * If arrival rate is > 0 then the jobs are loaded, dispatcher is notified and then we sleep for arrival_time seconds
 * After sleeping we load the next job, if there is one
 */
void test_scheduler(char *benchmark, int num_of_jobs, int arrival_rate, int priority_levels, int min_CPU_time, int max_CPU_time)
{
    if (!arrival_rate)
        batch = 1;
    else
        batch = 0;

    // create jobs based on num_of_jobs
    int i;
    for (i = 0; i < num_of_jobs; i++)
    {
        if (i >= CMD_BUF_SIZE)
        // if i is larger than cmd_buff we need to notify dispatcher earlier
        // without this we would be stuck forever
        {
            pthread_cond_signal(&cmd_buf_not_empty);
        }

        /* lock the shared command queue */
        pthread_mutex_lock(&cmd_queue_lock);

        while (count == CMD_BUF_SIZE)
        {
            pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
        }

        pthread_mutex_unlock(&cmd_queue_lock);

        process_p process = malloc(sizeof(process_t));

        int priority = (rand() % (priority_levels + 1)) + 1;
        int cpu_burst = (rand() % (max_CPU_time + 1)) + min_CPU_time;
        strcpy(process->cmd, "./microbatch.out");
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
        // sort before moding buf_head, without this if you were to sort CMD_BUF_SIZE items it would break
        sort_buffer(process_buffer);
        buf_head %= CMD_BUF_SIZE;

        pthread_mutex_unlock(&cmd_queue_lock);

        if (arrival_rate)
        {
            // if there is an arrival rate, notify dispatcher immediately and then sleep for arrival_rate
            pthread_cond_signal(&cmd_buf_not_empty);
            sleep(arrival_rate); // wait for the arrival rate
        }
    }
    if (!arrival_rate) // if arrival rate is 0, load all the jobs and then notify dispatcher
    {
        /* Unlock the shared command queue */
        pthread_cond_signal(&cmd_buf_not_empty);
    }
}
/* 
 * This function takes in arguments from command line when users select run
 * 
 * Takes in one job / process at a time and notifies dispatcher there is a new job
 */
void scheduler(int argc, char **argv)
{
    /* lock the shared command queue */
    pthread_mutex_lock(&cmd_queue_lock);

    while (count == CMD_BUF_SIZE)
    {
        pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
    }

    pthread_mutex_unlock(&cmd_queue_lock);
    process_p process = get_process(argv);

    // print information about job
    submit_job(process->cmd);

    process_buffer[buf_head] = process;
    pthread_mutex_lock(&cmd_queue_lock);

    count++;

    /* Move buf_head forward, this is a circular queue */
    buf_head++;
    // ensure buffer is in accordance to current policy
    sort_buffer(process_buffer);
    buf_head %= CMD_BUF_SIZE;

    /* Unlock the shared command queue */
    pthread_cond_signal(&cmd_buf_not_empty);
    pthread_mutex_unlock(&cmd_queue_lock);
}

/*
 * Runs jobs on the process_buffer queue. After job is completed the job is sent to the finished_buffer queue
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

        running_process = NULL;
    }
    return (void *)NULL;
}

/*
 * Calculates the estimated wait time for the newly added process 
 * to be loaded onto the CPU
 */
int calculate_wait()
{
    int wait = 0;
    int i;
    for (i = buf_tail; i < buf_head; i++)
    {
        wait += process_buffer[i]->cpu_remaining_burst;
    }
    return wait;
}

/*
 * Loads process via argv, this is called when `run` is specified in the command line
 */
process_p get_process(char **argv)
{
    process_p process = malloc(sizeof(process_t));
    remove_newline(argv[3]);

    // load process structure
    strcpy(process->cmd, argv[1]);
    process->arrival_time = time(NULL);
    process->cpu_burst = atoi(argv[2]);
    process->cpu_remaining_burst = process->cpu_burst;
    process->priority = atoi(argv[3]);
    process->interruptions = 0;
    process->first_time_on_cpu = 0;
    return process;
}

/*
 * Finishes a process. Runs the process using system and loads it into the
 * finished_process buffer. Setting all the correct values as needed.
 */
void complete_process(process_p process)
{
    char cmd[MAX_CMD_LEN * 2];
    if (!strcmp(process->cmd, "./microbatch.out"))
        sprintf(cmd, "%s %d", process->cmd, process->cpu_remaining_burst);
    else
        sprintf(cmd, "%s > /dev/null", process->cmd);

    if (process->first_time_on_cpu == 0)
        process->first_time_on_cpu = time(NULL);

    system(cmd);

    process->cpu_remaining_burst = 0;

    finished_process_p finished_process = malloc(sizeof(finished_process_t));
    finished_process->finish_time = time(NULL);

    //allows more accurate cpu burst, if we run ls 10 1, ls wont actually run for 10 seconds, therefore we need to update its burst time
    process->cpu_burst = (int)(finished_process->finish_time - process->first_time_on_cpu);

    strcpy(finished_process->cmd, process->cmd);
    finished_process->arrival_time = process->arrival_time;
    finished_process->cpu_burst = process->cpu_burst;
    finished_process->interruptions = process->interruptions;
    finished_process->priority = process->priority;
    finished_process->first_time_on_cpu = process->first_time_on_cpu;
    finished_process->turnaround_time = finished_process->finish_time - finished_process->arrival_time;
    if (finished_process->turnaround_time)
        finished_process->waiting_time = finished_process->turnaround_time - finished_process->cpu_burst;
    else
        finished_process->waiting_time = 0;

    finished_process->response_time = finished_process->first_time_on_cpu - finished_process->arrival_time;

    finished_process_buffer[finished_head] = finished_process;
    finished_head++;

    free(process);
}

/*
 * Reports job / process metrics. If no jobs are completed then notify user and return
 * 
 * Else dump all metrics per job and overall metrics
 */
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

    printf("\n=== Reporting Metrics for %s ===\n\n", get_policy_string());
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

/*
 * Sorts buffer per scheduling policy
 */
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

    // only sort buf_tail and more, if we sort at the base of process_buffer we will
    // sort processes that are currently running on the cpu
    int index;

    // if we are doing a batch job, aka arrival rate is not 0 then add 1 to buf_tail
    // if we sort ahead of buf_tail for a batch job we will all the processes even tho
    // none are currently on the cpu
    if (!batch)
        index = buf_tail + 1;
    else
        index = buf_tail;
    qsort(&process_buffer[index], buf_head - index, sizeof(process_p), sort);
}

/*
 * SJF sorting algorithm used by qsort
 */
int sjf_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->cpu_remaining_burst - process_b->cpu_remaining_burst);
}

/*
 * FCFS sorting algorithm used by qsort
 */
int fcfs_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (process_a->arrival_time - process_b->arrival_time);
}

/*
 * Priority Based sorting algorithm used by qsort
 */
int priority_scheduler(const void *a, const void *b)
{

    process_p process_a = *(process_p *)a;
    process_p process_b = *(process_p *)b;

    return (-process_a->priority + process_b->priority);
}

/*
 * Utility function that removes new line from end of buffer
 * 
 * When using fgets or any other function the new line is part of the buffer
 */
void remove_newline(char *buffer)
{
    int string_length = strlen(buffer);
    if (buffer[string_length - 1] == '\n')
    {
        buffer[string_length - 1] = '\0';
    }
}

/*
 * converts epoch time into a human readable format
 */
char *convert_time(time_t time)
{
    return asctime(localtime(&time));
}

/*
 * Returns a human readbale string of the current policy
 */
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

/*
 * After a job is submitted print information about the current queue
 */
void submit_job(const char *cmd)
{
    const char *str_policy = get_policy_string();
    printf("Job %s was submitted.\n", cmd);
    printf("Total number of jobs in the queue: %d\n", count + 1);
    printf("Expected waiting time: %d\n",
           calculate_wait());
    printf("Scheduling Policy: %s.\n", str_policy);
}