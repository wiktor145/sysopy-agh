//
// Created by wicia on 12.05.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include "common.h"

int semaphores = -1;

void end(void)
{
    printf("Loadersmaker end...\n\n");
}


int main(int argc, char** argv) {

    atexit(end);
    if (argc != 2 && argc != 3)
    {
        printf("Error. Should give 1 or 2 arguments\n");
        printf("First - number of loaders\n");
        printf("Second (optional) - cycles of loaders life\n");
        exit(1);
    }

    int loaders = atoi(argv[1]);

    if (loaders <=0)
    {
        printf("Error in loaders number: %s\n",argv[1]);
        exit(2);
    }

    int cycles;

    if (argc == 3)
    {
        cycles = atoi(argv[2]);
        if (cycles <=0)
        {
            printf("Error in cycles value: %s\n",argv[2]);
            exit(2);
        }

    }
    else
    {
        cycles = 0;
    }

    int pid;
    key_t key = ftok(PATH_SEM,CHAR);

    semaphores = semget(key,0, 0);
    if (semaphores == -1)
    {
        printf("Error!\n");
        exit(1);
    }

    semctl(semaphores,1,SETVAL,loaders);


    for (int i=0; i<loaders; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            if (cycles)
            {
                execl("./loader","./loader",argv[2],NULL);
            }
            else
            {
                execl("./loader","./loader","0\n",NULL);

            }
            printf("Error during execution!\n");
            exit(1);
        }
    }
   // printf("loadersmaker returns\n");
    return 0;
}