//
// Created by wicia on 10.05.19.
//

#ifndef INC_07_COMMON_H
#define INC_07_COMMON_H

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
} arg;

//for ftok
#define PATH_SEM_0 "/racingorpingpong0"
#define PATH_SEM_1 "/racingorpingpong1"
#define PATH_SEM_2 "/racingorpingpong2"
#define PATH_SEM_3 "/racingorpingpong3"

#define PATH_MEM "/honestly"
#define CHAR 'x'

// max mass of one package
#define N 10

#define MAX_LINE_LENGTH 32

//semaphores
// 0 - can add/take package on/from line
// 1 - active loaders (or 1 at the begining)
// 2 - pieces on line
// 3 - end of days
int sem_no =4;

struct package
{
    short mass;
    pid_t pid;
    //time of trying
    struct timeval try_time;
};

struct line
{
    int max_weight;
    int max_pcs;

    int weight_left;
    int pcs_left;

    int load_index;
    int take_index;

    struct package  packages[MAX_LINE_LENGTH];

} *line;




#endif //INC_07_COMMON_H
