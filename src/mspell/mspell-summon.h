#pragma once

#include "system/angband.h"

MONSTER_NUMBER summon_Kin(player_type* target_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx);
void spell_RF6_S_KIN(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_CYBER(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_MONSTER(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_MONSTERS(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_ANT(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_SPIDER(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_HOUND(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_HYDRA(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_ANGEL(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_DEMON(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_UNDEAD(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_DRAGON(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_HI_UNDEAD(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_HI_DRAGON(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_AMBERITES(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
void spell_RF6_S_UNIQUE(player_type* target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
