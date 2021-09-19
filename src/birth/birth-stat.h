#pragma once

#include "system/angband.h"

struct player_type;
int adjust_stat(int value, int amount);
void get_stats(player_type* player_ptr);
uint16_t get_expfact(player_type *player_ptr);
void get_extra(player_type *player_ptr, bool roll_hitdie);

void get_max_stats(player_type* player_ptr);
