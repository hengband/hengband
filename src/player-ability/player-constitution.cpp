#include "player-ability/player-constitution.h"
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

PlayerConstitution::PlayerConstitution(player_type *player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

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
 */
int16_t PlayerConstitution::race_value()
{
    int16_t result = PlayerBasicStatistics::race_value();

    result += PlayerRace(this->player_ptr).additional_constitution();

    return result;
}

/*!
 * @brief 耐久力補正計算 - 一時効果
 * @return 耐久力補正値
 * @details
 * * 一時効果による耐久力修正値
 * * 呪術の肉体強化で加算(+4)
 */
int16_t PlayerConstitution::time_effect_value()
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
int16_t PlayerConstitution::stance_value()
{
    int16_t result = 0;

    PlayerClass pc(player_ptr);
    if (pc.samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (pc.monk_stance_is(MonkStance::BYAKKO)) {
        result -= 3;
    } else if (pc.monk_stance_is(MonkStance::GENBU)) {
        result += 3;
    } else if (pc.monk_stance_is(MonkStance::SUZAKU)) {
        result -= 2;
    }
    if (this->player_ptr->tsuyoshi) {
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
int16_t PlayerConstitution::mutation_value()
{
    int16_t result = 0;

    if (this->player_ptr->muta.any()) {
        if (this->player_ptr->muta.has(MUTA::RESILIENT)) {
            result += 4;
        }

        if (this->player_ptr->muta.has(MUTA::ALBINO)) {
            result -= 4;
        }

        if (this->player_ptr->muta.has(MUTA::XTRA_FAT)) {
            result += 2;
        }

        if (this->player_ptr->muta.has(MUTA::FLESH_ROT)) {
            result -= 2;
        }
    }

    return result;
}
