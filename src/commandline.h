/* 
 * COMP7500/7506
 * Project 3: commandline header
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * Header file for commandline, used by driver
 *
 * Compilation Instruction: 
 * gcc -o aubatch.out aubatch.c commandline.c modules.c -lpthread -Wall
 *
 */

#include <assert.h>
#include <sys/wait.h>

/* Error Code */
#define EINVAL 1
#define E2BIG 2

#define MAXMENUARGS 8
#define MAXCMDLINE 64

void menu_execute(char *line, int isargs);
int cmd_run(int nargs, char **args);
int cmd_quit(int nargs, char **args);
void showmenu();
int cmd_helpmenu(int n, char **a);
int cmd_dispatch(char *cmd);
void *commandline(void *ptr);
int cmd_priority();
int cmd_fcfs();
int cmd_sjf();
int cmd_list();
int cmd_test(int nargs, char **args);
void change_scheduler();