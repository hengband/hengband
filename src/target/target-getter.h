#pragma once

#include <optional>

class PlayerType;
bool get_aim_dir(PlayerType *player_ptr, int *dp);
std::optional<int> get_direction(PlayerType *player_ptr);
bool get_rep_dir(PlayerType *player_ptr, int *dp, bool under = false);
