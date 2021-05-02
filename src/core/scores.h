#pragma once

#include "io/files-util.h"
#include "system/angband.h"

typedef struct player_type player_type;
struct high_score;
void display_scores_aux(int from, int to, int note, high_score *score);
void display_scores(int from, int to);
bool send_world_score(player_type *current_player_ptr, bool do_send, display_player_pf display_player);
errr top_twenty(player_type *current_player_ptr);
errr predict_score(player_type *current_player_ptr);
void race_legends(player_type *current_player_ptr);
void race_score(player_type *current_player_ptr, int race_num);
void show_highclass(player_type *current_player_ptr);
bool check_score(player_type *current_player_ptr);
