#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>


int main(int argc, char** argv)
{
    if (argc!= 3)
    {
        printf("Wrong number of arguments, you should give\n");
        printf("name of FIFO and then number n\n");
        exit(1);
    }

    long n = strtol(argv[2],NULL,10);

    if (n<=0)
    {
        printf("Error in number n\n");
        exit(2);
    }

    char date_buff[100];

    FILE* fifo = fopen(argv[1],"w");

    if (!fifo)
    {
        printf("Opening fifo failed\n");
        exit(3);
    }
    else
    {
        printf("Opened fifo\n");
    }

    int pid = getpid();
    int czas;

    printf("My PID: %d\n",pid);

    srand(time(NULL));
    char fifobuff[PIPE_BUF];

    for (int i=0; i<n; i++)
    {
        FILE * get_date = popen("date","r");

        fgets(date_buff,100,get_date);
        pclose(get_date);

        sprintf(fifobuff,"PID: %d  Date: %s",pid,date_buff);
        fputs(fifobuff,fifo);
        fflush(fifo);

        czas = (rand()%3) + 2;
        sleep(czas);
    }

    fclose(fifo);
    return 0;

}