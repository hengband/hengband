#pragma once

#include "system/angband.h"

void spell_RF5_CAUSE(player_type* target_ptr, int GF_TYPE, HIT_POINT dam, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, int MS_TYPE, int TARGET_TYPE);
HIT_POINT spell_RF5_CAUSE_1(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
HIT_POINT spell_RF5_CAUSE_2(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
HIT_POINT spell_RF5_CAUSE_3(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
HIT_POINT spell_RF5_CAUSE_4(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
