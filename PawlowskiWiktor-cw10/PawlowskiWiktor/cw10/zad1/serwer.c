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

void new_client(int socket)
{
    int client = accept(socket, NULL, NULL);
    if (client == -1)
    {
        printf("Error, could not accept new client\n");
        exit(4);
    }

    struct epoll_event newclient;

    newclient.events = EPOLLIN | EPOLLPRI;
    newclient.data.fd = client;

    if (epoll_ctl(EPOLL, EPOLL_CTL_ADD, client, &newclient) == -1)
    {
        printf("Error, could not add new client to epoll\n");
        exit(89);
    }

}

void make_client(char* name, int socket)
{
    pthread_mutex_lock(&clients_mutex);

    char message_type;

    if(clients_number == MAX_CLIENTS)
    {

        message_type = FAIL_TO_MUCH_CLIENTS;
        write(socket, &message_type, 1);

        epoll_ctl(EPOLL, EPOLL_CTL_DEL, socket, NULL);

        shutdown(socket, SHUT_RDWR);

        close(socket);
    }
    else

        {

        int exists = -1;

            for (int a = 0; a<clients_number; ++a)
            {
                if (strcmp(clients[a].name,name) == 0)
                {
                    exists=a;
                    break;
                }
            }

        if(exists != -1){

            message_type = FAIL_NAME_EXIST;
            write(socket,&message_type , 1);

            epoll_ctl(EPOLL, EPOLL_CTL_DEL, socket, NULL);

            shutdown(socket, SHUT_RDWR);

            close(socket);
        }
        else{

            clients[clients_number].fd = socket;
            clients[clients_number].name = malloc(strlen(name) + 1);
            clients[clients_number].active = 1;
            clients[clients_number].working = 0;
            strcpy(clients[clients_number].name, name);
            clients_number++;
            message_type = SUCCESS;
            write(socket,&message_type, 1);
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

        epoll_ctl(EPOLL, EPOLL_CTL_DEL, clients[i].fd, NULL);

        shutdown(clients[i].fd, SHUT_RDWR);

        close(clients[i].fd);

        free(clients[i].name);

        --clients_number;

        for (int j = i; j < clients_number; ++j)
            clients[j] = clients[j + 1];

        printf("Client deleted\n");
    }
    pthread_mutex_unlock(&clients_mutex);

}



void message (int socket)
{
    char message_type;

    uint16_t name_size;

    if (read(socket, &message_type, 1) != 1)
    {
        printf("Error, could not read message type\n");
        exit(6);
    }

    if (read(socket, &name_size, 2) != 2)
    {
        printf("Error, could not read name sze\n");
        exit(8);
    }

    char * client_name = malloc(name_size);

    switch (message_type)
    {
        case DELETE_ME:
        {
            if (read(socket,client_name, name_size) != name_size)
            {
                printf("Error, could not receive clients name \n");
                exit(81);
            }
            delete_client(client_name);
            break;
        }
        case NEW_CLIENT:
        {
            if (read(socket,client_name, name_size) != name_size)
            {
                printf("Error, could not receive clients name \n");
                exit(81);
            }
            make_client(client_name,socket);
            break;
        }
        case RESULT:
        {
            if (read(socket,client_name, name_size) != name_size)
            {
                printf("Error, could not receive clients name \n");
                exit(81);
            }

            result_t res;

            read(socket,&res, sizeof(res));

            printf("Client \"%s\" calculated request [%d]. Result: %d\n", client_name, res.request_no, res.counted_words);

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
                clients[i].working--;
            }
            pthread_mutex_unlock(&clients_mutex);

            break;
        }
        case PING_ANSWER:
        {
            if (read(socket,client_name, name_size) != name_size)
            {
                printf("Error, could not receive clients name \n");
                exit(81);
            }

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
                clients[i].active = 1;
            }
            pthread_mutex_unlock(&clients_mutex);


            break;
        }
        default:
        {
            printf("Error, got unexpected message type %d \n",message_type);
            break;
        }
    }

    free(client_name);
}



void *ping_thread(void *arg)
{
    char msg = PING;

    while (1)
    {
        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < clients_number; ++i)
        {
            if (clients[i].active == 0) {
                printf("Client \"%s\" not responding. Deleting...\n", clients[i].name);

                epoll_ctl(EPOLL, EPOLL_CTL_DEL, clients[i].fd, NULL);

                shutdown(clients[i].fd, SHUT_RDWR);

                close(clients[i].fd);

                free(clients[i].name);

                --clients_number;

                for (int j = i; j < clients_number; ++j)
                    clients[j] = clients[j + 1];

                printf("Client deleted\n");
                i--;
            }
            else
                {
                write(clients[i].fd, &msg, 1);
                clients[i].active = 0;
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

        if (event.data.fd < 0)
            new_client(-event.data.fd);
        else
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

    //inicjalizacja

    uint16_t port_num = (uint16_t) atoi(argv[1]);
    if (port_num < 1024 || port_num > 65535)
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
    web_address.sin_addr.s_addr = htonl(INADDR_ANY);
    web_address.sin_port = htons(port_num);

    if ((WEB_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error, could not create web sochet!\n");
        exit(11);
    }


    if (bind(WEB_SOCKET, (const struct sockaddr *) &web_address, sizeof(web_address)))
    {
        printf("Error, could not bind web socket!\n");
        exit(12);

    }

    if (listen(WEB_SOCKET, MAX_CLIENTS) == -1)
    {
        printf("Error, coulnd not listen!\n");
        exit(1);
    }



    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;

    snprintf(local_address.sun_path, UNIX_PATH_MAX, "%s", UNIX_PATH);

    if ((LOCKAL_SOCKET = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        printf("Error, could not create local sochet!\n");
        exit(11);
    }

    if (bind(LOCKAL_SOCKET, (const struct sockaddr *) &local_address, sizeof(local_address)))
    {
        printf("Error, could not bind local socket!\n");
        exit(12);

    }

    if (listen(LOCKAL_SOCKET, MAX_CLIENTS) == -1)
    {
        printf("Error, coulnd not listen!\n");
        exit(1);
    }


    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((EPOLL = epoll_create1(0)) == -1)
    {
        printf("Error! cant create epoll  \n");
        exit(8);

    }


    event.data.fd = -WEB_SOCKET;
    if(epoll_ctl(EPOLL, EPOLL_CTL_ADD, WEB_SOCKET, &event) == -1)
    {
        printf("Error! cant add web socket to epolll  \n");
        exit(8);

    }

    event.data.fd = -LOCKAL_SOCKET;
    if(epoll_ctl(EPOLL, EPOLL_CTL_ADD, LOCKAL_SOCKET, &event) == -1)
    {
        printf("Error! cant add local socket to epolll   \n");
        exit(8);

    }

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
                char a;
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

               // printf(" text size: %d\n\n",text_size);


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

                a = COUNT_ALL;

                int error = 0;

                if (write(clients[min].fd, &a, 1) != 1)
                {
                    printf("Could not send message!\n");
                    error = 1;
                }

                if (write(clients[min].fd, &request_no, 2) != 2)
                {
                    printf("Could not send request no!\n");
                    error = 1;
                }

                request_no++;

               // printf(" text size: %d\n\n",text_size);

                if (write(clients[min].fd, &text_size, 2) != 2)
                {
                    printf("Could not send text size!\n");
                    error = 1;
                }

                if (write(clients[min].fd, file_text, text_size) != text_size)
                {
                    printf("Could not send text!\n");
                    error = 1;
                }

                if (error == 0)
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
                char a;
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

                a = COUNT_SPECIFIC;

                int error = 0;

                if (write(clients[min].fd, &a, 1) != 1)
                {
                    printf("Could not send message!\n");
                    error = 1;
                }

                if (write(clients[min].fd, &request_no, 2) != 2)
                {
                    printf("Could not send request no!\n");
                    error = 1;
                }

                request_no++;

                if (write(clients[min].fd, &word_size, 1) != 1)
                {
                    printf("Could not send word size!\n");
                    error = 1;
                }

                if (write(clients[min].fd, word, word_size) != word_size)
                {
                    printf("Could not send word!\n");
                    error = 1;
                }

                if (write(clients[min].fd, &text_size, 2) != 2)
                {
                    printf("Could not send text size!\n");
                    error = 1;
                }

                if (write(clients[min].fd, file_text, text_size) != text_size)
                {
                    printf("Could not send text!\n");
                    error = 1;
                }

                if (error == 0)
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
