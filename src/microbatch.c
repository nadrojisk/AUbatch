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
