/* 
 * COMP7500/7506
 * Project 3: microbatch 
 * 
 * Author: Jordan Sosnowski
 * Reference: Dr. Xiao Qin
 * 
 * Date: March 9, 2020. Version 1.0
 *
 * Example program to be called by aubatch, simply sleeps for argv[2] time
 * This assumes argv[2] is a number
 * 
 * If you were to call ./microbatch 10 it would simply sleep for 10 seconds
 * 
 * Compilation Instruction: 
 * gcc -o microbatch.out microbatch.c
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void remove_newline(char *buffer)
{
    int string_length = strlen(buffer);
    if (buffer[string_length - 1] == '\n')
    {
        buffer[string_length - 1] = '\0';
    }
}

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        return 1;
    }

    remove_newline(argv[1]);

    sleep(atoi(argv[1]));

    return 0;
}
