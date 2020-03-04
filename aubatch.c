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

enum
{
    FCFS,
    SJF,
    PRIORITY,
} scheduling_policy;

typedef struct
{
    char cmd[MAX_CMD_LEN];
    time_t arrival_time;
    int cpu_burst;
    int cpu_remaining_burst;
    int priority;
    int interruptions;

} process_t;

typedef process_t *process_p;

/* 
 * When a job is submitted, the job must be compiled before it
 * is running by the dispatcher thread (see also dispatcher()).
 */
void print_message(char *ptr);
void *scheduler(void *ptr);  /* To simulate job submissions and scheduling */
void *dispatcher(void *ptr); /* To simulate job execution */
char *first_come_first_serve_scheduler(process_p *process_buffer);
char *shortest_job_first_scheduler(process_p *process_buffer);
int compare(const process_p x, const process_p y);
process_p *copy_process_buffer(process_p process_buffer);
process_p copy_process(process_p process);

pthread_mutex_t cmd_queue_lock; /* Lock for critical sections */

pthread_cond_t cmd_buf_not_full;  /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int count;
process_p process_buffer[CMD_BUF_SIZE];

int main()
{
    pthread_t scheduling_thread, dispatching_thread; /* Two concurrent threads */
    char *message1 = "Scheduling Thread";
    char *message2 = "Dispatching Thread";
    int iret1, iret2;

    /* Initilize count, two buffer pionters */
    count = 0;
    buf_head = 0;
    buf_tail = 0;

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

        while (count == CMD_BUF_SIZE)
        {
            pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
        }

        /* lock the shared command queue */
        pthread_mutex_lock(&cmd_queue_lock);

        printf("In scheduler: count = %d\n", count);

        /* Dynamically create a buffer slot to hold a scheduler */

        // pthread_mutex_unlock(&cmd_queue_lock);
        process_p process = malloc(sizeof(process));

        printf("Please submit a batch processing job:\n");
        printf(">");

        char cmd_string[MAX_CMD_LEN];
        fgets(&cmd_string, MAX_CMD_LEN, stdin);

        // load process structure
        strcpy(process->cmd, cmd_string);
        process->arrival_time = time(NULL);
        process->cpu_burst = rand(); // TODO
        process->cpu_remaining_burst = process->cpu_burst;
        process->priority = 0; // TODO
        process->interruptions = 0;

        process_buffer[buf_head] = process;

        // pthread_mutex_lock(&cmd_queue_lock);

        printf("In scheduler: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]->cmd);

        count++;

        /* Move buf_head forward, this is a circular queue */
        buf_head++;
        buf_head %= CMD_BUF_SIZE;

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

        while (count == 0)
        {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }

        /* lock and unlock for the shared process queue */
        pthread_mutex_lock(&cmd_queue_lock);
        printf("In dispatcher: count = %d\n", count);

        /* Run the command scheduled in the queue */
        count--;

        char *cmd;
        switch (scheduling_policy)
        {
        case FCFS:
            cmd = first_come_first_serve_scheduler(process_buffer);
            break;
        case SJF:
            cmd = shortest_job_first_scheduler(process_buffer);
            break;
        case PRIORITY:
            break;
        default:
            printf("In dispatcher: process_buffer[%d] = %s\n", buf_head, process_buffer[buf_head]);
            cmd = process_buffer[buf_head];
        }
        execv(cmd, NULL);
        /* Free the dynamically allocated memory for the buffer */
        free(cmd);

        /* Move buf_tail forward, this is a circular queue */
        buf_tail++;
        buf_tail %= CMD_BUF_SIZE;

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlok the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);
    }
}

void print_message(char *ptr)
{
    printf("%s \n", ptr);
}

process_p *get_process(process_p *process_buffer, int index)
{
    printf("In dispatcher: process_buffer[%d] = %s\n", index, process_buffer[index]);
    return process_buffer[index];
}
char *first_come_first_serve_scheduler(process_p *process_buffer)
{
    process_p *ready = get_process(process_buffer, buf_tail);
    process_buffer = process_buffer[buf_tail + 1];

    return ready;
}

char *shortest_job_first_scheduler(process_p *process_buffer)
{
    // printf("In dispatcher: process_buffer[%d] = %s\n", buf_tail, process_buffer[buf_tail]);
    process_p *sorted_buffer = copy_process_buffer(process_buffer);
    qsort(sorted_buffer, CMD_BUF_SIZE, sizeof(process_t), compare);

    printf("In dispatcher: process_buffer[%d] = %s\n", buf_tail, process_buffer[buf_tail]);

    char *ready = process_buffer[buf_tail];
    process_buffer = process_buffer[buf_tail + 1];
    return ready;
}

int compare(const process_p x, const process_p y)
{
    int a = x->cpu_remaining_burst;
    int b = y->cpu_remaining_burst;

    if (a < b)
        return -1;
    if (a > b)
        return 1;
    if (a == b)
        return 0;
}

process_p *copy_process_buffer(process_p process_buffer)
{
    process_p *new_process_buffer = malloc(sizeof(new_process_buffer));
    for (int i = 0; i < CMD_BUF_SIZE; i++)
    {
        process_p tmp = &process_buffer[i];
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
// ProcessControlBlock *SJF_Scheduler() {
//   /* Select Process with Shortest Remaining Time*/
//   ProcessControlBlock *minimumProcess = (ProcessControlBlock *) DequeueProcess(READYQUEUE);
//   if(minimumProcess){
//     ProcessControlBlock *compareProcess = DequeueProcess(READYQUEUE);
//     ProcessControlBlock *originalProcess = minimumProcess;    //add original process to use later for a check to see if loop is done
//     while(compareProcess){
//       if(compareProcess->RemainingCpuBurstTime < minimumProcess->RemainingCpuBurstTime){ //compare current process with relative min time process
//         EnqueueProcess(READYQUEUE, minimumProcess);  //makes sure to put process back on queue if its no long min
//         minimumProcess = compareProcess;
//       }
//       else{       //make sure to put compareProcess back on ready queue if not picked
//         EnqueueProcess(READYQUEUE, compareProcess);
//       }
//       if(originalProcess->ProcessID == compareProcess->ProcessID){   //add in check to make sure to not endlessly loop
//         if(minimumProcess->ProcessID != compareProcess->ProcessID){ //if the chosen process is not the current process put the current back on the queue
//           EnqueueProcess(READYQUEUE, compareProcess);
//         }
//         break;
//       }
//       compareProcess = DequeueProcess(READYQUEUE);
//     }
//   }
//   return(minimumProcess);
// }