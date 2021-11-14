#pragma once

#include "system/angband.h"

class PlayerType;
bool door_creation(PlayerType *player_ptr, POSITION y, POSITION x);
bool trap_creation(PlayerType *player_ptr, POSITION y, POSITION x);
bool tree_creation(PlayerType *player_ptr, POSITION y, POSITION x);
bool create_rune_protection_area(PlayerType *player_ptr, POSITION y, POSITION x);
bool wall_stone(PlayerType *player_ptr);
bool destroy_doors_touch(PlayerType *player_ptr);
bool disarm_traps_touch(PlayerType *player_ptr);
bool sleep_monsters_touch(PlayerType *player_ptr);
bool animate_dead(PlayerType *player_ptr, MONSTER_IDX who, POSITION y, POSITION x);
void wall_breaker(PlayerType *player_ptr);
