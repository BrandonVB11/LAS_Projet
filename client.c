#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include "messages.h"
#include "network.h"
#include "utils_v1.h"

#define TILE_PLACEMENT 1
#define BOARD_SIZE 20
#define MAX_MESSAGE_TEXT 100

typedef struct {
    int board[BOARD_SIZE];
    char pseudo[MAX_PSEUDO];
} PlayerBoard;

// Initialisation de la grille avec des valeurs par défaut
void initialize_board(PlayerBoard *player) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        player->board[i] = -1;
    }
}

// Afficher la grille pour le joueur
void display_board(const PlayerBoard *player) {
    printf("Grille du joueur %s:\n", player->pseudo);
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (player->board[i] == -1) {  // Arrow notation
            printf("[ ] "); // Case vide
        } else {
            printf("[%d] ", player->board[i]); // Case occupée
        }
    }
    printf("\n");
}



int count_score(const PlayerBoard* player) {
    int points = 0;
    int current_sequence_length = 0;

    // Parcours de la grille
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (player->board[i] != -1) { // Si la case n'est pas vide
            if (current_sequence_length == 0 || (player->board[i] == player->board[i - 1] + 1)) {
                current_sequence_length++; // Si la tuile est consécutive, on augmente la suite
            } else {
                // Fin d'une suite, calculer les points
                if (current_sequence_length >= 3) {
                    if (current_sequence_length == 3) {
                        points += 2; // 3 tuiles rapportent 2 points
                    } else if (current_sequence_length == 4) {
                        points += 5; // 4 tuiles rapportent 5 points
                    } else if (current_sequence_length > 4) {
                        points += 5 + (current_sequence_length - 3); // 5 tuiles et plus
                    }
                }

                // Réinitialiser la longueur de la suite
                current_sequence_length = 1; // Recommencer une nouvelle suite
            }
        } else {
            // Si on trouve une case vide, on calcule la suite précédente
            if (current_sequence_length >= 3) {
                if (current_sequence_length == 3) {
                    points += 2; 
                } else if (current_sequence_length == 4) {
                    points += 5; 
                } else if (current_sequence_length > 4) {
                    points += 5 + (current_sequence_length - 3);
                }
            }

            current_sequence_length = 0; // Réinitialiser pour suite suivante
        }
    }

    // Traiter la dernière suite si nécessaire
    if (current_sequence_length >= 3) {
        if (current_sequence_length == 3) {
            points += 2;
        } else if (current_sequence_length == 4) {
            points += 5;
        } else if (current_sequence_length > 4) {
            points += 5 + (current_sequence_length - 3);
        }
    }

    return points; // Retourner le score total
}

int main(int argc, char **argv) {
    char pseudo[MAX_PSEUDO];
    int sockfd;
    int ret;

    StructMessage msg;
    int tile; // Tuile envoyée par le serveur
    PlayerBoard player; // Plateau du joueur
    char position[4]; // Position saisie par le joueur

    /* retrieve player name */
    printf("Bienvenue dans la phase d'inscription au jeu\n");
    printf("Pour participer, entrez votre nom :\n");
    ret = sread(0, pseudo, MAX_PSEUDO);
    checkNeg(ret, "Erreur de lecture du nom du joueur");
    pseudo[ret - 1] = '\0';
    strcpy(player.pseudo, pseudo);

    initialize_board(&player);

    msg.code = INSCRIPTION_REQUEST;
    strcpy(msg.messageText, player.pseudo);

    sockfd = initSocketClient(SERVER_IP, SERVER_PORT);

    // Envoyer la demande d'inscription
    swrite(sockfd, &msg, sizeof(msg)); 
    // Lire la réponse du serveur
    sread(sockfd, &msg, sizeof(msg)); 

    if (msg.code == INSCRIPTION_OK) {
        printf("Réponse du serveur : Inscription acceptée\n");
    } else {
        printf("Réponse du serveur : Inscription refusée\n");
        sclose(sockfd);
        exit(0);
    }

    /* Attente du début du jeu */
    sread(sockfd, &msg, sizeof(msg)); // Lire le message du serveur

    if (msg.code == START_GAME) {
        printf("Début du jeu!\n");
        // Boucle de jeu
        while (true) {
            // Recevoir la tuile envoyée par le serveur
            ret = sread(sockfd, &tile, sizeof(int));
            if (ret <= 0) {
                printf("Erreur ou fin du jeu\n");
                break;
            }

            // Afficher la grille actuelle
            display_board(&player);

            // Demander au joueur de choisir une position
            printf("Veuillez entrer la position (0-%d) pour placer la tuile %d :\n", BOARD_SIZE - 1, tile);
            sread(0, position, sizeof(position));
            int pos = atoi(position);

            // Si la position est déjà occupée, on cherche la prochaine position libre vers la droite
            int initial_pos = pos;
            while (player.board[pos] != -1) {
                pos = (pos + 1) % BOARD_SIZE; // Aller vers la droite, et recommencer à zéro si on atteint la fin

                // Tour complet : PROBLEME
                if (pos == initial_pos) {
                    printf("Pas de positions libres\n");
                    sclose(sockfd);
                    exit(1);
                }
            }

            // Placer la tuile sur la grille du joueur
            player.board[pos] = tile;

            // Envoyer le placement au serveur
            msg.code = TILE_PLACEMENT; // Code pour placement de tuile
            // Safely copy into msg.messageText
            int written = snprintf(msg.messageText, MAX_MESSAGE_TEXT, 
                                   "Joueur %s a placé la tuile %d en position %d", 
                                   player.pseudo, tile, pos);

            // Check for truncation
            if (written >= MAX_MESSAGE_TEXT || written < 0) {
                printf("Warning: Message text may be truncated or an error occurred\n");
            }
            swrite(sockfd, &msg, sizeof(msg)); // Envoyer le message au serveur
        }
    }
    if (msg.code == END_OF_GAME) {
        printf("Jeu terminé\n");
        int score = count_score(&player);
        //TODO : envoyer score au serveur
        printf("Score final du joueur %s: %d points\n", player.pseudo, score);
        } else {
        printf("Jeu annulé\n");
        sclose(sockfd);
    }

    sclose(sockfd); // Fermer le socket
    return 0;
}
