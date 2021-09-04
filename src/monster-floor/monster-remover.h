#pragma once

#include "system/angband.h"

struct player_type;
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i);
void wipe_monsters_list(player_type *player_ptr);
void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
