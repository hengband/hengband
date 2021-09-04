#pragma once

#include "io/files-util.h"
#include "system/angband.h"

struct player_type;
bool send_world_score(player_type *current_player_ptr, bool do_send, display_player_pf display_player);
errr top_twenty(player_type *current_player_ptr);
errr predict_score(player_type *current_player_ptr);
void race_legends(player_type *current_player_ptr);
void race_score(player_type *current_player_ptr, int race_num);
void show_highclass(player_type *current_player_ptr);
bool check_score(player_type *current_player_ptr);
