/* 
 * COMP7500/7506
 * Project 3: commandline parser
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * This sample source code demonstrates how to:
 * (1) separate policies from a mechanism
 * (2) parse a commandline using getline() and strtok_r()
 *
 * Compilation Instruction: 
 * gcc commandline_parser.c -o commandline_parser
 * ./commandline_parser
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "aubatch.h"

/* Error Code */
#define EINVAL 1
#define E2BIG 2

#define MAXMENUARGS 4
#define MAXCMDLINE 64

void menu_execute(char *line, int isargs);
int cmd_run(int nargs, char **args);
int cmd_quit(int nargs, char **args);
void showmenu(const char *name, const char *x[]);
int cmd_helpmenu(int n, char **a);
int cmd_dispatch(char *cmd);
int commandline();

static const char *helpmenu[] = {
    "[run] <job> <time> <priority>       ",
    "[quit] Exit AUbatch                 ",
    "[help] Print help menu              ",
    /* Please add more menu options below */
    NULL};

typedef struct
{
    const char *name;
    int (*func)(int nargs, char **args);
} cmd;

static const cmd cmdtable[] = {
    /* commands: single command must end with \n */
    {"?\n", cmd_helpmenu},
    {"h\n", cmd_helpmenu},
    {"help\n", cmd_helpmenu},
    {"r", cmd_run},
    {"run", cmd_run},
    {"q\n", cmd_quit},
    {"quit\n", cmd_quit},
    /* Please add more operations below. */
    {NULL, NULL}};

/*
 * Command line main loop.
 */
int main()
{
    char *buffer;
    size_t bufsize = 64;

    buffer = (char *)malloc(bufsize * sizeof(char));
    if (buffer == NULL)
    {
        perror("Unable to malloc buffer");
        exit(1);
    }

    while (1)
    {
        printf("> [? for menu]: ");
        getline(&buffer, &bufsize, stdin);
        cmd_dispatch(buffer);
    }
    return 0;
}

/*
 * The run command - submit a job.
 */
int cmd_run(int nargs, char **args)
{
    if (nargs != 4)
    {
        printf("Usage: run <job> <time> <priority>\n");
        return EINVAL;
    }

    // setup IPC information
    pid_t pid;
    int fd1[2];
    int fd2[2];
    if (pipe(fd1) == -1)
    {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }
    if (pipe(fd2) == -1)
    {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }
    // fork running process
    pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork Failed");
        return 1;
    }

    if (pid == 0) //child
    {
        aubatch(nargs, args);
    }

    /* Use execv to run the submitted job in AUbatch */
    // printf("use execv to run the job in AUbatch.\n");
    return 0; /* if succeed */
}

int aubatch(int argc, char **argv)
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
 * The quit command.
 */
int cmd_quit(int nargs, char **args)
{
    printf("Please display performance information before exiting AUbatch!\n");
    exit(0);
}

/*
 * Display menu information
 */
void showmenu(const char *name, const char *x[])
{
    int ct, half, i;

    printf("\n");
    printf("%s\n", name);

    for (i = ct = 0; x[i]; i++)
    {
        ct++;
    }
    half = (ct + 1) / 2;

    for (i = 0; i < half; i++)
    {
        printf("    %-36s", x[i]);
        if (i + half < ct)
        {
            printf("%s", x[i + half]);
        }
        printf("\n");
    }

    printf("\n");
}

int cmd_helpmenu(int n, char **a)
{
    (void)n;
    (void)a;

    showmenu("AUbatch help menu", helpmenu);
    return 0;
}

/*
 * Process a single command.
 */
int cmd_dispatch(char *cmd)
{
    time_t beforesecs, aftersecs, secs;
    u_int32_t beforensecs, afternsecs, nsecs;
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

            /*Qin: Call function through the cmd_table */
            result = cmdtable[i].func(nargs, args);
            return result;
        }
    }

    printf("%s: Command not found\n", args[0]);
    return EINVAL;
}
