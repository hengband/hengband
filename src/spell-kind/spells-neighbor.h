#pragma once

#include "system/angband.h"

struct player_type;
bool door_creation(player_type *caster_ptr, POSITION y, POSITION x);
bool trap_creation(player_type *caster_ptr, POSITION y, POSITION x);
bool tree_creation(player_type *caster_ptr, POSITION y, POSITION x);
bool create_rune_protection_area(player_type *caster_ptr, POSITION y, POSITION x);
bool wall_stone(player_type *caster_ptr);
bool destroy_doors_touch(player_type *caster_ptr);
bool disarm_traps_touch(player_type *caster_ptr);
bool sleep_monsters_touch(player_type *caster_ptr);
bool animate_dead(player_type *caster_ptr, MONSTER_IDX who, POSITION y, POSITION x);
void wall_breaker(player_type *caster_ptr);
