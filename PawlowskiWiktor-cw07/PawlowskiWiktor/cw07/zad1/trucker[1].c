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
int X,K,M;
int truck;
struct timeval ende;
int for_end = 0;


void end(int _)
{
    printf("\nCTRL-C ...\n\n");
    for_end = 1;
    semctl(semaphores,3,SETVAL,1);
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
    semctl(semaphores,3,SETVAL,1);
    usleep(100);

    struct sembuf take_2;
    take_2.sem_num = 2;
    take_2.sem_op = -1;
    take_2.sem_flg = 0;

   while (semctl(semaphores,1,GETVAL) > 0)
   {
       usleep(100);
   }

    while(semctl(semaphores,2,GETVAL) > 0)
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
        semop(semaphores,&take_2,1);

        line_statistics();

        line->take_index=(line->take_index+1)%line->max_pcs;

        if (truck == 0)
            new_truck();

    }

    printf("Truck leaves...\n\n");

    if (semaphores != -1)
    {
        semctl(semaphores,0,IPC_RMID);
    }
    shmdt(line);

    if (shared_memory_id != -1)
    {
        shmctl(shared_memory_id,IPC_RMID,NULL);
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


    key_t key = ftok(PATH_SEM,CHAR);

    semaphores = semget(key,sem_no, IPC_CREAT | S_IRWXU);

    if (semaphores == -1)
    {
        printf("Error during creating semaphores\n");
        exit(1);
    }

    union semun arg;
    arg.val = 1;
    //setting possibility to load/take
    semctl(semaphores,0,SETVAL,arg);
    arg.val = 1;
    //setting 1 to be able to wait
    semctl(semaphores,1,SETVAL,arg);

    arg.val = 0;
    //setting no of packages on line
    semctl(semaphores,2,SETVAL,arg);

    // 0 - no end of work, 1 - end of work
    semctl(semaphores,3,SETVAL,0);


    key = ftok(PATH_MEM,CHAR);

    shared_memory_id = shmget(key,sizeof(struct line),S_IRWXU | IPC_CREAT);

    line = shmat(shared_memory_id,NULL,0);

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

    struct sembuf take_0;
    take_0.sem_num = 0;
    take_0.sem_op = -1;
    take_0.sem_flg = 0;

    struct sembuf give_0;
    give_0.sem_num = 0;
    give_0.sem_op = 1;
    give_0.sem_flg = 0;

    struct sembuf take_2;
    take_2.sem_num = 2;
    take_2.sem_op = -1;
    take_2.sem_flg = 0;

//    struct sembuf give_2;
//    give_2.sem_num = 0;
//    give_2.sem_op = 1;
//    give_2.sem_flg = 0;

    //struct timeval start;


    truck = X;
    truck_arrives();

    while (1)
    {

        printf("Waiting for package...\n");
        time_get();
        //waiting for non empty line
        while (semctl(semaphores,2,GETVAL) == 0)
        {
            if (semctl(semaphores, 1, GETVAL) == 0) {
                printf("All workers ended work\n");
                printf("Truck leaves...\n");
                time_get();
                exit(0);
            }
            usleep(40);
        }

        //waiting for turn
        semop(semaphores,&take_0,1);

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
        semop(semaphores,&take_2,1);


        line_statistics();

        line->take_index=(line->take_index+1)%line->max_pcs;

        if (truck == 0)
        {
            new_truck();

            if ((semctl(semaphores, 2, GETVAL) == 0) && semctl(semaphores, 1, GETVAL) == 0)
            {
                printf("All workers ended work\n");
                printf("Truck leaves...\n");
                time_get();
                exit(0);
            }
            else truck_arrives();
        }
        if ((semctl(semaphores, 2, GETVAL) == 0) && semctl(semaphores, 1, GETVAL) == 0) {
            printf("All workers ended work\n");
            printf("Truck leaves...\n");
            time_get();
            exit(0);
        }



        semop(semaphores,&give_0,1);
        if (for_end) exit(0);

    }

    return(0);

}




