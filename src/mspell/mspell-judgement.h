#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

struct monster_type;
struct player_type;
bool direct_beam(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, monster_type *m_ptr);
bool breath_direct(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, AttributeType typ, bool is_friend);
void get_project_point(player_type *player_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
bool dispel_check_monster(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool dispel_check(player_type *player_ptr, MONSTER_IDX m_idx);
