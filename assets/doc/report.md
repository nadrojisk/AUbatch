---
title: "Project 3: AUBatch Report"
author: [Jordan Sosnowski]
date: "2020-3-9"
subject: "AUBatch"
keywords: [c, commandline, cpu scheduling, algorithms, fcfs, sjf, priority]
lang: "en"
titlepage: "true"
titlepage-rule-height: "0"
titlepage-background: "./background2.pdf"
---
<!-- `pandoc report.md -o project.pdf --from markdown --template eisvogel --listing --toc` -->

/newpage
# Introduction

## I. Problem Description

Central processing units are the core of any computer.
Any program that has to run has to go through the CPU, as without the CPU the program cannot be executed.
However, one single program cannot fully utilize a CPU; therefore, if we were to leave a single program on the CPU until it is finished executing we would be wasting valuable time.
Imagine if you could only run one program at a time per CPU on a computer, that would be horrendous.
Therefore, it is important to keep a CPU as active as possible.
For example, if one program is busing doing I/O it should probably be booted off the CPU so a program that can actually use the CPU's resources can be loaded.
But which program should be loaded next?
AUBatch is a simulation that looks into CPU process, or job, scheduling. We look into three algorithms: first come, first served, shortest job first, and priority based.

We assume all of our algorithms are non-preemptive, so once a process is loaded onto the CPU it is there until it is completed. 
Preemptive algorithms are extremely popular and efficient, as state earlier, but to implement one is out of scope for this current project.

## II. Background

To fully understand some of the processes and applications discussed in this paper, a background in these methodologies needs to be established.

### 1. Central Processing Unit

A central processing unit (CPU)^[1]^ is hardware that executes instructions that make up a computer program.
Also referred to as the brain of the computer, without it the computer would not be able to operate.
Most modern CPUs have multiple cores, each core can load a single thread of execution. Therefore a CPU with two cores can run two parallel processes.

### First Come, First Served

First come, first served (FCFS)^[2]^ is a scheduling algorithm that loads processes onto the CPU as they arrive.
Therefore, if three processes arrive in the following order A, C, B they will execute in the same order.

### Shortest Job First

Shortest job first (SJF)^[3]^, also known as shortest job next, loads processes based on the remaining CPU burst time. 
This scheduler minimizes responses time as jobs are usually loaded faster.

### Priority Based

Priority based scheduling is similar to SJF. Instead of sorting by remaining CPU burst, it will sort based on priority (highest priority first, lowest last).
Our implementation of priority based is non-preemptive, most are preemptive.


# Code Walkthrough

## I. AUBatch

The first module we will discuss is aubatch.c.
It is the driver for the whole framework, and the only module with a main function.

```c++
#include "commandline.h"
#include "modules.h"
```
We first include `commandline.h` and `modules.h` so we can gain the functionality of `commandline.c` and `modules.c`.

```c++
int main(int argc, char **argv)
{
    printf("Welcome to Jordan Sosnowski's batch job scheduler Version 1.0.\nType ‘help’ to find more about AUbatch commands.\n");
    pthread_t executor_thread, dispatcher_thread; /* Two concurrent threads */

    int iret1, iret2;

    policy = FCFS; // default policy for scheduler

    /* Initialize count, three buffer pointers */
    count = 0;
    buf_head = 0;
    buf_tail = 0;
    finished_head = 0;

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
```
After the include statement we flesh out main.
Within main we print the welcome message. Follow this we declare and instantiate a multitude of variables used throughout `commandline.c` and `modules.c`.
Following this we create two threads, one that calls `commandline` located in `commandline.c` and another that calls `dispatcher` which is located in `modules.c`.

Following this we wait for the threads to join, and if they have return values we print them out.

## II. Commandline

Within `commandline.c` we include `modules.h` and `commandline.h`.

```c++
#include "commandline.h"
#include "modules.h"
```

Following this we declare an array of strings that define the different values help should print out.

```c++
static const char *helpmenu[] = {
    "run <job> <time> <priority>: submit a job named <job>, execution time is <time>, priority is <pr>",
    "list: display the job status",
    "help: Print help menu",
    "fcfs: change the scheduling policy to FCFS",
    "sjf: changes the scheduling policy to SJF",
    "priority: changes the scheduling policy to priority",
    "test: <benchmark> <policy> <num_of_jobs> <priority_levels> <min_CPU_time> <max_CPU_time>",
    "quit: Exit AUbatch | -i quits after current job finishes | -d quits after all jobs finish",
    NULL};
```

Next we define a custom type `cmd` which is a struct that houses a string and a function. After that we define an array of `cmd`s. 
This will be used by `cmd_dispatch` to help decide which function to call based on the input value.

```c++
typedef struct
{
    const char *name;
    int (*func)(int nargs, char **args);
} cmd;

// array of cmds to be used by the command line
static const cmd cmdtable[] = {
    {"?", cmd_helpmenu},
    {"h", cmd_helpmenu},
    {"help", cmd_helpmenu},
    {"r", cmd_run},
    {"run", cmd_run},
    {"q", cmd_quit},
    {"quit", cmd_quit},
    {"fcfs", cmd_fcfs},
    {"sjf", cmd_sjf},
    {"priority", cmd_priority},
    {"list", cmd_list},
    {"ls", cmd_list},
    {"test", cmd_test},
    {NULL, NULL}};
```

After this we hit `commandline` which is called by the executor thread back in `aubatch.c`.
This is where command line gets input from the user to then determine what to do based on said input.

```c++
void *commandline(void *ptr)
{

    char *buffer;

    buffer = (char *)malloc(MAX_CMD_LEN * sizeof(char));
    if (buffer == NULL)
    {
        perror("Unable to malloc buffer");
        exit(1);
    }

    while (1)
    {
        printf("> [? for menu]: ");
        fgets(buffer, MAX_CMD_LEN, stdin);
        remove_newline(buffer);
        cmd_dispatch(buffer);
    }
    return (void *)NULL;
}
```
Next we will look into `cmd_dispatch` this is the main brains of `commandline` as it helps determine code flow.
Within this fun

```c++
int cmd_dispatch(char *cmd)
{
    char *args[MAXMENUARGS];
    int nargs = 0;
    char *word;
    char *context;
    int i, result;

    for (word = strtok_r(cmd, " ", &context);
         word != NULL;
         word = strtok_r(NULL, " ", &context))
    {

        if (nargs >= MAXMENUARGS)
        {
            printf("Command line has too many words\n");
            return E2BIG;
        }
        args[nargs++] = word;
    }

    if (nargs == 0)
    {
        return 0;
    }

    for (i = 0; cmdtable[i].name; i++)
    {
        if (*cmdtable[i].name && !strcmp(args[0], cmdtable[i].name))
        {
            assert(cmdtable[i].func != NULL);

            result = cmdtable[i].func(nargs, args);
            return result;
        }
    }

    printf("%s: Command not found\n", args[0]);
    return EINVAL;
}
```

# Recommendations

# Conclusion

# Source Code

You will find below the raw source code of the framework.

\newpage


# References

1: <https://en.wikipedia.org/wiki/Central_processing_unit>

2: <https://en.wikipedia.org/wiki/Scheduling_(computing)#First_come,_first_served>

3: <https://en.wikipedia.org/wiki/Shortest_job_next>

4: <https://en.wikipedia.org/wiki/Scheduling_(computing)#Fixed_priority_pre-emptive_scheduling>

[1]: <https://en.wikipedia.org/wiki/Central_processing_unit>
[2]: <https://en.wikipedia.org/wiki/Scheduling_(computing)#First_come,_first_served>
[3]: <https://en.wikipedia.org/wiki/Shortest_job_next>
[4]: <https://en.wikipedia.org/wiki/Scheduling_(computing)#Fixed_priority_pre-emptive_scheduling>