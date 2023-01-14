#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>


int main(int argc, char** argv)
{
    if (argc!= 2)
    {
        printf("Wrong number of arguments, you should only give\n");
        printf("name of FIFO\n");
        exit(1);
    }
    //                 permissions
    if (mkfifo(argv[1],0666) == -1)
    {
        printf("Error in creating FIFO %s\n",argv[1]);
        exit(2);
    }

    FILE* fifo = fopen(argv[1], "r");
    if (!fifo)
    {
        printf("Opening fifo failed\n");
        exit(3);
    }
    else
    {
        printf("Opened FIFO %s\n",argv[1]);
    }

    char buff[PIPE_BUF];

    while(fgets(buff,PIPE_BUF,fifo))
    {
        printf("%s\n",buff);
    }

    fclose(fifo);

    return 0;

}