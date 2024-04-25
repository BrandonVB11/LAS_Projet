#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include "utils_v1.h"
#include "network.h"

#define BACKLOG 5


/**
 * PRE:  serverPort: a valid port number
 * POST: on success, binds a socket to 0.0.0.0:serverPort and listens to it ;
 *       on failure, displays error cause and quits the program
 * RES:  return socket file descriptor
 */
int setup_server_socket(int port){
    int sockfd = ssocket();

	int option = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

	sbind(port, sockfd);

	slisten(sockfd, BACKLOG);

    return sockfd;
}

int initSocketClient(char *serverIP, int serverPort) {
    int sockfd = ssocket();
    sconnect(serverIP, serverPort, sockfd);
    return sockfd;
}