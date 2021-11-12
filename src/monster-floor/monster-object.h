#pragma once

#include "system/angband.h"

struct monster_type;
class PlayerType;
struct turn_flags;
void update_object_by_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
void monster_drop_carried_objects(PlayerType *player_ptr, monster_type *m_ptr);
