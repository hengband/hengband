﻿#pragma once

#include "io/files-util.h"
#include "system/angband.h"

class PlayerType;
bool send_world_score(PlayerType *current_player_ptr, bool do_send, display_player_pf display_player);
errr top_twenty(PlayerType *current_player_ptr);
errr predict_score(PlayerType *current_player_ptr);
void race_legends(PlayerType *current_player_ptr);
void race_score(PlayerType *current_player_ptr, int race_num);
void show_highclass(PlayerType *current_player_ptr);
bool check_score(PlayerType *current_player_ptr);
