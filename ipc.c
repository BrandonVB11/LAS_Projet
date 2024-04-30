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

void register_player_score(char *pseudo, int score){
    
    int sem_id = sem_get(CLIENT_SERVEUR_SEM_KEY, PERM);
    int shm_id = sshmget(CLIENT_SERVEUR_SHM_KEY, MAX_PSEUDO*sizeof(char), PERM);
    char* shm = sshmat(shm_id);
    //down la memoire partagee
    sem_down(sem_id, 0);
    //enregistre le pseudo du joueur et le score dans la memoire partagee
    strcpy(shm, pseudo);
    shm += MAX_PSEUDO;
    sprintf(shm, "%d", score);
    //up la memoire partagee
    sem_up(sem_id, 0);
    printf("PSEUDO ET SCORE ENREGISTRE\n");

}
