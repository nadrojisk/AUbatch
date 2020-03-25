/* 
 * COMP7500/7506
 * Project 3: AUbatch 
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * The main driver for aubatch, sets up all the necessary variables
 * for the program to work properly. Creates two threads, executor and disaptcher
 *
 * Compilation Instruction: 
 * gcc -o aubatch.out aubatch.c commandline.c modules.c -lpthread -Wall
 *
 */

#include "commandline.h"
#include "modules.h"

int main(int argc, char **argv)
{
    printf("Welcome to Jordan Sosnowski's batch job scheduler Version 1.0.\nType 'help' to find more about AUbatch commands.\n");
    pthread_t executor_thread, dispatcher_thread; /* Two concurrent threads */

    int iret1, iret2;

    policy = FCFS; // default policy for scheduler

    /* Initialize count, three buffer pointers */
    count = 0;
    buf_head = 0;
    buf_tail = 0;
    finished_head = 0;
    batch = 0;

    /* Create two independent threads: executor and dispatcher */

    iret1 = pthread_create(&executor_thread, NULL, commandline, (void *)NULL);
    iret2 = pthread_create(&dispatcher_thread, NULL, dispatcher, (void *)NULL);

    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(executor_thread, NULL);
    pthread_join(dispatcher_thread, NULL);

    if (iret1)
        printf("executor_thread returns: %d\n", iret1);
    if (iret2)
        printf("dispatcher_thread returns: %d\n", iret1);
    return 0;
}