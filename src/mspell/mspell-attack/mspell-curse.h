#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
MonsterSpellResult spell_RF5_CAUSE_1(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_CAUSE_2(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_CAUSE_3(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_CAUSE_4(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
