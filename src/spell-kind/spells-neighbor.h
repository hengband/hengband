﻿#pragma once

#include "system/angband.h"

class player_type;
bool door_creation(player_type *player_ptr, POSITION y, POSITION x);
bool trap_creation(player_type *player_ptr, POSITION y, POSITION x);
bool tree_creation(player_type *player_ptr, POSITION y, POSITION x);
bool create_rune_protection_area(player_type *player_ptr, POSITION y, POSITION x);
bool wall_stone(player_type *player_ptr);
bool destroy_doors_touch(player_type *player_ptr);
bool disarm_traps_touch(player_type *player_ptr);
bool sleep_monsters_touch(player_type *player_ptr);
bool animate_dead(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x);
void wall_breaker(player_type *player_ptr);
