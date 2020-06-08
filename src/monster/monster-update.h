#pragma once

#include "system/angband.h"
#include "monster/monster-processor-util.h"
#include "system/monster-type-definition.h"

bool update_riding_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx);
void update_player_type(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr);
void update_monster_race_flags(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr);
void update_player_window(player_type *target_ptr, old_race_flags *old_race_flags_ptr);
