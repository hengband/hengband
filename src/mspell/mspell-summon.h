#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
MonsterSpellResult spell_RF6_S_KIN(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_CYBER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_MONSTER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_MONSTERS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_ANT(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_SPIDER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_HOUND(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_HYDRA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_ANGEL(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_DEMON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_UNDEAD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_DRAGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_HI_UNDEAD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_HI_DRAGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_AMBERITES(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_S_UNIQUE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
