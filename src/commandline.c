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

#include "commandline.h"

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
void *commandline(void *ptr);
int cmd_priority();
int cmd_fcfs();
int cmd_sjf();
int cmd_list();

static const char *helpmenu[] = {
    "run <job> <time> <priority>: submit a job named <job>, execution time is <time>, priority is <pr>",
    "list: display the job status"
    "help: Print help menu",
    "fcfs: change the scheduling policy to FCFS",
    "sjf: changes the scheduling policy to SJF",
    "priority: changes the scheduling policy to priority",
    "quit: Exit AUbatch",
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
    {"fcfs\n", cmd_fcfs},
    {"sjf\n", cmd_sjf},
    {"priority\n", cmd_priority},
    {"list\n", cmd_list},
    {"ls\n", cmd_list},
    /* Please add more operations below. */
    {NULL, NULL}};

/*
 * Command line main loop.
 */
void *commandline(void *ptr)
{
    printf("%s \n", (char *)ptr);

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
    return (void *)NULL;
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

    scheduler(nargs, args);

    /* Use execv to run the submitted job in AUbatch */
    // printf("use execv to run the job in AUbatch.\n");
    return 0; /* if succeed */
}

/*
 * The quit command.
 */
int cmd_quit(int nargs, char **args)
{
    report_metrics();
    // printf("Please display performance information before exiting AUbatch!\n");
    exit(0);
}

/*
 * Display menu information
 */
void showmenu(const char *name, const char *x[])
{

    printf("\n");
    printf("%s\n", name);

    int i = 0;
    while (1)
    {
        if (x[i] == NULL)
        {
            break;
        }
        printf("%s\n", x[i]);
        i++;
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
    // time_t beforesecs, aftersecs, secs;
    // u_int32_t beforensecs, afternsecs, nsecs;
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

int cmd_priority()
{
    policy = PRIORITY;
    return 0;
}
int cmd_sjf()
{
    policy = SJF;
    return 0;
}
int cmd_fcfs()
{
    policy = FCFS;
    return 0;
}

int cmd_list()
{
    printf("Name CPU_Time Pri Arrival_time             Progress\n");
    for (int i = 0; i < finished_head; i++)
    {

        finished_process_p process = finished_process_buffer[i];
        char *status = "finished";

        char *time = convert_time(process->arrival_time);
        remove_newline(time);
        printf("%4s %8d %3d %s %s\n",
               process->cmd,
               process->cpu_burst,
               process->priority,
               time,
               status);
    }

    for (int i = 0; i < buf_head; i++)
    {

        process_p process = process_buffer[i];
        char *status = "-------";
        if (process->cpu_remaining_burst == 0)
        {
            continue;
        }
        else if (process->first_time_on_cpu > 0 && process->cpu_remaining_burst > 0)
        {
            status = "running ";
        }

        char *time = convert_time(process->arrival_time);
        remove_newline(time);
        printf("%4s %8d %3d %s %s\n",
               process->cmd,
               process->cpu_burst,
               process->priority,
               time,
               status);
    }
    printf("\n");
    return 0;
}