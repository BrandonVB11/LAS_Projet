#ifndef _IPC_H_
#define _IPC_H_

#include "messages.h"

void create_share_memory();

void register_player_score(char *pseudo, int score);

Message* read_player_scores(int nbPlayers);

#endif
