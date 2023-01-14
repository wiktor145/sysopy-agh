//
// Created by wicia on 10.05.19.
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
int shared_memory_id = -1;

void end_int(int _)
{
    exit(0);
}

void end(void)
{
    printf("Loader with pid: %d  ending work...\n\n",getpid());
    struct sembuf take;
    take.sem_num = 1;
    take.sem_op = -1;
    take.sem_flg = 0;

   // printf("1, %d\n",getpid());
    semop(semaphores,&take,1);
    //printf("2, %d\n",getpid());

    shmdt(line);
   // printf("3, %d\n",getpid());

}

void time_get(void)
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    long milli = curTime.tv_usec;

    char buffer [80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

    char currentTime[84] = "";
    sprintf(currentTime, "%s:%ld", buffer, milli);
    printf("%s\n\n", currentTime);
}

void line_statistics(void)
{
    printf("Line:\n");
    printf("Pieces: %d free out of %d\n",line->pcs_left,line->max_pcs);
    printf("Load: %d out of %d\n\n",line->weight_left,line->max_weight);
}


int main(int argc, char** argv)
{


    key_t key = ftok(PATH_SEM,CHAR);

    semaphores = semget(key,0, 0);

    key = ftok(PATH_MEM,CHAR);

    shared_memory_id = shmget(key,0,0);

    line = shmat(shared_memory_id,NULL,0);

    if (semaphores == -1 || shared_memory_id == -1 || line == (void*) -1)
    {
        printf("Error!\n\n");
        exit(1);
    }

    atexit(end);

    struct sigaction act;
    act.sa_handler = end_int;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    int cycles  = atoi(argv[1]);

    struct sembuf take_0;
    take_0.sem_num = 0;
    take_0.sem_op = -1;
    take_0.sem_flg = 0;

    struct sembuf give_0;
    give_0.sem_num = 0;
    give_0.sem_op = 1;
    give_0.sem_flg = 0;

    struct sembuf give_2;
    give_2.sem_num = 2;
    give_2.sem_op = 1;
    give_2.sem_flg = 0;

    srand(time(NULL) + getpid());
    int mass = rand()%N + 1;
    int pid = getpid();

    

    struct timeval start;

    if (cycles)
    {
        for (int i=0; i<cycles; i++)
        {
            if (semctl(semaphores,3,GETVAL) == 1) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                exit(0);
            }

            printf("Waiting for line... PID: %d\n\n",pid);
            time_get();
            gettimeofday(&start, NULL);
            semop(semaphores,&take_0,1);

            if (errno == EIDRM) exit(0);
            if (semctl(semaphores,3,GETVAL) == 1) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                semop(semaphores,&give_0,1);
                exit(0);
            }
            if (line->pcs_left == 0 || line->weight_left < mass)
            {
                semop(semaphores,&give_0,1);
                if (errno == EIDRM) exit(0);
                usleep(100);
            }
            else
            {
                printf("Loading package on line with mass %d  PID: %d\n\n",mass,pid);
                time_get();

                --line->pcs_left;
                line->weight_left-=mass;
                line->packages[line->load_index].mass=mass;
                line->packages[line->load_index].pid=pid;
                line->packages[line->load_index].try_time = start;

                line_statistics();

                line->load_index = (line->load_index + 1)%line->max_pcs;

                semop(semaphores,&give_2,1);
                if (errno == EIDRM) exit(0);
                semop(semaphores,&give_0,1);
                if (errno == EIDRM) exit(0);
                usleep(300);
            }

        }
        printf("Loader with pid: %d  ending work...\n\n",pid);

    }
    else{

        while (1)
        {
            if (semctl(semaphores,3,GETVAL) == 1) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                exit(0);
            }

            printf("Waiting for line... PID: %d\n\n",pid);
            time_get();
            gettimeofday(&start, NULL);
            semop(semaphores,&take_0,1);

            if (errno == EIDRM) exit(0);
            if (semctl(semaphores,3,GETVAL) == 1) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                semop(semaphores,&give_0,1);
                exit(0);
            }
            if (line->pcs_left == 0 || line->weight_left < mass)
            {
                semop(semaphores,&give_0,1);
                if (errno == EIDRM) exit(0);
                usleep(100);
            }
            else
            {
                printf("Loading package on line with mass %d  PID: %d\n\n",mass,pid);
                time_get();

                --line->pcs_left;
                line->weight_left-=mass;
                line->packages[line->load_index].mass=mass;
                line->packages[line->load_index].pid=pid;
                line->packages[line->load_index].try_time = start;

                line_statistics();

                line->load_index = (line->load_index + 1)%line->max_pcs;

                semop(semaphores,&give_2,1);
                if (errno == EIDRM) exit(0);
                semop(semaphores,&give_0,1);
                if (errno == EIDRM) exit(0);
                usleep(300);
            }

        }
    }

    return 0;
}