#ifndef _NETWORK_H_
#define _NETWORK_H_

/**
 * PRE:  serverPort: a valid port number
 * POST: on success, binds a socket to 0.0.0.0:serverPort and listens to it ;
 *       on failure, displays error cause and quits the program
 * RES:  return socket file descriptor
 */
int setup_server_socket(int port);

int initSocketClient(char *serverIP, int serverPort);

#endif