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


int SOCKET;
char connection_type;
char * NAME;

void sig_handler (int signo)
{
    printf("SIGINT - exiting...\n");
    exit(0);
}

//sends beginning of message -
//message type
//NAME size
//NAME of client

void send_message(char message, uint16_t request_no, uint16_t result)
{
    result_t ret;

    ret.request_no = request_no;
    ret.message_type = message;
    ret.connection_type = connection_type;
    ret.counted_words = result;
    strcpy(ret.client_name,NAME);

    if(write(SOCKET, &ret, sizeof(result_t)) != sizeof(result_t))
    {
        printf("Error, could not send message\n");
        exit(4);
    }
    //printf("");

}


void at_exit(void)
{

    send_message(DELETE_ME,0,0);

    if (shutdown(SOCKET, SHUT_RDWR) == -1)
        printf("\nError : Could not shutdown Socket\n");
    if (close(SOCKET) == -1)
        printf("\nError : Could not close Socket\n");

}


void count_all(Request r)
{

    //counts words by counting white spaces
    uint16_t result = 0;

    for (int i=0; i<r.text_size; i++)
    {
        if (r.text[i] == ' ' || r.text[i] == '\t' || r.text[i] == '\n')
            result++;
    }

    send_message(RESULT,r.request_no,result);

}

void count_specific(Request r)
{
    uint16_t result = 0;

    int i=0, j=0;

    while (i<r.text_size)
    {

        if (r.text[i] == r.word[j])
            j++;
        else
            j = 0;

        if (j == r.word_size-1)
        {
            j=0;
            result++;
        }

        i ++;

    }

    send_message(RESULT,r.request_no,result);

}


int main (int argc, char** argv)
{
    signal(SIGINT, sig_handler);

    if (argc != 4 && argc!= 5)
    {
        printf("Error! Program klient should be executed with 3 args:\n");
        printf("1 - NAME; max 10 characters\n");
        printf("2 - WEB or LOCAL\n");
        printf("3 - Server address IPv4 or UNIX socket)\n");
        printf("4 - if WEB - port number\n");
        exit(1);
    }

    if (atexit(at_exit) == -1)
    {
        printf("Error, could not set atexit function\n");
        exit(2);
    }


    NAME = argv[1];

    if (strlen(NAME) > MAX_NAME_LENGTH)
    {
        printf("Error, NAME cant be longer than 10 characters!\n");
        exit(3);
    }

    if (strcmp(argv[2],"WEB") == 0)
    {

        uint16_t port_num = (uint16_t) atoi(argv[4]);
        if (port_num < 1024 || port_num > 65535) {
            printf("Error!\n");
            exit(1);
        }

        struct sockaddr_in web_address;
        memset(&web_address, 0, sizeof(struct sockaddr_in));

        web_address.sin_family = AF_INET;
        web_address.sin_addr.s_addr = htonl(
                INADDR_ANY);  //inet_addr(server_ip_path);//inet_addr("192.168.0.66");
        web_address.sin_port = htons(port_num);

        if ((SOCKET = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            printf("Error!\n");
            exit(1);
        }
        if (connect(SOCKET, (const struct sockaddr *) &web_address, sizeof(web_address)) == -1) {
            printf("Error!\n");
            exit(1);
        }
        printf("Connected to web socket \n");

        connection_type = 'W';

    }
    else if (strcmp(argv[2],"LOCAL") == 0)
    {

        char * path = argv[3];

        if (strlen(path) < 1 || strlen(path) > UNIX_PATH_MAX)
        {
            printf("Error in unix path NAME!\n");
            exit(5);
        }



        if ((SOCKET = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        {    printf("Error!\n");
            exit(1);
        }
        struct sockaddr_un local_address_bind;
        local_address_bind.sun_family = AF_UNIX;
        snprintf(local_address_bind.sun_path, UNIX_PATH_MAX, "%s", NAME);

        if (bind(SOCKET, (const struct sockaddr *) &local_address_bind, sizeof(local_address_bind)))
        {    printf("Error!\n");
        exit(1);
    }

        struct sockaddr_un local_address;
        local_address.sun_family = AF_UNIX;
        snprintf(local_address.sun_path, UNIX_PATH_MAX, "%s", path);

        if (connect(SOCKET, (const struct sockaddr *) &local_address, sizeof(local_address)) == -1)
        {    printf("Error!\n");
            exit(1);
        }

        printf("Connected to local socket \n");

        connection_type = 'L';

    }
    else
    {
        printf("Error, second argument should be either WEB or LOCAL\n");
        exit(4);
    }


    send_message(NEW_CLIENT,0,0);

    Request message_received;

    if (read(SOCKET,&message_received,sizeof(message_received)) != sizeof(message_received))
    {
        printf("Error, cant receive message!\n");
        exit(2);
    }


    switch (message_received.message_type)
    {
        case SUCCESS:
        {
            printf("Yeah! Successfully logged!");
            printf("\n");
            break;
        }
        case FAIL_NAME_EXIST:
        {
            printf("Error, server said, that client with this NAME %s\n",NAME);
            printf("Already exist... :( exiting...\n");
            exit(22);
            break;
        }

        case FAIL_TO_MUCH_CLIENTS:
        {
            printf("Error, server said, that it has already max number of clients :(\n");
            exit(23);
            break;
        }
        default:
        {
            printf("Wow, i did not expected that... error...\n");
            exit(42);
        }

    }
    printf("\n");



    while (1)
    {
        if (read(SOCKET,&message_received,sizeof(Request)) != sizeof(Request))
        {
            printf("Error during receiving message\n");
            exit(1);
        }
        switch (message_received.message_type)
        {
            case COUNT_ALL:
            {
                count_all(message_received);
                break;
            }
            case COUNT_SPECIFIC:
            {
                count_specific(message_received);
                break;
            }
            case PING:
            {
                send_message(PING_ANSWER,0,0);
                break;
            }
        }
    }

    return 0;
}






