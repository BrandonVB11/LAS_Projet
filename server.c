#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "messages.h"
#include "ipc_conf.h"
#include "utils_v1.h"

#define MIN_PLAYERS 2
#define MAX_PLAYERS 10
#define BACKLOG 5
#define TIME_INSCRIPTION 15 /*30 a metttre à la fin*/

typedef struct Player
{
	char pseudo[MAX_PSEUDO];
	int sockfd;
	int position_tuile;
    int score;
} Player;

Player tabPlayers[MAX_PLAYERS];
volatile sig_atomic_t end_inscriptions = 0;

void endServerHandler(int sig)
{
	end_inscriptions = 1;
}

void disconnect_players(Player *tabPlayers, int nbPlayers)
{
	for (int i = 0; i < nbPlayers; i++)
		sclose(tabPlayers[i].sockfd);
	return;
}

void relance_server(int sig)
{
	printf("Relance du serveur\n");
	disconnect_players(tabPlayers, MAX_PLAYERS);
	//delete shared memory
	sshmdelete(CLIENT_SERVEUR_SHM_KEY);
	//delete semaphore
	sem_delete(CLIENT_SERVEUR_SEM_KEY);
	end_inscriptions = 0;
}


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

/*int accept_client_connection(int server_socket){

}*/


int main(int argc, char const *argv[])
{
    int sockfd, newsockfd, i;
	StructMessage msg;


	printf("LANCEMENT DU SERVEUR\n");
	sockfd = setup_server_socket(SERVER_PORT);
	printf("Le serveur tourne sur le port : %i \n", SERVER_PORT);

	/*Setup de la memoire partagée*/
    int sem_id = sem_create(CLIENT_SERVEUR_SEM_KEY, 1, PERM, 1);

    int shm_id = sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PSEUDO, IPC_CREAT | PERM);
    char * shm = sshmat(shm_id);
    printf("MEMOIRE PARTAGE CREEE\n");

    //gestion des signaux
	ssigaction(SIGINT, endServerHandler);
    ssigaction(SIGALRM, endServerHandler);

  

	i = 0;
	int nbPLayers = 0;

	// INSCRIPTION PART
	alarm(TIME_INSCRIPTION);

	while (!end_inscriptions)
	{	
		/* client trt */
		newsockfd = accept(sockfd, NULL, NULL); 
		if (newsockfd > 0)						
		{

			sread(newsockfd, &msg, sizeof(msg));

			if (msg.code == INSCRIPTION_REQUEST)
			{
				printf("Inscription demandée par le joueur : %s\n", msg.messageText);

				strcpy(tabPlayers[i].pseudo, msg.messageText);
				tabPlayers[i].sockfd = newsockfd;
				i++;

				if (nbPLayers < MIN_PLAYERS)
				{
                    //down la memoire partagee
                    sem_down(sem_id, 0);
                    //enregistre le pseudo du joueur dans la memoire partagee
                    strcpy(shm, msg.messageText);
                    printf("Pseudo enregistré dans la mémoire partagée : %s\n", shm);
                    //up la memoire partagee
                    sem_up(sem_id, 0);

					msg.code = INSCRIPTION_OK;
					nbPLayers++;
					if (alarm(TIME_INSCRIPTION) == -1)
					{
							alarm(0);
					}
				}
				else
				{
					msg.code = INSCRIPTION_KO;
				}
				swrite(newsockfd, &msg, sizeof(msg));
				printf("Nb Inscriptions : %i\n", nbPLayers);
			}
		}
	}

if(nbPLayers < MIN_PLAYERS){
	printf("PAS ASSEZ DE JOUEURS ... JEU ANNULEE\n");
	msg.code = CANCEL_GAME;
	for (i = 0; i < nbPLayers; i++)
		swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
	disconnect_players(tabPlayers, nbPLayers);
	
}
else{
	printf("FIN DES INSCRIPTIONS\n");
	//mettre toutes nouvelles inscriptions en attente
	msg.code = INSCRIPTION_EN_ATTENTE;
	for (i = 0; i < nbPLayers; i++)
		swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
	/*int pipelChild[nbPLayers][2];
	for (int i = 0; i < nbPLayers; i++)
	{
		//pipe init
		spipe(pipelChild[i]);
		//fork and run
		fork_and_run1(pipelChild[i]);

		sclose(pipelChild[i][1]);

	}*/
		printf("PARTIE VA DEMARRER ... \n");
		msg.code = START_GAME;
		for (i = 0; i < nbPLayers; i++)
			swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
}
}
