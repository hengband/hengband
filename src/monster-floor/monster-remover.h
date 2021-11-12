#pragma once

#include "system/angband.h"

class PlayerType;
void delete_monster_idx(PlayerType *player_ptr, MONSTER_IDX i);
void wipe_monsters_list(PlayerType *player_ptr);
void delete_monster(PlayerType *player_ptr, POSITION y, POSITION x);
