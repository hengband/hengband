#pragma once

#include "angband.h"
#include "monster/monster-util.h"

bool process_wall(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx, bool can_cross);
bool process_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx);

bool process_protection_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx);
bool process_explosive_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx);
