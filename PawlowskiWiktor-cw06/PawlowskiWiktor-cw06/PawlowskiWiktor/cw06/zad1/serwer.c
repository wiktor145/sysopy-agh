#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

int RUNNING = 1;

int queue;
struct client clients[10];
//int clients_no = 0;

int init (struct message *);
int echo (struct message* );
void list (void);
int friends (struct message *);
int _2all (struct message *);
int _2friends (struct message *);
int _2one (struct message *);
void stop (struct message *);
int add (struct message *);
int del (struct message *);
char * prepare(struct message*);

void end (int);

/*
 * max 10 klientow
 * id klienta -1 oznacaza, ze go ni ma
 */

int main(void)
{
    struct sigaction act;
    act.sa_handler = end;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    char* home = getenv("HOME");
    if (!home)
    {
        printf("Error home\n");
        exit(1);
    }

    key_t key = ftok(home,'k');
    //printf("%d\n\n",key);

    msgctl(key,IPC_RMID,NULL);
    queue = msgget(key, IPC_CREAT  | 0666);

    if (queue == -1)
    {
        printf("Error during creating main queue!\n");
        exit(1);
    }

    for (int i=0; i<10; i++) clients[i].id = -1;


    struct message msg;// = malloc(sizeof(struct message));

    while (RUNNING)
    {
        msgrcv(queue,&msg,MESSAGESIZE,-4,0);

        switch (msg.command_type)
        {
            case INIT:
                if (init(&msg) != 0) printf("Error in init!\n");
                else
                    printf("Wykonano INIT dla %d\n",msg.id);
                break;
            case ECHO:
                if (echo(&msg) != 0) printf("Error in init!\n");
                else
                    printf("Wykonano ECHO dla %d\n",msg.id);
                break;
            case LIST:
                list();
                break;

            case FRIENDS:
                if (friends(&msg) != 0) printf("Error in friends! tried to friend a non existing client\n");
                else
                    printf("Wykonano FRIENDS dla %d\n",msg.id);
                break;
            case _2ALL:
                if (_2all(&msg) != 0) printf("Error in 2all!\n");
                else
                    printf("Wykonano 2ALL dla %d\n",msg.id);
                break;

            case _2FRIENDS:
                if (_2friends(&msg) != 0) printf("Error in 2friends!\n");
                else
                    printf("Wykonano 2FRIENDS dla %d\n",msg.id);
                break;

            case _2ONE:
                if (_2one(&msg) != 0) printf("Error in 2one!\n");
                else
                    printf("Wykonano 2ONE dla %d\n",msg.id);
                break;


            case STOP:
                stop(&msg);

                break;
            case ADD:
                if (add(&msg) != 0) printf("Error in ADD! tried to add a non existing client\n");
                else
                    printf("Wykonano ADD dla %d\n",msg.id);
                break;
            case DEL:
                if (del(&msg) != 0) printf("Error in DEL! tried to delete a non existing client\n");
                else
                    printf("Wykonano DEL dla %d\n",msg.id);
                break;
        }


    }


    msgctl(queue,IPC_RMID,NULL);

    return 0;
}


int init (struct message * msg)
{
    int clients_no = -1;

    for (int i=0; i<10; i++)
        if (clients[i].id == -1)
        {
            clients_no = i;
            break;
        }


    if (clients_no == -1)
    {
        printf("Only 10 clients are supported!\n");
        return -1;
    }

    clients[clients_no].id=clients_no;
    clients[clients_no].client_queue = msgget(msg->id,0);

    if (!clients[clients_no].client_queue)
    {
        printf("Error during opening queue!\n");
        return -1;
    }

    for (int i=0; i<10; ++i) clients[clients_no].friends[i]=0;

    struct message m;
    m.mtype=1;
    m.command_type = GIVEID;
    m.id = clients_no;
    m.command[0]='\0';

    msgsnd(clients[clients_no].client_queue,&m,MESSAGESIZE,0);
    //printf("%d\n",clients[clients_no].client_queue);
    //clients_no++;
    return 0;
}

//max echo message: 70
int echo (struct message* msg)
{
    char text[30];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(text, sizeof(text)-1, "%Y-%m-%d %H:%M:%S", t);

    //free(t);

    char echostring[COMMANDSIZE];

    int i=0;

    while (i<COMMANDSIZE && msg->command[i]!='\0')
    {
        echostring[i]=msg->command[i];
        i++;
    }
    echostring[i++]=' ';
    if (i > 69)
    {
        printf("to long echo message!\n");
        return -1;
    }

    int j=0;
    while (i<COMMANDSIZE && text[j]!='\0')
    {
        echostring[i]=text[j];
        i++;
        j++;
    }
    echostring[i]='\0';

    struct message m;

    m.command_type=ECHO;
    m.mtype=1;

    for (int i=0; i<COMMANDSIZE; i++)
    {
        m.command[i]=echostring[i];
        if (echostring[i] == '\0') break;
    }

    msgsnd(clients[msg->id].client_queue,&m,MESSAGESIZE,0);

    return 0;

}

void list (void)
{
    //jesli id == -1 to juz go nie ma albo nigdy nie było

    printf("Aktywni klienci:\n");
    for (int i=0; i<10; ++i)
        if (clients[i].id!= -1)
            printf("Id: %d\n",i);
    return;
}


int friends (struct message *msg)
{
    // ma byc w char[100] id spacja id spacja id nullbyte
    //id sa jednocyfrowe
    // jesli proba dodania frienda ktory nie istnieje => blad
    //pusty friends musi miec nullbyte w message

    int client = msg->id;

    for (int i=0; i<10; i++) clients[client].friends[i] = 0;

    //char c;
    int nr;
    for (int i=0; i<COMMANDSIZE && msg->command[i]!= '\0'; i++)
    {
        if (msg->command[i] == ' ') continue;

        nr = atoi(&msg->command[i]);
        if (clients[nr].id == -1) return -1;
        else
            clients[client].friends[nr] = 1;
    }

    return 0;
}

int add (struct message *msg)
{
    int client = msg->id;

    int nr;
    for (int i=0; i<COMMANDSIZE && msg->command[i]!= '\0'; i++)
    {
        if (msg->command[i] == ' ') continue;
        nr = atoi(&msg->command[i]);
        if (clients[nr].id == -1) return -1;
        else
            clients[client].friends[nr] = 1;
    }

    return 0;
}

int del (struct message *msg)
{
    int client = msg->id;

    int nr;
    for (int i=0; i<COMMANDSIZE && msg->command[i]!= '\0'; i++)
    {
        if (msg->command[i] == ' ') continue;
        nr = atoi(&msg->command[i]);
        if (clients[nr].id == -1) return -1;
        else
            clients[client].friends[nr] = 0;
    }

    return 0;
}

char * prepare (struct message * msg)
{
    char text[30];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(text, sizeof(text)-1, "%Y-%m-%d %H:%M:%S", t);

    //free(t);

    char *echostring = malloc(COMMANDSIZE*sizeof(char));

    int i=0;

    while (i<COMMANDSIZE && msg->command[i]!='\0')
    {
        echostring[i]=msg->command[i];
        i++;
    }
    echostring[i++]=' ';
    if (i > 69)
    {
        printf("to long echo message!\n");
        free(echostring);
        return NULL;
    }

    echostring[i++] = (char)(msg->id+ '0');
    echostring[i++]=' ';

    int j=0;
    while (i<COMMANDSIZE && text[j]!='\0')
    {
        echostring[i]=text[j];
        i++;
        j++;
    }
    echostring[i] = '\0';
    //echostring[COMMANDSIZE]='\0';
    return echostring;
}

int _2all (struct message *msg)
{
    char * mess = prepare(msg);
    if (mess == NULL)
    {
        printf("Message error!\n");
        return 1;
    }

    struct message m;

    for (int i=0; i<COMMANDSIZE; i++) m.command[i]=mess[i];

    m.mtype=1;
    m.command_type=_2ALL;

    for (int i=0; i<10; i++)
        if (clients[i].id != -1)
            msgsnd(clients[i].client_queue,&m,MESSAGESIZE,0);

    free(mess);
    return 0;

}


int _2friends (struct message *msg)
{
    char * mess = prepare(msg);
    if (mess == NULL)
    {
        printf("Message error!\n");
        return 1;
    }

    struct message m;//= malloc(sizeof(struct message));

    for (int i=0; i<COMMANDSIZE; i++) m.command[i]=mess[i];

    m.mtype=1;
    m.command_type=_2FRIENDS;


    for (int i=0; i<10; i++)
        if (clients[msg->id].friends[i] != 0)
            msgsnd(clients[i].client_queue,&m,MESSAGESIZE,0);

    free(mess);
    return 0;

}


int _2one (struct message *msg)
{
    char * mess = prepare(msg);

    int ide = (int)(mess[0]- '0');
    //printf("%d\n",id);
    mess[0]=' ';
    //printf("%d\n",id);
    if (mess == NULL)
    {
        printf("Message error!\n");
        return 1;
    }
   // printf("%d\n",id);
    if (clients[ide].id == -1)
    {
        printf("Given client id does not exist!\n");
        return -1;
    }
   // printf("%d\n",id);
    struct message m;// = malloc(sizeof(struct message));
    //printf("%d\n",ide);
    for (int a=0; a<COMMANDSIZE; a++) m.command[a]=mess[a];
    //printf("%d\n",ide);
    m.mtype=1;
    m.command_type=_2ONE;

    //printf("%d\n",id);
    msgsnd(clients[ide].client_queue,&m,MESSAGESIZE,0);
    //printf("%d\n",clients[id].client_queue);

    free(mess);
    //free(m);
    return 0;

}

void stop (struct message *msg)
{
    clients[msg->id].id = -1;
    for (int i=0; i<10; i++)
        clients[i].friends[msg->id]=0;
}

void end (int i)
{
    struct message m;// = malloc(sizeof(struct message));

    m.mtype=3;
    m.command_type=SERVERSTOP;
    m.command[0]='\0';


    for (int i=0; i<10; i++)
        if (clients[i].id != -1)
            msgsnd(clients[i].client_queue,&m,MESSAGESIZE,0);

    RUNNING = 0;
    msgctl(queue,IPC_RMID,NULL);

    exit(0);


}