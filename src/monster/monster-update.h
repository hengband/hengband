#pragma once

#include "system/angband.h"

class MonraceDefinition;
class MonsterEntity;
class PlayerType;
struct turn_flags;
bool update_riding_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx);
void update_map_flags(turn_flags *turn_flags_ptr);
void update_lite_flags(turn_flags *turn_flags_ptr, const MonraceDefinition &monrace);
void update_monster_race_flags(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster);
void update_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool full);
void update_monsters(PlayerType *player_ptr, bool full);
void update_smart_learn(PlayerType *player_ptr, MONSTER_IDX m_idx, int what);
