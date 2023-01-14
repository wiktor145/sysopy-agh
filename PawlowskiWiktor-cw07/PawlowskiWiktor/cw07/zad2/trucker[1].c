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

sem_t * semaphore_0=SEM_FAILED;
sem_t * semaphore_1=SEM_FAILED;
sem_t * semaphore_2=SEM_FAILED;
sem_t * semaphore_3=SEM_FAILED;

int shared_memory_id = 0;
int X,K,M;
int truck;
struct timeval ende;
int for_end = 0;

void end(int _)
{
    printf("\nCTRL-C ...\n\n");
    sem_post(semaphore_3);
    for_end = 1;
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



void truck_arrives(void)
{
    printf("Truck arrives at: ");
    time_get();

}

void line_statistics(void)
{
    printf("Line:\n");
    printf("Pieces: %d free out of %d\n",line->pcs_left,line->max_pcs);
    printf("Load: %d out of %d\n\n",line->weight_left,line->max_weight);
}

void new_truck(void)
{
    printf("Truck is full! Leaving...\n\n");
    time_get();
    truck = X;

}


void closing(void)
{
    sem_post(semaphore_3);
    int val;
    usleep(100);

    sem_getvalue(semaphore_1,&val);

    while (val > 0)
    {
        usleep(500);
        sem_getvalue(semaphore_1,&val);
    }


    sem_getvalue(semaphore_2,&val);
    while(val > 0)
    {
        if (truck == 0)
            truck_arrives();

        //informations about package
        printf("Loading package:\n");
        time_get();
        printf("Worker PID: %d\n",line->packages[line->take_index].pid);

        //loading time
        gettimeofday(&ende, NULL);
        printf("Time: %ld:%ld\n",
               -1*(line->packages[line->take_index].try_time.tv_sec-ende.tv_sec),
               -1*(line->packages[line->take_index].try_time.tv_usec-ende.tv_usec));
        printf("Package mass: %d\n\n",line->packages[line->take_index].mass);

        //decreasen free places on truck
        --truck;
        //info truck
        printf("Truck: %d free out of %d\n\n",truck,X);

        // line - decrease line load and pcs
        line->weight_left+=line->packages[line->take_index].mass;
        ++line->pcs_left;
        sem_wait(semaphore_2);

        line_statistics();

        line->take_index=(line->take_index+1)%line->max_pcs;

        if (truck == 0)
            new_truck();

        sem_getvalue(semaphore_2,&val);
    }

    printf("Truck leaves...\n\n");

    sem_close(semaphore_0);
    if (semaphore_0 != SEM_FAILED) sem_unlink(PATH_SEM_0);
    sem_close(semaphore_1);
    if (semaphore_1 != SEM_FAILED) sem_unlink(PATH_SEM_1);
    sem_close(semaphore_2);
    if (semaphore_2 != SEM_FAILED) sem_unlink(PATH_SEM_2);
    sem_close(semaphore_3);
    if (semaphore_3 != SEM_FAILED) sem_unlink(PATH_SEM_3);

    munmap(line,sizeof(line));

    if (shared_memory_id != 0)
    {
        shm_unlink(PATH_MEM);
    }

}



int main(int argc, char** argv)
{
    atexit(closing);

    struct sigaction act;
    act.sa_handler = end;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    if (argc != 4)
    {
        printf("Error, program should be executed\n with 3 arguments\n");
        printf("X - load of truck, K - max packages at line\n");
        printf("M - max load of line\n");
        exit(1);
    }



    X = atoi(argv[1]);
    if (X == 0 || X < N)
    {
        printf("Error in X value - %s\n",argv[1]);
        printf("X must be greater than max weight of package %d\n",N);
        exit(1);
    }

    K = atoi(argv[2]);
    if (K == 0 || K > MAX_LINE_LENGTH)
    {
        printf("Error in K value - %s\n",argv[2]);
        printf("K must be less or equal to: %d\n",MAX_LINE_LENGTH);
        exit(1);
    }

    M = atoi(argv[3]);
    if (M == 0  || M < N)
    {
        printf("Error in M value - %s\n",argv[3]);
        printf("M must be greater or equal to the max weight of package %d\n",N);
        exit(1);
    }

    semaphore_0 = sem_open(PATH_SEM_0, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG,1);
    semaphore_1 = sem_open(PATH_SEM_1, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG,1);
    semaphore_2 = sem_open(PATH_SEM_2, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG,0);
    semaphore_3 = sem_open(PATH_SEM_3, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG,0);

    if (semaphore_0 ==SEM_FAILED|| semaphore_1 ==SEM_FAILED || semaphore_2 ==SEM_FAILED || semaphore_3 ==SEM_FAILED )
    {
        printf("Error during creating semaphores\n");
        exit(1);
    }

    shared_memory_id =  shm_open(PATH_MEM, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG );
    if (shared_memory_id == -1)
    {
        printf("Error during creating shared memory...\n\n");
        exit(1);
    }

    int e = ftruncate(shared_memory_id,sizeof(*line));
    if (e == -1)
    {
        printf("Error during truncating memory\n\n");
        exit(2);
    }

    line =  mmap(
            NULL,                   // address
            sizeof(*line),    // length
            PROT_READ | PROT_WRITE, // prot (memory segment security)
            MAP_SHARED,             // flags
            shared_memory_id,       // file descriptor
            0                       // offset
    );

    if (line == (void*) -1)
    {
        printf("Error during accessing shared memory\n");
        exit(2);
    }

    line->max_weight = M;
    line->max_pcs = K;

    line->weight_left = M;
    line->pcs_left = K;

    line->load_index = 0;
    line->take_index = 0;

    truck = X;
    truck_arrives();

    int val;
    int val1;


    while (1)
    {

        printf("Waiting for package...\n");
        time_get();
        //waiting for non empty line
        sem_getvalue(semaphore_2,&val);
        while (val == 0)
        {
            sem_getvalue(semaphore_1,&val);

            if (val == 0) {
                printf("All workers ended work\n");
                printf("Truck leaves...\n");
                time_get();
                exit(0);
            }
            usleep(40);
            sem_getvalue(semaphore_2,&val);
        }

        //waiting for turn
        sem_wait(semaphore_0);

        //informations about package
        printf("Loading package:\n");
        time_get();
        printf("Worker PID: %d\n",line->packages[line->take_index].pid);

        //loading time
        gettimeofday(&ende, NULL);
        printf("Time: %ld:%ld\n",
               -1*(line->packages[line->take_index].try_time.tv_sec-ende.tv_sec),
               -1*(line->packages[line->take_index].try_time.tv_usec-ende.tv_usec));
        printf("Package mass: %d\n\n",line->packages[line->take_index].mass);

        //decreasen free places on truck
        --truck;
        //info truck
        printf("Truck: %d free out of %d\n\n",truck,X);

        // line - decrease line load and pcs
        line->weight_left+=line->packages[line->take_index].mass;
        ++line->pcs_left;
        sem_wait(semaphore_2);

        line_statistics();

        line->take_index=(line->take_index+1)%line->max_pcs;

        if (truck == 0)
        {
            new_truck();

            val = sem_getvalue(semaphore_2,&val);
            val1 = sem_getvalue(semaphore_1,&val);
            if (val == 0 && val1 == 0)
            {
                printf("All workers ended work\n");
                printf("Truck leaves...\n");
                time_get();
                exit(0);
            }
            else truck_arrives();
        }
        val = sem_getvalue(semaphore_2,&val);
        val1 = sem_getvalue(semaphore_1,&val);

        if (val == 0 && val1 == 0) {
            printf("All workers ended work\n");
            printf("Truck leaves...\n");
            time_get();
            exit(0);
        }


        sem_post(semaphore_0);
        if (for_end) exit(0);

    }

    return(0);

}




