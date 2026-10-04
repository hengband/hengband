#pragma once

#include <utility>

class PlayerType;
std::pair<int, int> calc_displayed_melee_bonus(PlayerType *player_ptr, int hand);
std::pair<int, int> calc_displayed_bow_bonus(PlayerType *player_ptr);
std::pair<int, int> calc_displayed_speed(PlayerType *player_ptr);
void display_player_middle(PlayerType *player_ptr);
