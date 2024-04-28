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
#include <stdlib.h>
#include <poll.h> // Include this for `struct pollfd`

#include "messages.h"
#include "network.h"
#include "ipc.h"
#include "utils_v1.h"

#define MIN_PLAYERS 2
#define MAX_PLAYERS 10
#define TIME_INSCRIPTION 15 /*30 a metttre à la fin*/
#define NOMBRE_TOUR 20

typedef struct Player
{
	char pseudo[MAX_PSEUDO];
	int sockfd;
	int position_tuile;
    int score;
} Player;

Player tabPlayers[MAX_PLAYERS];
volatile sig_atomic_t end_inscriptions = 0;
volatile sig_atomic_t list_size_var = 40;
volatile sig_atomic_t end = 0;

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

//TEST

int* createTilesList(){
	int* tiles_list = (int*)malloc(list_size_var * sizeof(int));
	for(int i=0; i<list_size_var; i++){
		if(i<31){
			tiles_list[i] = i+1;  //de 1-31
		} else {
			tiles_list[i] = i-20; // de 11-19
		}
	}

	return tiles_list;
}



//draws the tile that got randomly picked
/*int draw_tile(int tile){

}*/

//calcul du nouveau tableau
int* createNewTilesList(int* oldList, int random_index){
	int new_size = list_size_var-1;
	int* newTiles = (int*)malloc(new_size* sizeof(int));
	int j=0;
	for (int i = 0; i<list_size_var-1; i++){
		if(i == random_index){
			j++;
		}
		newTiles[i] = oldList[j]; 
		j++;
	}
	list_size_var --;
	printf("list_size_var: %d\n", list_size_var);
	return newTiles;
}



//TEST

/*int accept_client_connection(int server_socket){
t
}*/

void serveur_fils(void *sockfd, void *pipe1, void *pipe2){
	//creation pipes
    StructMessage msg;
	int* pipefd1 = (int*)pipe1;
	int* pipefd2 = (int*)pipe2;

    printf("Pipe for : %d, %d\n", pipefd1[0], pipefd1[1]);

    int num_tuile;
    printf("Reading from pipefd1[0]: %d\n", pipefd1[0]);
    sread(pipefd1[0], &num_tuile, sizeof(int));
    printf("Random number sent to %d: %d\n", (*(int*) sockfd), num_tuile);	
    // envoie de la tuile via socket au clientfpipedfds
    swrite((*(int*) sockfd), &num_tuile, sizeof(int));
    sread((*(int*) sockfd), &msg, sizeof(int));
    printf("Code received: %d \n", msg.code);
    // fermer les descripteurs de fichiers restants  
    sclose(pipefd1[0]); // Close read end of the first pipe
    sclose(pipefd1[1]); // Close write end of the first pipe

    sclose(pipefd2[0]); // Close read end of the second pipe
    sclose(pipefd2[1]); // Close write end of the second pipe


    printf("Closed all pipes\n");
}

void exitHandler(int sig)
{
    end = 1;
}


int main(int argc, char const *argv[])
{
    int sockfd, newsockfd, i;
	StructMessage msg;
	
    create_share_memory();

    ssigaction(SIGALRM, endServerHandler);

    printf("LANCEMENT DU SERVEUR\n");
	sockfd = setup_server_socket(SERVER_PORT);
	printf("Le serveur tourne sur le port : %i \n", SERVER_PORT);

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
	exit(0);
}
else{
	int* tiles_list = createTilesList();

    // Display the generated tiles list
    printf("Generated tiles list:\n");
    for (int i = 0; i < list_size_var; i++) {
        printf("%d ", tiles_list[i]);
    }

    printf("\n");

	printf("FIN DES INSCRIPTIONS\n");
	printf("PARTIE VA DEMARRER ... \n\n");
	msg.code = START_GAME;

    int **pipeChild = (int **)malloc(nbPLayers * sizeof(int *));
    char buffer[sizeof(int)];
    struct pollfd* fds = (struct pollfd*)malloc(nbPLayers * sizeof(struct pollfd));

    ssigaction(SIGINT, exitHandler);
    ssigaction(SIGTERM, exitHandler);
	//creation des pipes
        for (int i = 0; i < nbPLayers; i++)
    {
        swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
        pipeChild[i] = (int *)malloc(2 * sizeof(int));

        printf("Initializing pipes for player %d\n", i);
        spipe(pipeChild[i]);
        printf("pipeChild[%d][0]: %d, pipeChild[%d][1]: %d\n\n", i, pipeChild[i][0], i, pipeChild[i][1]);
        fork_and_run3(serveur_fils, (void *)&tabPlayers[i].sockfd, (void *)&pipeChild[i][0], (void *)&pipeChild[i][1]);
        // init poll
        fds[i].fd = pipeChild[i][0];
        fds[i].events = POLLIN;
    }

	int i=0;
    while(i<NOMBRE_TOUR){
    	int random_index = randomIntBetween(0, list_size_var-1);
	    int random_tuile = tiles_list[random_index];
	    printf("randomNbr %d: %d\n", i, random_tuile);
	    createNewTilesList(tiles_list, random_index);
    	i++;

        // Ecriture des tuiles chez les enfants
        for (int i = 0; i < nbPLayers; i++)
        {
            printf("Attempting to write to pipeChild[%d][1]: %d\n", i, pipeChild[i][1]);
            if (pipeChild[i][1] < 0) {
                printf("Invalid file descriptor for pipeChild[%d][1]\n", i);
            }

            printf("Random tile to write: %d\n", random_tuile);
            swrite(pipeChild[i][1], &random_tuile, sizeof(int));
            printf("Written tile : %d\n", random_tuile);
        }
        // Polling
        while (end == 0) {
        spoll(fds, nbPLayers, 0); // pas de souci si le pipe est fermé côté écrivain --> pas d'événement POLLIN
        int chosen_tile;

            if (fds[i].revents & POLLIN) {
                chosen_tile = sread(pipeChild[i][0], buffer, sizeof(int));

                swrite(1, &chosen_tile, sizeof(int));
            }
        }
    }

    for (int i = 0; i < nbPLayers; i++) {
        sclose(pipeChild[i][0]);
        free(pipeChild[i]);
    }

    free(pipeChild);


    }
    
}
