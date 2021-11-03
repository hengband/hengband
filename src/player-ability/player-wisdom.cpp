﻿#include "player-ability/player-wisdom.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerWisdom::PlayerWisdom(player_type *player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

void PlayerWisdom::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_WIS;
    this->tr_flag = TR_WIS;
    this->tr_bad_flag = TR_WIS;
}

/*!
 * @brief 賢さ補正計算 - 型
 * @return 賢さ補正値
 */
int16_t PlayerWisdom::stance_value()
{
    return PlayerClass(this->player_ptr).stance_wisdom();
}

/*!
 * @brief 賢さ補正計算 - 変異
 * @return 賢さ補正値
 * @details
 * * 変異による賢さ修正値
 * * 変異MUT3_HYPER_INTで加算(+4)
 * * 変異MUT3_MORONICで減算(-4)
 */
int16_t PlayerWisdom::mutation_value()
{
    int16_t result = 0;

    if (this->player_ptr->muta.any()) {
        if (this->player_ptr->muta.has(MUTA::HYPER_INT)) {
            result += 4;
        }

        if (this->player_ptr->muta.has(MUTA::MORONIC)) {
            result -= 4;
        }
    }

    return result;
}
