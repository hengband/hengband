#pragma once

#include "monster/monster-processor-util.h"
#include "system/angband.h"
#include "system/monster-type-definition.h"

bool process_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, DIRECTION *mm, POSITION oy, POSITION ox, int *count);
void process_speak_sound(player_type *target_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware);
void set_target(monster_type *m_ptr, POSITION y, POSITION x);
void reset_target(monster_type *m_ptr);
