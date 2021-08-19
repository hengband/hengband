﻿#include "player-ability/player-dexterity.h"
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

void PlayerDexterity::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_DEX;
    this->tr_flag = TR_DEX;
    this->tr_bad_flag = TR_DEX;
}

/*!
 * @brief 器用さ補正計算 - 種族
 * @return 器用さ補正値
 * @details
 * * 種族による器用さ修正値。
 * * エントは別途レベル26,41,46到達ごとに減算(-1)
 */
short PlayerDexterity::race_value()
{
    short result = PlayerBasicStatistics::race_value();

    if (is_specific_player_race(this->owner_ptr, player_race_type::ENT)) {
        if (this->owner_ptr->lev > 25)
            result--;
        if (this->owner_ptr->lev > 40)
            result--;
        if (this->owner_ptr->lev > 45)
            result--;
    }

    return result;
}

/*!
 * @brief 器用さ補正計算 - 一時効果
 * @return 器用さ補正値
 * @details
 * * 一時効果による器用さ修正値
 * * 呪術の肉体強化で加算(+4)
 */
short PlayerDexterity::time_effect_value()
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
 * @brief 器用さ補正計算 - 型
 * @return 器用さ補正値
 * @details
 * * 型による器用さ修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 玄武の構えで減算(-2)
 * * 朱雀の構えで加算(+2)
 */
short PlayerDexterity::battleform_value()
{
    short result = 0;

    if (any_bits(this->owner_ptr->special_defense, KATA_KOUKIJIN)) {
        result += 5;
    }

    if (any_bits(this->owner_ptr->special_defense, KAMAE_BYAKKO)) {
        result += 2;
    } else if (any_bits(this->owner_ptr->special_defense, KAMAE_GENBU)) {
        result -= 2;
    } else if (any_bits(this->owner_ptr->special_defense, KAMAE_SUZAKU)) {
        result += 2;
    }

    return result;
}

/*!
 * @brief 器用さ腕力補正計算 - 変異
 * @return 器用さ補正値
 * @details
 * * 変異による器用さ修正値
 * * 変異MUT3_IRON_SKINで減算(-1)
 * * 変異MUT3_LIMBERで加算(+3)
 * * 変異MUT3_ARTHRITISで減算(-3)
 */
short PlayerDexterity::mutation_value()
{
    short result = 0;

    if (this->owner_ptr->muta.has(MUTA::IRON_SKIN)) {
        result -= 1;
    }

    if (this->owner_ptr->muta.has(MUTA::LIMBER)) {
        result += 3;
    }

    if (this->owner_ptr->muta.has(MUTA::ARTHRITIS)) {
        result -= 3;
    }

    return result;
}
