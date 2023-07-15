#pragma once

class PlayerType;
bool get_aim_dir(PlayerType *player_ptr, int *dp);
bool get_direction(PlayerType *player_ptr, int *dp);
bool get_rep_dir(PlayerType *player_ptr, int *dp, bool under = false);
