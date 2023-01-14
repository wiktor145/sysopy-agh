//
// Created by wicia on 01.06.19.
//

#ifndef SYSOPY_COMMON_H
#define SYSOPY_COMMON_H


#define UNIX_PATH_MAX 108

#define MAX_CLIENTS 20

#define MAX_NAME_LENGTH 10

#define MAX_WORD_TO_SEARCH_LENGTH 10

#define MAX_BUFFER 1024

// message types
#define NEW_CLIENT 'a'
#define DELETE_ME 'b'
#define SUCCESS 'c'
#define FAIL_NAME_EXIST 'd'
#define FAIL_TO_MUCH_CLIENTS 'e'
#define COUNT_ALL 'f'
#define COUNT_SPECIFIC 'g'
#define RESULT 'h'
#define PING 'i'
#define PING_ANSWER 'j'

typedef struct Client
{
    struct sockaddr* socket_addr;
    socklen_t socket_len;
    char*  name;
    char connection_type;
    short active;
    short working;
} Client;


typedef struct result_t
{
    char message_type;
    char client_name[11];
    char connection_type;
    uint16_t request_no;
    uint16_t counted_words;
} result_t;

typedef struct Request
{
    char message_type;
    uint16_t request_no;

    int word_size;
    char word[11];

    int text_size;
    char text[MAX_BUFFER];

} Request ;




#endif //SYSOPY_COMMON_H
