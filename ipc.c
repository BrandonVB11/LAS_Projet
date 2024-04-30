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
#include "ipc.h"
#include "messages.h"

#define CLIENT_SERVEUR_SEM_KEY  5
#define CLIENT_SERVEUR_SHM_KEY  6

#define PERM 0666


Message msg;



void create_share_memory(){
    sem_create(CLIENT_SERVEUR_SEM_KEY, 1, PERM, 1);
    sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PSEUDO*sizeof(char), IPC_CREAT | PERM);
    printf("MEMOIRE PARTAGE CREEE\n");

}

void delete_shared_memory(){
    int sem_id = sem_get(CLIENT_SERVEUR_SEM_KEY, 1);
    int shm_id = sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PLAYERS * sizeof(char), PERM);

    sem_delete(sem_id);
    sshmdelete(shm_id);
    printf("MEMOIRE PARTAGE DETRUITE\n");
}

void register_player_score(char *pseudo, int score){
    
    int sem_id = sem_get(CLIENT_SERVEUR_SEM_KEY, 1);
    int shm_id = sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PLAYERS * sizeof(char), PERM);
    Message* shm = sshmat(shm_id);
    //down la memoire partagee
    sem_down(sem_id, 0);
    
    int i = 0;
    while (i < MAX_PLAYERS && strlen(shm[i].messageText) != 0) {
        i++;
    }  
    if (i < MAX_PLAYERS) {
        strcpy(shm[i].messageText, pseudo);
        shm[i].score = score;
    }

    sem_up(sem_id, 0);
    sshmdt(shm);
    printf("PSEUDO  ET SCORE ENREGISTRE\n");

}


Message* read_player_scores(int nbPlayers) {
    int sem_id = sem_get(CLIENT_SERVEUR_SEM_KEY, 1);
    int shm_id = sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PLAYERS * sizeof(char), PERM);
    Message* shm = (Message*)sshmat(shm_id);

    sem_down(sem_id, 0); // Lock the shared memory

    // Count the number of players
    int num_players = nbPlayers;

    // Sort player scores by score using bubble sort (descending order)
    for (int i = 0; i < num_players - 1; i++) {
        for (int j = 0; j < num_players - i - 1; j++) {
            if (shm[j].score < shm[j + 1].score) {
                // Swap player scores
                Message temp = shm[j];
                shm[j] = shm[j + 1];
                shm[j + 1] = temp;
            }
        }
    }

    // Allocate memory for player scores
    Message* player_scores = (Message*)malloc(num_players * sizeof(Message));
    memcpy(player_scores, shm, num_players * sizeof(Message));

    sem_up(sem_id, 0); // Release the lock
    sshmdt(shm); // Detach the shared memory

    return player_scores;
}

