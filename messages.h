#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#define SERVER_PORT 9502
#define SERVER_IP "127.0.0.1"
#define MAX_PSEUDO 32

#define INSCRIPTION_REQUEST 10
#define INSCRIPTION_OK 11
#define INSCRIPTION_KO 12
#define START_GAME 13
#define CANCEL_GAME 14
#define INSCRIPTION_EN_ATTENTE 15
#define END_OF_GAME 16

typedef struct
{
    char messageText[MAX_PSEUDO];
    int score;
} Message;



typedef struct
{
    char messageText[MAX_PSEUDO];
    int code;
} StructMessage;
#endif