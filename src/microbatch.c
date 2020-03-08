#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 1;
    }
    sleep(atoi(argv[2]));
    return 0;
}