#pragma once

#include "system/angband.h"

struct monster_type;
struct player_type;
bool direct_beam(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, monster_type *m_ptr);
bool breath_direct(player_type *master_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, EFFECT_ID typ, bool is_friend);
void get_project_point(player_type *target_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
bool dispel_check_monster(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool dispel_check(player_type *creature_ptr, MONSTER_IDX m_idx);
