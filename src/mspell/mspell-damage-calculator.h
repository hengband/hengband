#pragma once

#include "mspell/mspell-type.h"
#include "system/angband.h"

enum spell_flag_type {
	DAM_ROLL = 1,
    DAM_MAX = 2,
    DAM_MIN = 3,
    DICE_NUM = 4,
    DICE_SIDE = 5,
    DICE_MULT = 6,
    DICE_DIV = 7,
    BASE_DAM = 8,
};

HIT_POINT monspell_damage(player_type* target_ptr, monster_spell_type ms_type, MONSTER_IDX m_idx, int TYPE);
HIT_POINT monspell_race_damage(player_type* target_ptr, monster_spell_type ms_type, MONRACE_IDX r_idx, int TYPE);
HIT_POINT monspell_bluemage_damage(player_type* target_ptr, monster_spell_type ms_type, PLAYER_LEVEL plev, int TYPE);
