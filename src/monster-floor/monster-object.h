#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
typedef struct player_type player_type;
typedef struct turn_flags turn_flags;
void update_object_by_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr);
