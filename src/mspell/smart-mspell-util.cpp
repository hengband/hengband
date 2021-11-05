﻿#include "mspell/smart-mspell-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags2.h"
#include "monster/smart-learn-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

msr_type *initialize_msr_type(PlayerType *player_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const EnumClassFlagGroup<RF_ABILITY> &ability_flags)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    msr_ptr->r_ptr = &r_info[m_ptr->r_idx];
    msr_ptr->ability_flags = ability_flags;
    msr_ptr->smart.clear();
    return msr_ptr;
}

/*!
 * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
 * Internal probability routine
 * @param r_ptr モンスター種族の構造体参照ポインタ
 * @param prob 基本確率(%)
 * @return 適した選択を取るならばTRUEを返す。
 */
bool int_outof(monster_race *r_ptr, PERCENTAGE prob)
{
    if (!(r_ptr->flags2 & RF2_SMART))
        prob = prob / 2;

    return (randint0(100) < prob);
}
