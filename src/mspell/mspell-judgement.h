#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class MonsterEntity;
class PlayerType;
bool direct_beam(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, MonsterEntity *m_ptr);
bool breath_direct(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, AttributeType typ, bool is_friend);
void get_project_point(PlayerType *player_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg);
bool dispel_check_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool dispel_check(PlayerType *player_ptr, MONSTER_IDX m_idx);
