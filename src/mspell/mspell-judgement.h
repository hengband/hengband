#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include "util/point-2d.h"

class MonsterEntity;
class PlayerType;
bool direct_beam(PlayerType *player_ptr, const MonsterEntity &monster, const Pos2D &pos_target);
bool breath_direct(PlayerType *player_ptr, const Pos2D &pos_source, const Pos2D &pos_target, int rad, AttributeType typ, bool is_friend);
Pos2D get_project_point(PlayerType *player_ptr, const Pos2D &pos_source, const Pos2D &pos_target_initial, BIT_FLAGS flags);
bool dispel_check_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool dispel_check(PlayerType *player_ptr, MONSTER_IDX m_idx);
