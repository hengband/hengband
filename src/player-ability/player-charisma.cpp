#include "player-ability/player-charisma.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/samurai-data-type.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerCharisma::PlayerCharisma(PlayerType *player_ptr)
    : PlayerBasicStatistics(player_ptr)
{
}

void PlayerCharisma::set_locals()
{
    this->max_value = +99;
    this->min_value = -99;
    this->ability_type = A_CHR;
    this->tr_flag = TR_CHR;
    this->tr_bad_flag = TR_CHR;
}

/*!
 * @brief 魅力補正計算 - 型
 * @return 魅力補正値
 * @details
 * * 型による魅力修正値
 * * 降鬼陣で加算(+5)
 */
int16_t PlayerCharisma::stance_bonus()
{
    int16_t result = 0;

    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        result += 5;
    }

    return result;
}

/*!
 * @brief 魅力補正計算 - 変異
 * @return 魅力補正値
 * @details
 * * 変異による魅力修正値
 * * 変異MUT3_FLESH_ROTで減算(-1)
 * * 変異MUT3_SILLY_VOIで減算(-4)
 * * 変異MUT3_BLANK_FACで減算(-1)
 * * 変異MUT3_WART_SKINで減算(-2)
 * * 変異MUT3_SCALESで減算(-1)
 */
int16_t PlayerCharisma::mutation_bonus()
{
    int16_t result = 0;

    if (this->player_ptr->muta.any()) {
        if (this->player_ptr->muta.has(PlayerMutationType::FLESH_ROT)) {
            result -= 1;
        }
        if (this->player_ptr->muta.has(PlayerMutationType::SILLY_VOI)) {
            result -= 4;
        }
        if (this->player_ptr->muta.has(PlayerMutationType::BLANK_FAC)) {
            result -= 1;
        }
        if (this->player_ptr->muta.has(PlayerMutationType::WART_SKIN)) {
            result -= 2;
        }
        if (this->player_ptr->muta.has(PlayerMutationType::SCALES)) {
            result -= 1;
        }
    }

    return result;
}

int16_t PlayerCharisma::set_exception_bonus(int16_t value)
{
    int16_t result = value;

    if (this->player_ptr->muta.has(PlayerMutationType::ILL_NORM)) {
        result = 0;
    }

    return result;
}

BIT_FLAGS PlayerCharisma::get_all_flags()
{
    BIT_FLAGS flags = PlayerStatusBase::get_all_flags();

    if (this->player_ptr->muta.has(PlayerMutationType::ILL_NORM)) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    return flags;
}

BIT_FLAGS PlayerCharisma::get_bad_flags()
{
    BIT_FLAGS flags = PlayerStatusBase::get_bad_flags();

    if (this->player_ptr->muta.has(PlayerMutationType::ILL_NORM)) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    return flags;
}

/*!
 * @brief ステータス現在値更新の例外処理
 * @param 通常処理されたステータスの値
 * @returns 例外処理されたステータスの値
 * @details
 * * MUT3_ILL_NORMを保持しているときの例外処理。
 * * 魅力現在値をレベル依存の値に修正する。
 */
int16_t PlayerCharisma::set_exception_use_status(int16_t value)
{
    if (this->player_ptr->muta.has(PlayerMutationType::ILL_NORM)) {
        /* 10 to 18/90 charisma, guaranteed, based on level */
        if (value < 8 + 2 * this->player_ptr->lev) {
            value = 8 + 2 * this->player_ptr->lev;
        }
    }
    return value;
}
