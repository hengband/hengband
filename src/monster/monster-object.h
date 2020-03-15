/*!
 * @brief monster-processのための構造体群
 * @date 2020/03/07
 * @author Hourier
 */

#pragma once

#include "angband.h"
#include "monster/monster-util.h"

void update_object_by_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
