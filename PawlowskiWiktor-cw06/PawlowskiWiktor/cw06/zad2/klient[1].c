#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

mqd_t queue = -1;
mqd_t myqueue = -1;
int MYID;

char name[3]="/a\0";

struct message msg;

void wyjscie (void)
{
    if (queue != -1) mq_close(queue);
    if (myqueue != -1)
        while (mq_unlink(name) == -1) {usleep(400000);}
}

void clearmsg(void)
{
    msg.id=MYID;
    for (int i=0; i<COMMANDSIZE; i++) msg.command[i]='\0';
}

void end(int i)
{
    msg.mtype=3;
    msg.command_type=STOP;
    mq_send(queue,(char*)&msg,MESSAGESIZE,3);

    exit(0);
}

void commandlist(void)
{
    printf("\nPossible commands:\n");
    printf("STOP - stops\n");
    printf("LIST - lists (server) all working clients\n");
    printf("FRIENDS NR1 NR2 ... - replace friends list\n");
    printf("ADD a b c ... - adds a b c ... to friends\n");
    printf("DEL a ... - deletes a ... from friends\n");
    printf("2ALL message - sends message to all working cilents\n");
    printf("2FRIENDS message - sends message to all friends\n");
    printf("2ONE id message - sends mesage to id\n");
    printf("ECHO message - echo!!!!!\n");
    printf("\n");
}


int main(int argc, char** argv)
{

    struct sigaction act;
    act.sa_handler = end;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    atexit(wyjscie);

    queue = mq_open(name, O_WRONLY);

    if (queue == -1)
    {
        printf("Error during creating main queue!\n");
        exit(1);
    }

    struct mq_attr posix_attr;
    posix_attr.mq_maxmsg = MAX_QUEUE_SIZE;
    posix_attr.mq_msgsize = MESSAGESIZE;

    myqueue = mq_open(name,O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0666,&posix_attr);


    while (myqueue == -1)
    {
        name[1]++;
        myqueue = mq_open(name,O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0666,&posix_attr);
    }

    struct message init_message;
    init_message.mtype = 1;
    for (int i=0; i<3; i++) init_message.name[i]=name[i];
    init_message.command_type = INIT;

    mq_send(queue,(char*)&init_message,MESSAGESIZE,1);

    mq_open("/a\0", IPC_EXCL | IPC_CREAT);
    if (errno == EEXIST)
    {
        errno = 0;
       // printf("ok\n");
    }
    else
    {
        exit(1);
    }

    mq_receive(myqueue,(char*)&msg,MESSAGESIZE,NULL);

    if (errno == EIDRM || msg.command_type == SERVERSTOP)
    {
        exit(1);
    }

    MYID = msg.id;

    int file_mode = 0;
    FILE *f;

    if (argc > 1)
    {
        f = fopen(argv[1],"r");

        if (!f)
        {
            printf("Error opening file %s\n",argv[1]);
            exit(2);
        }
        file_mode = 1;

    }
    char buff[200];
    char comm[20];
    char arg[80];
    int i=0,j;

    while (1)
    {
        mq_receive(myqueue,(char*)&msg,MESSAGESIZE,NULL);
        if (errno == EAGAIN)
        {
            errno = 0;
        }
        else
        {
            if (msg.command_type == SERVERSTOP)
            {
                printf("Serwer zakonczyl dzialanie\n");
                exit(0);
            }
            else
            {
                printf("Odebrano wiadomosc:\n%s\n\n",msg.command);
                clearmsg();
            }

        }

        if (file_mode)
        {
            if (feof(f))
            {
                buff[0]='S';
                buff[1]='T';
                buff[2]='O';
                buff[3]='P';
                buff[4]='\0';
            }
            else fgets(buff,200,f);
            usleep(400000);

        }
        else
        {
            commandlist(); // napisac ma wyswietlac liste komend
            fgets(buff,200,stdin);
        }
        i = 0;
        while (i < 200 && buff[i]!='\0' && buff[i] != '\n' && buff[i] != ' ')
        {
            comm[i] = buff[i];
            i++;
        }
        comm[i++] = '\0';
        j = 0;
        while (i < 200 && buff[i]!='\0' && buff[i] != '\n')
        {
            arg[j]=buff[i];
            ++i;
            ++j;
        }
        arg[j++]='\0';
        if (j > MAXMSG)
        {
            printf("Arguments are too long!\n");
            clearmsg();
            continue;
        }

        if (strcmp(comm,"STOP\0") == 0)
        {
            msg.mtype=3;
            msg.command_type=STOP;
            mq_send(queue,(char*)&msg,MESSAGESIZE,3);
            usleep(400000);
            exit(0);
        }
        else if (strcmp(comm,"ECHO\0") == 0)
        {
            msg.mtype=1;
            msg.command_type=ECHO;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else if (strcmp(comm,"LIST\0") == 0)
        {
            msg.mtype=2;
            msg.command_type=LIST;
            mq_send(queue,(char*)&msg,MESSAGESIZE,2);

        }
        else if (strcmp(comm,"FRIENDS\0") == 0)
        {
            msg.mtype=2;
            msg.command_type=FRIENDS;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,2);
        }
        else if (strcmp(comm,"2ALL\0") == 0)
        {
            msg.mtype=1;
            msg.command_type=_2ALL;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else if (strcmp(comm,"2FRIENDS\0") == 0)
        {
            msg.mtype=1;
            msg.command_type=_2FRIENDS;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else if (strcmp(comm,"2ONE\0") == 0)
        {
            msg.mtype=1;
            msg.command_type=_2ONE;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else if (strcmp(comm,"ADD\0") == 0)
        {
            if (j == 1)
            {
                printf("Cant send empty ADD\n");
                continue;
            }
            msg.mtype=1;
            msg.command_type=ADD;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else if (strcmp(comm,"DEL\0") == 0)
        {
            if (j == 1)
            {
                printf("Cant send empty DEL\n");
                continue;
            }
            msg.mtype=1;
            msg.command_type=DEL;
            for (int i=0; i<j; i++) msg.command[i]=arg[i];
            mq_send(queue,(char*)&msg,MESSAGESIZE,1);

        }
        else
        {
            printf("Wrong action %s\n\n",arg);
        }

        clearmsg();
    }

    return 0;
}