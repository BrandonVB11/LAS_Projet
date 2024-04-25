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
	//int sockfd = (int)sockfd1
	int* pipefd1 = (int*)pipe1;
	int* pipefd2 = (int*)pipe2;
    spipe(pipefd1);
    spipe(pipefd2);

    close(pipefd1[1]); // fermer l'extrémité d'écriture du premier pipe
    close(pipefd2[0]); // fermer l'extrémité de lecture du deuxième pipe

    // recevoir num de tuile via le pipe du processus père de server
    int num_tuile;
    printf("ICI 1: %d\n", pipefd1[0]);	
    sread(pipefd1[0], &num_tuile, sizeof(int));
    printf("LA BAS\n");	
    printf("Random number sent to client: %d\n", num_tuile);	
    // envoie de la tuile via socket au clientfpipedfds
    //swrite(*((int *)sockfd), &num_tuile, sizeof(int));

    // fermer les descripteurs de fichiers restants  
    close(pipefd1[0]);  
    close(pipefd2[1]);

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
		printf("PARTIE VA DEMARRER ... \n");
		msg.code = START_GAME;

	//creation des pipes
	int **pipefds = (int **)malloc(nbPLayers * sizeof(int *));
	for (int i = 0; i < nbPLayers; i++) {
	    pipefds[i] = (int *)malloc(2 * sizeof(int));
	    spipe(&pipefds[i][0]); // Pass the address of pipefds[i][0]
	    spipe(&pipefds[i][1]); // Pass the address of pipefds[i][1]
	    fork_and_run3(serveur_fils, (void *)&tabPlayers[i].sockfd, (void *)&pipefds[i][0], (void *)&pipefds[i][1]);
	}


	int i=0;
    while(i<NOMBRE_TOUR){
    	int random_index = randomIntBetween(0, list_size_var-1);
	    int randomNbr = tiles_list[random_index];
	    printf("randomNbr %d: %d\n", i, randomNbr);
	    createNewTilesList(tiles_list, random_index);
    	i++;
	    printf("\n");

		for (int j = 0; j < nbPLayers; j++) {
		    // Écrire dans l'extrémité d'écriture du tube
		    printf("ICI: %d\n", pipefds[j][0]);	
		    //TODO FIX LE WRITE
		    nwrite(pipefds[j][0], &randomNbr, sizeof(int));
		}


	    //attendre que tous les fils aient eu une reponse
	    for (int j = 0; j < nbPLayers; j++) {
        	// lire dans l'extrémité d'écriture du tube
        	sread(pipefds[j][1], &randomNbr, sizeof(int));
    	}

    }
    
    // Free the allocated memory for the list
    free(tiles_list);
	for (int i = 0; i < nbPLayers; i++) {
		close(pipefds[i][0]);
		close(pipefds[i][1]);
		free(pipefds[i]);
	}
	free(pipefds);

}
}
