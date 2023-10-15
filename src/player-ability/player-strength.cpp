#include "player-ability/player-strength.h"
#include "mutation/mutation-flag-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerStrength::PlayerStrength(PlayerType *player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

void PlayerStrength::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_STR;
    this->tr_flag = TR_STR;
    this->tr_bad_flag = TR_STR;
}

/*!
 * @brief 腕力補正計算 - 種族
 * @return 腕力補正値
 */
int16_t PlayerStrength::race_bonus()
{
    int16_t result = PlayerBasicStatistics::race_bonus();

    result += PlayerRace(this->player_ptr).additional_strength();

    return result;
}

/*!
 * @brief 腕力補正計算 - 一時効果
 * @return 腕力補正値
 * @details
 * * 一時効果による腕力修正値
 * * 呪術の腕力強化で加算(+4)
 * * 呪術の肉体強化で加算(+4)
 * * ネオ・つよしスペシャル中で加算(+4)
 */
int16_t PlayerStrength::time_effect_bonus()
{
    int16_t result = 0;

    if (this->player_ptr->realm1 == REALM_HEX) {
        SpellHex spell_hex(this->player_ptr);
        if (spell_hex.is_spelling_specific(HEX_XTRA_MIGHT)) {
            result += 4;
        }
        if (spell_hex.is_spelling_specific(HEX_BUILDING)) {
            result += 4;
        }
    }

    if (this->player_ptr->tsuyoshi) {
        result += 4;
    }

    return result;
}

/*!
 * @brief 腕力補正計算 - 型
 * @return 腕力補正値
 * @details
 * * 型による腕力修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 朱雀の構えで減算(-2)
 */
int16_t PlayerStrength::stance_bonus()
{
    int16_t result = 0;

    PlayerClass pc(player_ptr);
    if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        result += 5;
    }

    if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        result += 2;
    } else if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        result -= 2;
    }

    return result;
}

/*!
 * @brief 腕力補正計算 - 変異
 * @return 腕力補正値
 * @details
 * * 変異による腕力修正値
 * * 変異MUT3_HYPER_STRで加算(+4)
 * * 変異MUT3_PUNYで減算(-4)
 */
int16_t PlayerStrength::mutation_bonus()
{
    int16_t result = 0;

    if (this->player_ptr->muta.any()) {
        if (this->player_ptr->muta.has(PlayerMutationType::HYPER_STR)) {
            result += 4;
        }

        if (this->player_ptr->muta.has(PlayerMutationType::PUNY)) {
            result -= 4;
        }
    }

    return result;
}
