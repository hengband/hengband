﻿#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class player_type;
MonsterSpellResult spell_RF4_SHRIEK(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_WORLD(player_type *player_ptr, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_BLINK(player_type *player_ptr, MONSTER_IDX m_idx, int TARGET_TYPE, bool is_quantum_effect);
MonsterSpellResult spell_RF6_TPORT(player_type *player_ptr, MONSTER_IDX m_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_TELE_TO(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_TELE_AWAY(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_TELE_LEVEL(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_DARKNESS(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_TRAPS(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx);
MonsterSpellResult spell_RF6_RAISE_DEAD(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
