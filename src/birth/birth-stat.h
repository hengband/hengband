#pragma once

#include "system/angband.h"

class PlayerType;
int adjust_stat(int value, int amount);
void get_stats(PlayerType *player_ptr);
uint16_t get_expfact(PlayerType *player_ptr);
void get_extra(PlayerType *player_ptr, bool roll_hitdie);

void get_max_stats(PlayerType *player_ptr);
