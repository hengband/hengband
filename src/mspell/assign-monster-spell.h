﻿#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

enum class RF_ABILITY;

class PlayerType;
MonsterSpellResult monspell_to_player(PlayerType *player_ptr, RF_ABILITY ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx);
MonsterSpellResult monspell_to_monster(
    PlayerType *player_ptr, RF_ABILITY ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell);
