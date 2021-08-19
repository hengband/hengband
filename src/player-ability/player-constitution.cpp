﻿#include "player-ability/player-constitution.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

void PlayerConstitution::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_CON;
    this->tr_flag = TR_CON;
    this->tr_bad_flag = TR_CON;
}

/*!
 * @brief 耐久力補正計算 - 種族
 * @return 耐久力補正値
 * @details
 * * 種族による耐久力修正値。
 * * エントは別途レベル26,41,46到達ごとに加算(+1)
 */
short PlayerConstitution::race_value()
{
    short result = PlayerBasicStatistics::race_value();

    if (is_specific_player_race(this->owner_ptr, player_race_type::ENT)) {
        if (this->owner_ptr->lev > 25)
            result++;
        if (this->owner_ptr->lev > 40)
            result++;
        if (this->owner_ptr->lev > 45)
            result++;
    }

    return result;
}

/*!
 * @brief 耐久力補正計算 - 一時効果
 * @return 耐久力補正値
 * @details
 * * 一時効果による耐久力修正値
 * * 呪術の肉体強化で加算(+4)
 */
short PlayerConstitution::time_effect_value()
{
    short result = 0;

    if (this->owner_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(this->owner_ptr, HEX_BUILDING)) {
            result += 4;
        }
    }

    return result;
}

/*!
 * @brief 耐久力補正計算 - 型
 * @return 耐久力補正値
 * @details
 * * 型による耐久力修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで減算(-3)
 * * 玄武の構えで加算(+3)
 * * 朱雀の構えで減算(-2)
 * * ネオ・つよしスペシャル中で加算(+4)
 */
short PlayerConstitution::battleform_value()
{
    short result = 0;

    if (any_bits(this->owner_ptr->special_defense, KATA_KOUKIJIN)) {
        result += 5;
    }

    if (any_bits(this->owner_ptr->special_defense, KAMAE_BYAKKO)) {
        result -= 3;
    } else if (any_bits(this->owner_ptr->special_defense, KAMAE_GENBU)) {
        result += 3;
    } else if (any_bits(this->owner_ptr->special_defense, KAMAE_SUZAKU)) {
        result -= 2;
    }
    if (this->owner_ptr->tsuyoshi) {
        result += 4;
    }

    return result;
}

/*!
 * @brief 耐久力補正計算 - 変異
 * @return 耐久力補正値
 * @details
 * * 変異による耐久力修正値
 * * 変異MUT3_RESILIENTで加算(+4)
 * * 変異MUT3_ALBINOで減算(-4)
 * * 変異MUT3_XTRA_FATで加算(+2)
 * * 変異MUT3_FLESH_ROTで減算(-2)
 */
short PlayerConstitution::mutation_value()
{
    short result = 0;

    if (this->owner_ptr->muta.any()) {
        if (this->owner_ptr->muta.has(MUTA::RESILIENT)) {
            result += 4;
        }

        if (this->owner_ptr->muta.has(MUTA::ALBINO)) {
            result -= 4;
        }

        if (this->owner_ptr->muta.has(MUTA::XTRA_FAT)) {
            result += 2;
        }

        if (this->owner_ptr->muta.has(MUTA::FLESH_ROT)) {
            result -= 2;
        }
    }

    return result;
}
