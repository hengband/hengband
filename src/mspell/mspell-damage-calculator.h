#pragma once

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

enum class MonsterAbilityType;
enum class MonsterRaceId : int16_t;
class PlayerType;
int monspell_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, MONSTER_IDX m_idx, int TYPE);
int monspell_race_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, MonsterRaceId r_idx, int TYPE);
int monspell_bluemage_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, PLAYER_LEVEL plev, int TYPE);
