//
// Created by wicia on 23.03.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>


int main (int argc, char**argv)
{
    srand(time(NULL));

    if (argc!=5)
    {
        printf("Wrong number of arguments\n");
        exit(1);
    }

    FILE *f;

    long pmin = strtol(argv[2],NULL, 10);
    long pmax = strtol(argv[3],NULL,10);

    if (pmin <= 0 || pmax <=0 || pmin>pmax)
    {
        printf("Erro in pmin or/and pmax values\n");
        exit(1);
    }

    long bytes = strtol(argv[4],NULL, 10);

    if (bytes <= 0 )
    {
        printf("Error in bytes\n");
        exit(1);
    }

    time_t czas;
    int wait_time;
    char buff[25];


    while (1)
    {
        f = fopen(argv[1],"a");
        if (!f)
        {
            printf("Error \n");
            exit(1);
        }
        else {
            wait_time = (rand() % (pmax - pmin + 1)) + pmin;
            sleep(wait_time);
            time(&czas);
            strftime(buff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&czas));
            fprintf(f, "\nPID: %d seconds: %d %s", getpid(), wait_time, buff);
            for (int i = 0; i < bytes; i++) {
                fprintf(f, "%c", '*');
            }
            fclose(f);
        }
    }

    return 0;
}