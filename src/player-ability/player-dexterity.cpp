#include "player-ability/player-dexterity.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerDexterity::PlayerDexterity(player_type* player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

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
 */
int16_t PlayerDexterity::race_value()
{
    int16_t result = PlayerBasicStatistics::race_value();

    result += PlayerRace(this->player_ptr).additional_dexterity();

    return result;
}

/*!
 * @brief 器用さ補正計算 - 一時効果
 * @return 器用さ補正値
 * @details
 * * 一時効果による器用さ修正値
 * * 呪術の肉体強化で加算(+4)
 */
int16_t PlayerDexterity::time_effect_value()
{
    int16_t result = 0;

    if (this->player_ptr->realm1 == REALM_HEX) {
        if (SpellHex(this->player_ptr).is_spelling_specific(HEX_BUILDING)) {
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
int16_t PlayerDexterity::battleform_value()
{
    int16_t result = 0;

    PlayerClass pc(player_ptr);
    if (pc.samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (pc.monk_stance_is(MonkStance::BYAKKO)) {
        result += 2;
    } else if (pc.monk_stance_is(MonkStance::GENBU)) {
        result -= 2;
    } else if (pc.monk_stance_is(MonkStance::SUZAKU)) {
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
int16_t PlayerDexterity::mutation_value()
{
    int16_t result = 0;

    if (this->player_ptr->muta.has(MUTA::IRON_SKIN)) {
        result -= 1;
    }

    if (this->player_ptr->muta.has(MUTA::LIMBER)) {
        result += 3;
    }

    if (this->player_ptr->muta.has(MUTA::ARTHRITIS)) {
        result -= 3;
    }

    return result;
}
