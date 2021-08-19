﻿#include "player-status/player-infravision.h"
#include "mutation/mutation-flag-types.h"
#include "player/mimic-info-table.h"
#include "player/player-race-types.h"
#include "player/race-info-table.h"
#include "system/player-type-definition.h"
/*
 * @brief 赤外線視力 - 初期値、下限、上限
 */
void PlayerInfravision::set_locals()
{
    this->default_value = 0;
    this->min_value = 0;
    this->max_value = 20;
    this->tr_flag = TR_INFRA;
    this->tr_bad_flag = TR_INFRA;
}

/*!
 * @brief 赤外線視力計算 - 種族
 * @return 赤外線視力の増分
 * @details
 * * 種族による加算
 */
short PlayerInfravision::race_value()
{
    const player_race *tmp_rp_ptr;

    if (this->owner_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[this->owner_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[static_cast<int>(this->owner_ptr->prace)];

    return tmp_rp_ptr->infra;
}

/*!
 * @brief 赤外線視力計算 - 変異
 * @return 赤外線視力の増分
 * @details
 * * 変異MUT3_INFRAVISによる加算(+3)
 */
short PlayerInfravision::mutation_value()
{
    short result = 0;
    if (this->owner_ptr->muta.has(MUTA::INFRAVIS)) {
        result += 3;
    }

    return result;
}

/*!
 * @brief 赤外線視力計算 - 一時効果
 * @return 赤外線視力の増分
 * @details
 * * 魔法効果tim_infraによる加算(+3)
 */
short PlayerInfravision::time_effect_value()
{
    short result = 0;
    if (this->owner_ptr->tim_infra) {
        result += 3;
    }

    return result;
}
