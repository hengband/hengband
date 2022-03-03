#include "player-ability/player-intelligence.h"
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
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerIntelligence::PlayerIntelligence(PlayerType *player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

void PlayerIntelligence::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_INT;
    this->tr_flag = TR_INT;
    this->tr_bad_flag = TR_INT;
}

/*!
 * @brief 知力補正計算 - 型
 * @return 知力補正値
 * @details
 * * 型による知力修正値
 * * 降鬼陣で加算(+5)
 * * 玄武の構えで減算(-1)
 * * 朱雀の構えで加算(+1)
 */
int16_t PlayerIntelligence::stance_bonus()
{
    int16_t result = 0;

    PlayerClass pc(player_ptr);
    if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        result += 5;
    }

    if (pc.monk_stance_is(MonkStanceType::GENBU)) {
        result -= 1;
    } else if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        result += 1;
    }

    return result;
}

/*!
 * @brief 知力補正計算 - 変異
 * @return 知力補正値
 * @details
 * * 変異による知力修正値
 * * 変異MUT3_HYPER_INTで加算(+4)
 * * 変異MUT3_MORONICで減算(-4)
 */
int16_t PlayerIntelligence::mutation_bonus()
{
    int16_t result = 0;
    if (this->player_ptr->muta.any()) {
        if (this->player_ptr->muta.has(PlayerMutationType::HYPER_INT)) {
            result += 4;
        }

        if (this->player_ptr->muta.has(PlayerMutationType::MORONIC)) {
            result -= 4;
        }
    }
    return result;
}
