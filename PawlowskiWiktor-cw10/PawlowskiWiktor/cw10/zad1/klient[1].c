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

void send_beginning(char message)
{
    uint16_t NAME_size = (uint16_t) (strlen(NAME) + 1);
    if(write(SOCKET, &message, 1) != 1)
    {
        printf("Error, could not send message type\n");
        exit(2);
    }

    if(write(SOCKET, &NAME_size, 2) != 2)
    {
        printf("Error, could not send NAME size!\n");
        exit(3);
    }
    if(write(SOCKET, NAME, NAME_size) != NAME_size)
    {
        printf("Error, could not send NAME\n");
        exit(4);
    }
    //printf("");

}


void at_exit(void)
{

    send_beginning(DELETE_ME);

    if (shutdown(SOCKET, SHUT_RDWR) == -1)
        printf("\nError : Could not shutdown Socket\n");
    if (close(SOCKET) == -1)
        printf("\nError : Could not close Socket\n");

}


void count_all(void)
{
    uint16_t request_no;

    if (read(SOCKET,&request_no,2) != 2)
    {
        printf("Error, cant receive request no!\n");
        exit(2);
    }

    //printf("Request no: %d \n",request_no);

    uint16_t text_size;

    if (read(SOCKET,&text_size,2) != 2)
    {
        printf("Error, cant receive text size!\n");
        exit(2);
    }
   // printf(" text size: %d\n\n",text_size);

    char * text = malloc(text_size*sizeof(char));


    int resulta;
   // printf(" text size: %d\n\n",text_size);

    resulta = read(SOCKET,text,text_size);
    //printf(" text size: %d\n\n",text_size);

    if (resulta != text_size)
    {
        printf("Error, cant receive text!\n");
        printf("text size: %d  result: %d\n",text_size,resulta );
        exit(2);
    }

    //counts words by counting white spaces
    uint16_t result = 0;

    for (int i=0; i<text_size; i++)
    {
        if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n')
            result++;
    }

    result_t ret;
    ret.request_no = request_no;
    ret.counted_words = result;

    send_beginning(RESULT);

    if (write(SOCKET,&ret, sizeof(ret)) != sizeof(ret))
    {
        printf("Error, could not send return message :(\n");
        exit(10);
    }

    free(text);

}

void count_specific(void)
{
    uint16_t request_no;

    if (read(SOCKET,&request_no,2) != 2)
    {
        printf("Error, cant receive request no!\n");
        exit(2);
    }

    uint8_t word_size;

    if (read(SOCKET,&word_size,1) != 1)
    {
        printf("Error, cant receive word size!\n");
        exit(2);
    }


    char * word = malloc(word_size*sizeof(char));


    if (read(SOCKET,word,word_size) != word_size)
    {
        printf("Error, cant receive word!\n");
        exit(2);
    }

    uint16_t text_size;

    if (read(SOCKET,&text_size,2) != 2)
    {
        printf("Error, cant receive text size!\n");
        exit(2);
    }

    char * text = malloc(text_size*sizeof(char));

    int resulta;

    resulta = read(SOCKET,text,text_size);

    if (resulta != text_size)
    {
        printf("Error, cant receive text!\n");
        printf("text size: %d  result: %d\n",text_size,resulta );
        exit(2);
    }


    uint16_t result = 0;

    int i=0, j=0;

    while (i<text_size)
    {
        //printf("i : %d   j:  %d \n",i,j);
        if (text[i] == word[j])
            j++;
        else
            j = 0;

        if (j == word_size-1)
        {
            j=0;
            result++;
        }

        i ++;

    }

    result_t ret;
    ret.request_no = request_no;
    ret.counted_words = result;

    send_beginning(RESULT);

    if (write(SOCKET,&ret, sizeof(ret)) != sizeof(ret))
    {
        printf("Error, could not send return message :(\n");
        exit(10);
    }

    free(text);
    free(word);
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

        uint32_t ip = inet_addr(argv[3]);
        if (ip == -1)
        {
            printf("IP error\n");
            exit(9);
        }

        uint16_t port_n = (uint16_t) atoi(argv[4]);

        if (port_n < 1024 || port_n > 65535)
        {
            printf("Error in port\n");
            exit(10);
        }

        struct sockaddr_in web;
        memset(&web, 0, sizeof(struct sockaddr_in));
        web.sin_family = AF_INET;
        web.sin_addr.s_addr = ip;
        web.sin_port = htons(port_n);

        if ((SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Error duting creating web socket\n");
            exit(11);
        }

        if (connect(SOCKET, (const struct sockaddr *) &web, sizeof(web)) == -1)
        {
            printf("Error, could not connect to web socket\n");
            exit(12);
        }

    }
    else if (strcmp(argv[2],"LOCAL") == 0)
    {
        char * path = argv[3];

        if (strlen(path) < 1 || strlen(path) > UNIX_PATH_MAX)
        {
            printf("Error in unix path NAME!\n");
            exit(5);
        }

        struct sockaddr_un address;
        address.sun_family = AF_UNIX;
        snprintf(address.sun_path, UNIX_PATH_MAX,"%s",path);

        if ((SOCKET = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            printf("Error, could not create local socket!\n");
            exit(6);
        }

        if (connect(SOCKET, (const struct sockaddr *) &address, sizeof(address)) == -1) {
            printf("Error, could not connect to socket!\n");
            exit(7);
        }

    }
    else
    {
        printf("Error, second argument should be either WEB or LOCAL\n");
        exit(4);
    }

    // registering on server
    send_beginning(NEW_CLIENT);

    char message_received;

    if (read(SOCKET,&message_received,1) != 1)
    {
        printf("Error, cant receive message!\n");
        exit(2);
    }
    printf("\n");

    switch (message_received)
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
        if (read(SOCKET, &message_received, 1) != 1)
        {
            printf("Error during receiving message\n");
            exit(1);
        }
        switch (message_received)
        {
            case COUNT_ALL:
            {
                count_all();
                break;
            }
            case COUNT_SPECIFIC:
            {
                count_specific();
                break;
            }
            case PING:
            {
                send_beginning(PING_ANSWER);
                break;
            }
        }
    }

    return 0;
}






