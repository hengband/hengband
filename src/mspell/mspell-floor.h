#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
MonsterSpellResult spell_RF4_SHRIEK(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_WORLD(PlayerType *player_ptr, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_BLINK(PlayerType *player_ptr, MONSTER_IDX m_idx, int target_type, bool is_quantum_effect);
MonsterSpellResult spell_RF6_TPORT(PlayerType *player_ptr, MONSTER_IDX m_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_TO(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_AWAY(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TELE_LEVEL(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_DARKNESS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_TRAPS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_RAISE_DEAD(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
