/* 
 * COMP7500/7506
 * Project 3: commandline parser
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * Provides implementation for the command line interface
 * which the user will interact with to submit jobs / processes
 * to scheduler and dispatcher
 *
 * Compilation Instruction: 
 * gcc -o aubatch.out aubatch.c commandline.c modules.c -lpthread -Wall
 *
 */

#include "commandline.h"
#include "modules.h"

#include <sys/stat.h>

// char array of help definitions
static const char *helpmenu[] = {
    "run <job> <time> <priority>: submit a job named <job>, execution time is <time>, priority is <pr>",
    "list: display the job status",
    "help: print help menu",
    "fcfs: change the scheduling policy to FCFS",
    "sjf: changes the scheduling policy to SJF",
    "priority: changes the scheduling policy to priority",
    "test <benchmark> <policy> <num_of_jobs> <arrival_time> <priority_levels> <min_CPU_time> <max_CPU_time>",
    "quit: exit AUbatch | -i quits after current job finishes | -d quits after all jobs finish",
    NULL};

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

/*
 * Command line main loop.
 */
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
/*
 * Process a single command.
 */
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
    // ensure file exists first
    FILE *f = fopen(args[1], "r");
    if (f == NULL)
    {
        printf("Error file does not exist. Please use relative or full path\n");
        fclose(f);
        return EINVAL;
    }
    fclose(f);
    scheduler(nargs, args);
    return 0; /* if succeed */
}

/*
 * The quit command.
 */
int cmd_quit(int nargs, char **args)
{
    if (nargs == 2)
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
    }
    printf("Quiting AUBatch... \n");

    report_metrics();

    exit(0);
}

/*
 * Display menu information
 */
int cmd_helpmenu(int n, char **a)
{

    printf("\n");
    printf("AUbatch help menu\n");

    int i = 0;
    while (1)
    {
        if (helpmenu[i] == NULL)
        {
            break;
        }
        printf("%s\n", helpmenu[i]);
        i++;
    }
    printf("\n");
    return 0;
}

/*
 * change scheduler to priority based
 */
int cmd_priority()
{
    policy = PRIORITY;
    change_scheduler();
    return 0;
}

/*
 * change scheduler to shorted job first
 */
int cmd_sjf()
{
    policy = SJF;
    change_scheduler();
    return 0;
}

/*
 * change scheduler to first come first served
 */
int cmd_fcfs()
{
    policy = FCFS;
    change_scheduler();
    return 0;
}

/*
 * print out notification that scheduler is being changed
 */
void change_scheduler()
{
    const char *str_policy = get_policy_string();
    printf("Scheduling policy is switched to %s. All the %d waiting jobs have been rescheduled.\n", str_policy, buf_head - buf_tail);
    if (count)
        sort_buffer(process_buffer);
}

/*
 * list running, finished, and waiting processes
 */
int cmd_list()
{
    if (finished_head || count)
    {
        printf("Name               CPU_Time Pri Arrival_time             Progress\n");
        int i;
        for (i = 0; i < finished_head; i++)
        {

            finished_process_p process = finished_process_buffer[i];
            char *status = "finished";

            char *time = convert_time(process->arrival_time);
            remove_newline(time);
            printf("%-18s %-8d %-3d %s %s\n",
                   process->cmd,
                   process->cpu_burst,
                   process->priority,
                   time,
                   status);
        }

        for (i = 0; i < buf_head; i++)
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
            printf("%-18s %-8d %-3d %s %s\n",
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

/* 
 * run benchmark test
 * 
 * instead of inputting each job one by one with `run` users can use test to 
 * input a large number of jobs at once
 * 
 * this can be used to compare different scheduling algorithms
 */
int cmd_test(int nargs, char **argv)
{

    srand(0); // ensure seed is set to the same value each time to make same jobs created
    if (nargs != 8)
    {
        printf("Usage: test <benchmark> <policy> <num_of_jobs> <arrival_rate> <priority_levels> <min_CPU_time> <max_CPU_time>\n");
        return EINVAL;
    }
    else if (count || finished_head)
    {
        printf("Error: Jobs current in queue / on CPU, no jobs should have ran if doing benchmark...\n");
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
    else if (num_of_jobs <= 0 || min_cpu_burst < 0 || max_cpu_burst < 0 || priority_levels < 0 || arrival_rate < 0)
    {
        printf("Error: <num_of_jobs> cannot be equal or less than zero\nError: <min_CPU_time> <max_CPU_time> <arrival_rate> and <priority_levels> must be greater than 0\n");
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

    // clear process queue and finished queue
    // ensures that the metrics aren't reported when quitting aubatch
    // also ensures if running metrics again that the prior jobs will not interfere
    int i;
    for (i = 0; i < finished_head; i++)
    {
        free(finished_process_buffer[i]);
    }
    finished_head = 0;
    buf_head = 0;
    buf_tail = 0;

    return 0;
}