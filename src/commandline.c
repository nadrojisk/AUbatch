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
int cmd_test(int nargs, char **args);
void change_scheduler();

static const char *helpmenu[] = {
    "run <job> <time> <priority>: submit a job named <job>, execution time is <time>, priority is <pr>",
    "list: display the job status",
    "help: Print help menu",
    "fcfs: change the scheduling policy to FCFS",
    "sjf: changes the scheduling policy to SJF",
    "priority: changes the scheduling policy to priority",
    "test: <benchmark> <policy> <num_of_jobs> <priority_levels> <min_CPU_time> <max_CPU_time>",
    "quit: Exit AUbatch | -i quits after current job finishes | -d quits after all jobs finish",
    /* Please add more menu options below */
    NULL};

typedef struct
{
    const char *name;
    int (*func)(int nargs, char **args);
} cmd;

static const cmd cmdtable[] = {
    /* commands: single command must end with \n */
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
        remove_newline(buffer);
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
    if (!strcmp(args[1], "-i")) // wait for current job to finish running
    {

        int cur_count = count;
        printf("Waiting for current job to finish ... \n");
        if (count)
        {
            while (cur_count == count)
            {
            }
        }
    }
    else if (!strcmp(args[1], "-d")) // wait for all jobs to finish
    {
        printf("Waiting for all jobs to finish...\n");
        while (count)
        {
        }
    }
    printf("Quiting AUBatch... \n");

    report_metrics();

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
    change_scheduler();
    return 0;
}
int cmd_sjf()
{
    policy = SJF;
    change_scheduler();
    return 0;
}
int cmd_fcfs()
{
    policy = FCFS;
    change_scheduler();
    return 0;
}

void change_scheduler()
{
    const char *str_policy = get_policy_string();
    printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n", str_policy, buf_head - buf_tail);
}
int cmd_list()
{
    if (finished_head || count)
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
    }
    else
        printf("No processes loaded yet!\n");
    return 0;
}

int cmd_test(int nargs, char **argv)
{

    srand(0);
    if (nargs != 8)
    {
        printf("Usage: test <benchmark> <policy> <num_of_jobs> <arrival_rate> <priority_levels> <min_CPU_time> <max_CPU_time>\n");
        return EINVAL;
    }
    else if (count || finished_head)
    {
        printf("Error: Jobs current in queue, no jobs should have ran if doing benchmark...\n");
        return EINVAL;
    }
    char *benchmark = argv[1];
    char *str_policy = argv[2];
    int num_of_jobs = atoi(argv[3]);
    int arrival_rate = atoi(argv[4]);
    int priority_levels = atoi(argv[5]);
    int min_cpu_burst = atoi(argv[6]);
    int max_cpu_burst = atoi(argv[7]);

    if (min_cpu_burst >= max_cpu_burst)
    {
        printf("Error: <min_CPU_time> cannot be greater than or equal to <max_CPU_time>\n");
        return EINVAL;
    }
    else if (num_of_jobs < 0 || min_cpu_burst < 0 || max_cpu_burst < 0 || priority_levels < 0 || arrival_rate < 0)
    {
        printf("Error: <num_of_jobs> <min_CPU_time> <max_CPU_time> <arrival_rate> and <priority_levels> must be greater than 0\n");
        return EINVAL;
    }

    if (!strcmp(str_policy, "fcfs"))
    {
        policy = FCFS;
    }
    else if (!strcmp(str_policy, "sjf"))
    {
        policy = SJF;
    }
    else if (!strcmp(str_policy, "priority"))
    {
        policy = PRIORITY;
    }
    else
    {
        printf("Error: <policy> must be either fcfs, sjf, or priority\n");
        return EINVAL;
    }

    test_scheduler(benchmark, num_of_jobs, arrival_rate, priority_levels, min_cpu_burst, max_cpu_burst);
    printf("Benchmark is running please wait...\n");
    while (count)
    {
    }

    report_metrics();

    for (int i = 0; i < finished_head; i++)
    {
        free(finished_process_buffer[i]);
        free(process_buffer[i]);
    }
    finished_head = 0;
    buf_head = 0;
    buf_tail = 0;

    return 0;
}