#ifndef HEADER
#define HEADER

//priorytety:
// STOP  i SERVERSTOP- 3
// LIST, FRIENDS - 2
// reszta - 1


#define ECHO 1
#define LIST 2
#define FRIENDS 3
#define _2ALL 4
#define _2FRIENDS 5
#define _2ONE 6
#define STOP 7
#define INIT 8
#define GIVEID 9
#define ADD 10
#define DEL 11
#define SERVERSTOP 12

#define MESSAGESIZE (sizeof(struct message)-sizeof(long))
#define COMMANDSIZE 100

#define MAXMSG 70

struct client
{
    int id;
    int client_queue;
    int friends[10];
};



struct message
{
    long mtype;
    int id;
    int command_type;
    char command[100];
};

#endif //HEADER