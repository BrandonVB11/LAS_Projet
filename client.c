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
    int last_tile_value = -1;

    // Scoring table for sequences of varying lengths
    int scoring_table[22] = {0, 0, 1, 3, 5, 7, 9, 11, 15, 20, 25, 30, 35, 40, 50, 60, 70, 85, 100, 150, 300};

    // Loop through the player's board to calculate score
    for (int i = 0; i < BOARD_SIZE; i++) {
        int tile = player->board[i];

        if (tile != -1) { // If the tile is not empty
            // If it's a joker (31) or part of a sequence
            if (tile == 31 || (last_tile_value != -1 && tile == last_tile_value + 1)) {
                current_sequence_length++; // Continue the sequence
            } else {
                        points += scoring_table[current_sequence_length];
                current_sequence_length = 1; // Reset sequence length
            }
        } else {
            // If there's a break in the sequence
            points += scoring_table[current_sequence_length];
            current_sequence_length = 1; // Reset sequence length
        }

        last_tile_value = tile; // Update the last tile value
    }

    // Calculate points for the final sequence
    points += scoring_table[current_sequence_length]; // Add final sequence points
    

    return points; // Return the total score
}


int main(int argc, char **argv) {
    char pseudo[MAX_PSEUDO];
    int sockfd;
    int ret;
    int positions[BOARD_SIZE];
    int pos_count = 0;
    int pos; // Position lue depuis le fichier
    StructMessage msg;
    int tile; // Tuile envoyée par le serveur
    PlayerBoard player; // Plateau du joueur

    if (argc > 1) { // Vérifier si un fichier a été fourni
        // Ouvrir le fichier pour obtenir le nom du joueur et les positions prédéfinies
        FILE* file = fopen(argv[1], "r");
        if (!file) {
            perror("Erreur lors de l'ouverture du fichier");
            return -1; // Sortir avec un code d'erreur
        }

        // Lire la première ligne pour obtenir le pseudo du joueur
        if (fgets(pseudo, MAX_PSEUDO, file)) {
            size_t len = strlen(pseudo); // Longueur de la ligne lue
            if (pseudo[len - 1] == '\n') { // Si la ligne se termine par un saut de ligne
                pseudo[len - 1] = '\0'; // Supprimer le saut de ligne
            }
        // Parcourir le fichier pour obtenir les positions
        while (pos_count < BOARD_SIZE && fscanf(file, "%d", &pos) == 1) {
            positions[pos_count] = pos; // Stocker la position
            pos_count++;
        }
        }
    } else {
    /* retrieve player name */
    printf("Bienvenue dans la phase d'inscription au jeu\n");
    printf("Pour participer, entrez votre nom :\n");
    ret = sread(0, pseudo, MAX_PSEUDO);
    checkNeg(ret, "Erreur de lecture du nom du joueur");
    pseudo[ret - 1] = '\0';
    }

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

        //TEST
        int p_idx = 0; // Index des positions prédéfinies
        // Boucle de jeu
        int i=0;
        while (i<BOARD_SIZE) {
            // Recevoir la tuile envoyée par le serveur
            ret = sread(sockfd, &tile, sizeof(int));
            printf("Random tile: %d received from %d\n", tile, sockfd);  

            if (ret <= 0) {
                printf("Erreur ou fin du jeu\n");
                break;
            }

            // Afficher la grille actuelle
            display_board(&player);

           // Choisir la position à partir des positions prédéfinies
            if (argc > 1) {
                pos = positions[p_idx]; // Prendre la position prédéfinie
                p_idx++; // Passer à la prochaine position
            } else {
                // Si pas de position prédéfinie, demander au joueur
            printf("Veuillez entrer la position (0-%d) pour placer la tuile %d :\n", BOARD_SIZE - 1, tile);
                scanf("%d", &pos); // Lire la position entrée par le joueur
            }
            pos = pos-1;
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
            //int written = snprintf(msg.messageText, MAX_MESSAGE_TEXT, 
            //                       "Joueur %s a placé la tuile %d en position %d", 
            //                       player.pseudo, tile, pos);

            // Check for truncation
            //if (written >= MAX_MESSAGE_TEXT || written < 0) {
            //    printf("Warning: Message text may be truncated or an error occurred\n");
            //}
            printf("Tile placed on position: %d sent to %d\n", pos, sockfd);  
            swrite(sockfd, &msg, sizeof(msg)); // Envoyer le message au serveur
            i++;
        }
    }

    printf("DISPLAYING FINAL BOARD\n");
    display_board(&player);
    sread(sockfd, &msg, sizeof(msg));

    if (msg.code == END_OF_GAME) {
        printf("Jeu terminé\n");
        int score = count_score(&player);
        printf("Score final du joueur %s: %d points\n", player.pseudo, score);

        //envoie du message
        Message scoremsg;
        scoremsg.score = score;
        swrite(sockfd, &scoremsg, sizeof(Message));




    } else {
        printf("Jeu annulé\n");
        sclose(sockfd);
    }

    sclose(sockfd); // Fermer le socket
    return 0;
}
