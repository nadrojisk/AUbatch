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

#include "commandline.h"

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

int main(int argc, char **argv)
{
    pthread_t executor_thread, dispatching_thread; /* Two concurrent threads */
    char *message1 = "Executor Thread";
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

    iret1 = pthread_create(&executor_thread, NULL, commandline, (void *)message1);
    iret2 = pthread_create(&dispatching_thread, NULL, dispatcher, (void *)message2);

    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(executor_thread, NULL);
    pthread_join(dispatching_thread, NULL);

    if (iret1)
        printf("executor_thread returns: %d\n", iret1);
    if (iret2)
        printf("dispatching_thread returns: %d\n", iret1);

    report_metrics();
    exit(0);
}