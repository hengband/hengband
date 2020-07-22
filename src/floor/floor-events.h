#pragma once

#include "system/angband.h"

void day_break(player_type *subject_ptr);
void night_falls(player_type *subject_ptr);
MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr);
void update_dungeon_feeling(player_type *subject_ptr);
void glow_deep_lava_and_bldg(player_type *subject_ptr);
void forget_lite(floor_type *floor_ptr);
void update_lite(player_type *subject_ptr);
void forget_view(floor_type *floor_ptr);
void update_view(player_type *subject_ptr);
void update_mon_lite(player_type *subject_ptr);
void clear_mon_lite(floor_type *floor_ptr);
