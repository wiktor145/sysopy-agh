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
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>

sem_t * semaphore_0 = SEM_FAILED;
sem_t * semaphore_1 = SEM_FAILED;
sem_t * semaphore_2 = SEM_FAILED;
sem_t * semaphore_3 = SEM_FAILED;

int shared_memory_id = -1;

void end_int(int _)
{
    exit(0);
}

void end(void)
{
    printf("Loader with pid: %d  ending work...\n\n",getpid());

   // printf("1, %d\n",getpid());
    sem_wait(semaphore_1);
    //printf("2, %d\n",getpid());
    sem_close(semaphore_0);
    sem_close(semaphore_1);
    sem_close(semaphore_2);
    sem_close(semaphore_3);


    munmap(line,sizeof(struct line));
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
    semaphore_0 = sem_open(PATH_SEM_0, O_WRONLY);
    semaphore_1 = sem_open(PATH_SEM_1, O_WRONLY);
    semaphore_2 = sem_open(PATH_SEM_2, O_WRONLY);
    semaphore_3 = sem_open(PATH_SEM_3, O_WRONLY);

    shared_memory_id = shm_open(PATH_MEM, O_RDWR, S_IRWXU | S_IRWXG);

    line = mmap(NULL,
                sizeof(*line),
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                shared_memory_id,
                0);

    if (semaphore_0 ==SEM_FAILED || semaphore_1 ==SEM_FAILED || semaphore_2 ==SEM_FAILED || semaphore_3 ==SEM_FAILED || shared_memory_id == -1 || line == (void*) -1)
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

    srand(time(NULL) + getpid());
    int mass = rand()%N + 1;
    int pid = getpid();

    struct timeval start;

    if (cycles)
    {
        for (int i=0; i<cycles; i++)
        {
            int val;
            sem_getvalue(semaphore_3,&val);
            if (val > 0) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                exit(0);
            }

            printf("Waiting for line... PID: %d\n\n",pid);
            time_get();
            gettimeofday(&start, NULL);
            sem_wait(semaphore_0);

            //if (errno == EIDRM) exit(0);

            sem_getvalue(semaphore_3,&val);
            if (val > 0) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                sem_post(semaphore_0);
                exit(0);
            }
            if (line->pcs_left == 0 || line->weight_left < mass)
            {
                sem_post(semaphore_0);
                //if (errno == EIDRM) exit(0);
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

                sem_post(semaphore_2);
                //if (errno == EIDRM) exit(0);
                sem_post(semaphore_0);
                //if (errno == EIDRM) exit(0);
                usleep(300);
            }

        }
        printf("Loader with pid: %d  ending work...\n\n",pid);

    }
    else{

        while (1)
        {
            int val;
            sem_getvalue(semaphore_3,&val);
            if (val > 0) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                exit(0);
            }

            printf("Waiting for line... PID: %d\n\n",pid);
            time_get();
            gettimeofday(&start, NULL);
            sem_wait(semaphore_0);

            //if (errno == EIDRM) exit(0);

            sem_getvalue(semaphore_3,&val);
            if (val > 0) {

                printf("Trucker did CTRL-Z, exiting...\n\n");
                sem_post(semaphore_0);
                exit(0);
            }
            if (line->pcs_left == 0 || line->weight_left < mass)
            {
                sem_post(semaphore_0);
                //if (errno == EIDRM) exit(0);
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

                sem_post(semaphore_2);
                //if (errno == EIDRM) exit(0);
                sem_post(semaphore_0);
                //if (errno == EIDRM) exit(0);
                usleep(300);
            }

        }
    }

    return 0;
}