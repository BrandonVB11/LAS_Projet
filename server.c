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


#define TIME_INSCRIPTION 15 /*30 a metttre à la fin*/
#define NOMBRE_TOUR 20


// Groupe 24
// Laurent Vandermeersch
// Brandon Van Bellinghen
// Lars Hanquet

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
    list_size_var --;
    int* newTiles = (int*)malloc(list_size_var* sizeof(int));
    int j=0;
    for (int i = 0; i<list_size_var; i++){
        if(i == random_index){
            j++;
        }
        newTiles[i] = oldList[j]; 
        j++;
    }
    
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
    Message scoremsg;
    int num_tuile;
    int nbPl;
    int* socketfd = sockfd;
    int* pipeR = (int*)pipe1;
    int* pipeW = (int*)pipe2;
    sclose(pipeW[1]);
    sclose(pipeR[0]);


    printf("Pipe for : %d, %d\n", pipeR[1], pipeW[0]);    
    
    int i = 0;
    while (i<20) {
        // Read random tile from pipe1
        sread(pipeW[0], &num_tuile, sizeof(int));
        printf("Random number sent to %d: %d\n", (*(int*) sockfd), num_tuile);
        
        // Send the tile to the client
        swrite((*(int*) socketfd), &num_tuile, sizeof(int));

        // Read client response
        printf("Setting up Answer from pipe\n");
        sread((*(int*) socketfd), &msg, sizeof(StructMessage));
        printf("\nCode received: %d \n\n", msg.code);
        
        // Write the response to pipe2
        swrite(pipeR[1], &msg, sizeof(StructMessage));
        i++;
    }
    sread((*(int*)socketfd), &scoremsg, sizeof(scoremsg));
    printf("On a lu la reponse du client via le socket\n");
    swrite(pipeR[1], &scoremsg, sizeof(scoremsg));
    printf("On a ecrit la reponse du client sur le pipe\n");

    //read pour nbr de jouers
    int nbPlayers = sread(pipeW[0], &nbPl, sizeof(int));


    //TODO print the read player scores and add the number of players in param
    Message* player_scores = read_player_scores(nbPlayers);

    // Print the player scores 
    
    printf("Player Scores:\n");
    for (int i = 0; i < nbPlayers; i++) {
        printf("Player %d: %s - Score: %d\n", i + 1, player_scores[i].messageText, player_scores[i].score);
    }
    

    // Free allocated memory
    free(player_scores);

    printf("Closed all pipes\n\n");
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
    //delete_shared_memory();     //ICI pour test

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


    /* -------------------------
      NBR PLAYERS OK WE START
    ------------------------- */
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

        //pipe list
        int pipeWrite[MAX_PLAYERS];
        int pipeRead[MAX_PLAYERS];
        struct pollfd fds[MAX_PLAYERS];

        ssigaction(SIGINT, exitHandler);
        ssigaction(SIGTERM, exitHandler);

        //creation des pipes
        for (int i = 0; i < nbPLayers; i++)
        {
            swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));
            
            int pipeR[2];
            int pipeW[2];
            spipe(pipeR);
            spipe(pipeW);

            pipeRead[i] = pipeR[0];
            pipeWrite[i] = pipeW[1];

            // Initialize poll structure
            fds[i].fd = pipeRead[i];
            fds[i].events = POLLIN;

            // Fork and run child process
            printf("fork and run: %d\n", i);
            fork_and_run3(serveur_fils, (void *)&tabPlayers[i].sockfd, pipeR, pipeW);
            
        }

        int game_round = 0;
        while (game_round < NOMBRE_TOUR) {
            printf("gameround: %d\n", game_round);
            // Send random tile to each client
            int random_index = randomIntBetween(0, list_size_var - 1);
            int random_tile = tiles_list[random_index];
            tiles_list = createNewTilesList(tiles_list, random_index);
            for (int j = 0; j < nbPLayers; j++) {                
                swrite(pipeWrite[j], &random_tile, sizeof(int));
            }

            // Wait for responses from all clients
            int responses_received = 0;
            while (responses_received < nbPLayers) {
                int ret = poll(fds, nbPLayers, -1); // Wait indefinitely for events
                if (ret > 0) {
                    for (int j = 0; j < nbPLayers; j++) {
                        if (fds[j].revents & POLLIN) {
                            StructMessage client_response;
                            sread(pipeRead[j], &client_response, sizeof(StructMessage));
                            printf("Received response from client %d\n", j);
                            responses_received++;
                        }
                    }
                } else if (ret < 0) {
                    perror("poll");
                    exit(EXIT_FAILURE);
                }
            }

            game_round++;
            responses_received = 0;
        }

        end=1;

        //TODO send end of game
        msg.code = END_OF_GAME;
        for (i = 0; i < nbPLayers; i++)
            swrite(tabPlayers[i].sockfd, &msg, sizeof(msg));

        printf("FINI DECRIRE \n");

        Message scoremsg;
        for (int j = 0; j < nbPLayers; j++) {                
            sread(pipeRead[j], &scoremsg, sizeof(Message));
            printf("Score read: %d\n", scoremsg.score);

            //write nbr players to all sons
            int nbPl = nbPLayers;
            swrite(pipeWrite[j], &nbPl, sizeof(int));

            //TODO ecrire dans la mem partagee
            register_player_score(tabPlayers[j].pseudo, scoremsg.score);
        }

        // Close pipes and free memory
        for (int i = 0; i < nbPLayers; i++) {
            sclose(pipeWrite[i]);
            sclose(pipeRead[i]);
            //sclose(pipeW[0]);
            //sclose(pipeR[1]);
        }

        //delete_shared_memory();
    }

    return 0;
}
