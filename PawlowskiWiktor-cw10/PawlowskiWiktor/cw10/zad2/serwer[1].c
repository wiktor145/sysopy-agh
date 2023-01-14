#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "common.h"
#include <arpa/inet.h>
#include <sys/epoll.h>

char * UNIX_PATH;
int WEB_SOCKET;
int LOCKAL_SOCKET;
int EPOLL;

pthread_t ping;
pthread_t messages;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
Client clients[MAX_CLIENTS];
short clients_number = 0;

uint16_t request_no = 0;

void signal_handler(int sig)
{
    printf("SIGINT!\n");
    exit(1);
}

void at_exit_fun(void)
{
    pthread_cancel(messages);
    pthread_cancel(ping);


    if (close(WEB_SOCKET) == -1)
        printf( "Error : Could not close Web Socket\n");

    if (close(LOCKAL_SOCKET) == -1)
        printf("Error : Could not close Local Socket\n");

    if (unlink(UNIX_PATH) == -1)
        printf("Error : Could not unlink Unix Path\n");

    if (close(EPOLL) == -1)
        printf("Error : Could not close epoll\n");


}


void make_client(result_t msg, int socket, struct sockaddr *sockaddr, socklen_t socklen)
{
    pthread_mutex_lock(&clients_mutex);

    Request message_type;

    if(clients_number == MAX_CLIENTS)
    {

        message_type.message_type = FAIL_TO_MUCH_CLIENTS;
        sendto(socket, &message_type, sizeof(message_type), 0, sockaddr, socklen);
        free(sockaddr);
    }
    else

        {

        int exists = -1;

            for (int a = 0; a<clients_number; ++a)
            {
                if (strcmp(clients[a].name,msg.client_name) == 0)
                {
                    exists=a;
                    break;
                }
            }

        if(exists != -1){

            message_type.message_type = FAIL_NAME_EXIST;
            sendto(socket, &message_type, sizeof(message_type), 0, sockaddr, socklen);

            free(sockaddr);
        }
        else{

            clients[clients_number].socket_addr = malloc(sizeof(struct sockaddr_un));
            memcpy(clients[clients_number].socket_addr, sockaddr, socklen);
            clients[clients_number].socket_len = socklen;
            clients[clients_number].connection_type = msg.connection_type;


            clients[clients_number].name = malloc(strlen(msg.client_name) + 1);
            clients[clients_number].active = 1;
            clients[clients_number].working = 0;
            strcpy(clients[clients_number].name, msg.client_name);
            clients_number++;
            message_type.message_type = SUCCESS;
            sendto(socket, &message_type, sizeof(message_type), 0, sockaddr, socklen);
        }
    }
    pthread_mutex_unlock(&clients_mutex);

}



void delete_client(char* client_name)
{
    pthread_mutex_lock(&clients_mutex);
    int i = -1;

    for (int a = 0; a<clients_number; ++a)
    {
        if (strcmp(clients[a].name,client_name) == 0)
        {
            i=a;
            break;
        }
    }

    if(i >= 0){

        free(clients[i].name);
        free(clients[i].socket_addr);

        --clients_number;

        for (int j = i; j < clients_number; ++j)
            clients[j] = clients[j + 1];

        printf("Client deleted\n");
    }
    pthread_mutex_unlock(&clients_mutex);

}



void message (int socket)
{
    result_t res;

    struct sockaddr* sockaddr = malloc(sizeof(struct sockaddr));
    socklen_t socklen = sizeof(struct sockaddr);

    if (recvfrom(socket, &res, sizeof(res),0,sockaddr,&socklen) != sizeof(res))
    {
        printf("Error, could not read message\n");
        exit(6);
    }


    switch (res.message_type)
    {
        case DELETE_ME:
        {
            delete_client(res.client_name);
            break;
        }
        case NEW_CLIENT:
        {
            make_client(res,socket,sockaddr,socklen);
            break;
        }
        case RESULT:
        {
            printf("Client %s calculated request %d. Result: %d\n", res.client_name, res.request_no, res.counted_words);

            pthread_mutex_lock(&clients_mutex);
            int i = -1;

            for (int a = 0; a<clients_number; ++a)
            {
                if (strcmp(clients[a].name,res.client_name) == 0)
                {
                    i=a;
                    break;
                }
            }

            if(i >= 0){
                clients[i].working--;
            }
            pthread_mutex_unlock(&clients_mutex);

            break;
        }
        case PING_ANSWER:
        {

            pthread_mutex_lock(&clients_mutex);
            int i = -1;

            for (int a = 0; a<clients_number; ++a)
            {
                if (strcmp(clients[a].name,res.client_name) == 0)
                {
                    i=a;
                    break;
                }
            }

            if(i >= 0){
                clients[i].active = 1;
            }
            pthread_mutex_unlock(&clients_mutex);


            break;
        }
        default:
        {
            printf("Error, got unexpected message type %d \n",res.message_type);
            break;
        }
    }


}



void *ping_thread(void *arg)
{
    Request ping;
    ping.message_type = PING;

    while (1)
    {
        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < clients_number; ++i)
        {
            if (clients[i].active == 0) {
                printf("Client \"%s\" not responding. Deleting...\n", clients[i].name);

                free(clients[i].name);
                free(clients[i].socket_addr);

                --clients_number;

                for (int j = i; j < clients_number; ++j)
                    clients[j] = clients[j + 1];

                printf("Client deleted\n");
                i--;
            }
            else
                {
                if (clients[i].connection_type == 'W')
                {
                    if (sendto(WEB_SOCKET,&ping,sizeof(ping),0,clients[i].socket_addr,clients[i].socket_len)
                    != sizeof(ping))
                    {
                        printf("Error, cant PING client!\n");
                       // exit(1);
                    }
                    clients[i].active=0;

                }
                else
                {
                    if (sendto(LOCKAL_SOCKET,&ping,sizeof(ping),0,clients[i].socket_addr,clients[i].socket_len)
                        != sizeof(ping))
                    {
                        printf("Error, cant PING client!\n");
                       // exit(1);
                    }
                    clients[i].active=0;


                }

            }
        }
        pthread_mutex_unlock(&clients_mutex);
        sleep(10);
    }
}

void *messages_thread(void *arg)
{
    struct epoll_event event;
    while (1)
    {
        if (epoll_wait(EPOLL, &event, 1, -1) == -1)
        {
            printf("Error, epoll failed!\n");
            exit(4);
        }

       message(event.data.fd);
    }

}


int main (int argc, char** argv)
{
    signal(SIGINT,signal_handler);

    if (argc != 3)
    {
        printf("Error, program serwer should be executed with 2 args: \n");
        printf("1 - port number\n");
        printf("2 - UNIX path\n");
        exit(1);
    }

    if (atexit(at_exit_fun) == -1)
    {
        printf("Error during setting atexit function!\n");
        exit(2);
    }



    in_port_t portID;
    char *dump;
    portID = (in_port_t)strtol(argv[1], &dump, 10);
    if (portID < 1024 || portID > 65535)
    {
        printf("Error, wrong port number (sholud be between 1025 and 65535!\n");
        exit(8);
    }

    UNIX_PATH = argv[2];
    if (strlen(UNIX_PATH) < 1 || strlen(UNIX_PATH) > UNIX_PATH_MAX)
    {
        printf("Error in unix path %s !",argv[2]);
        exit(9);
    }


    struct sockaddr_in web_address;

    memset(&web_address, 0, sizeof(struct sockaddr_in));
    web_address.sin_family = AF_INET;
    web_address.sin_addr.s_addr = INADDR_ANY;
    web_address.sin_port = htons(portID);

    if ((WEB_SOCKET = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error, could not create web socket!\n");
        exit(11);
    }


    if (bind(WEB_SOCKET, (const struct sockaddr *) &web_address, sizeof(web_address)))
    {
        printf("Error, could not bind web socket!\n");
        exit(12);

    }

    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;

    snprintf(local_address.sun_path, UNIX_PATH_MAX, "%s", UNIX_PATH);

    if ((LOCKAL_SOCKET = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        printf("Error, could not create local sochet!\n");
        exit(11);
    }

    if (bind(LOCKAL_SOCKET, (const struct sockaddr *) &local_address, sizeof(local_address)))
    {
        printf("Error, could not bind local socket!\n");
        exit(12);

    }

    EPOLL = epoll_create(2137);

    union epoll_data unixEpollData = {
            .fd = LOCKAL_SOCKET
    };
    struct epoll_event unixEpollEvent = {
            .events = EPOLLIN,
            .data = unixEpollData
    };
    epoll_ctl(EPOLL, EPOLL_CTL_ADD, LOCKAL_SOCKET, &unixEpollEvent);

    union epoll_data inetEpollData = {
            .fd = WEB_SOCKET
    };
    struct epoll_event inetEpollEvent = {
            .events = EPOLLIN,
            .data = inetEpollData
    };
    epoll_ctl(EPOLL, EPOLL_CTL_ADD, WEB_SOCKET, &inetEpollEvent);

    if (pthread_create(&ping, NULL, ping_thread, NULL) != 0)
    {
        printf("Error!  cant start ping thread \n");
        exit(8);

    }
    if (pthread_create(&messages, NULL, messages_thread, NULL) != 0)
    {
        printf("Error! cant start messages thread! \n");
        exit(8);

    }



    while (2)
    {
        while (clients_number == 0)
        {
            printf("By now, there are no active clients... waiting...\n");
            sleep(2);
        }

        printf("Type 1 for counting all words\n");
        printf("Type 2 for counting specific word\n");

        int a;
        scanf("%d",&a);

        switch (a)
        {
            case 1:
            {
                Request a;
                printf("Type path to the file\n");

                char file[256];
                scanf("%s",file);

                FILE *f = fopen(file,"r");

                if (f == NULL)
                {
                    printf("Error during opening file %s\n",file);
                    continue;
                }

                char file_text[MAX_BUFFER];

                fgets(file_text,MAX_BUFFER,f);

                uint16_t text_size = strlen(file_text) + 1;




                if (clients_number == 0)
                {
                    printf("Sorry, no clients connected :(\n");
                    continue;
                }

                int min = 0;

                pthread_mutex_lock(&clients_mutex);

                for (int i = 1; i < clients_number; ++i)
                {
                    if (clients[i].working < clients[min].working)
                        min = i;
                }

                clients[min].working++;

                a.message_type = COUNT_ALL;

                a.request_no = request_no;

                request_no++;

                a.text_size = text_size;

                strcpy(a.text,file_text);

                int destination = clients[min].connection_type == 'W' ? WEB_SOCKET : LOCKAL_SOCKET;

                if (sendto(destination,&a, sizeof(a),0,clients[min].socket_addr,clients[min].socket_len)
                == sizeof(a))
                {
                    printf("Request %d file %s send to client %s\n",request_no-1, file,clients[min].name);
                }
                else
                {
                    printf("ERROR! could not send Request %d file %s send to client %s\n",request_no-1, file,clients[min].name);
                }

                pthread_mutex_unlock(&clients_mutex);

                break;
            }
            case 2:
            {
                Request a;
                printf("\nType word to search (MAX 10 characters!)\n");
                char word[MAX_WORD_TO_SEARCH_LENGTH+1];

                scanf("%10s",word);

                uint8_t word_size = strlen(word) + 1;

                printf("\nType path to the file\n");

                char file[256];
                scanf("%s",file);

                FILE *f = fopen(file,"r");

                if (f == NULL)
                {
                    printf("Error during opening file %s\n",file);
                    continue;
                }

                char file_text[MAX_BUFFER];

                fgets(file_text,MAX_BUFFER,f);

                uint16_t text_size = strlen(file_text) + 1;

                printf(" text size: %d\n\n",text_size);

                if (clients_number == 0)
                {
                    printf("Sorry, no clients connected :(\n");
                    continue;
                }

                int min = 0;

                pthread_mutex_lock(&clients_mutex);

                for (int i = 1; i < clients_number; ++i)
                {
                    if (clients[i].working < clients[min].working)
                        min = i;
                }

                clients[min].working++;

                a.message_type = COUNT_SPECIFIC;

                a.request_no = request_no;

                request_no++;

                a.word_size = word_size;

                strcpy(a.word,word);

                a.text_size = text_size;

                strcpy(a.text,file_text);

                int destination = clients[min].connection_type == 'W' ? WEB_SOCKET : LOCKAL_SOCKET;

                if (sendto(destination,&a, sizeof(a),0,clients[min].socket_addr,clients[min].socket_len)
                    == sizeof(a))
                {
                    printf("Request %d file %s send to client %s\n",request_no-1, file,clients[min].name);
                }
                else
                {
                    printf("ERROR! could not send Request %d file %s send to client %s\n",request_no-1, file,clients[min].name);
                }

                pthread_mutex_unlock(&clients_mutex);

                break;
            }
            default:
            {
                printf("Wrong command!\n");
                break;
            }

        }

    }

    return 0;
}
