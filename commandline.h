#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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