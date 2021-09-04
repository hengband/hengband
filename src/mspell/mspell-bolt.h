#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

struct player_type;
MonsterSpellResult spell_RF4_SHOOT(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_ACID(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_ELEC(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_FIRE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_COLD(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_NETH(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_WATE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_MANA(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_PLAS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BO_ICEE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_MISSILE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
