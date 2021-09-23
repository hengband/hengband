/*!
 * @brief プレイヤーの職業クラスに基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details 本クラス作成時点で責務に対する余裕はかなりあるので、適宜ここへ移してくること.
 */
#include "player-base/player-class.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerClass::PlayerClass(player_type* player_ptr)
    : player_ptr(player_ptr)
{
}

bool PlayerClass::can_resist_stun() const
{
    return (this->player_ptr->pclass == CLASS_BERSERKER) && (this->player_ptr->lev > 34);
}

bool PlayerClass::is_wizard() const
{
    auto is_wizard = this->player_ptr->pclass == CLASS_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_HIGH_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_SORCERER;
    is_wizard |= this->player_ptr->pclass == CLASS_MAGIC_EATER;
    is_wizard |= this->player_ptr->pclass == CLASS_BLUE_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_ELEMENTALIST;
    return is_wizard;
}

bool PlayerClass::lose_balance()
{
    if (this->player_ptr->pclass != CLASS_SAMURAI) {
        return false;
    }

    if (none_bits(this->player_ptr->special_defense, KATA_MASK)) {
        return false;
    }

    reset_bits(this->player_ptr->special_defense, KATA_MASK);
    this->player_ptr->update |= PU_BONUS;
    this->player_ptr->update |= PU_MONSTERS;
    this->player_ptr->redraw |= PR_STATE;
    this->player_ptr->redraw |= PR_STATUS;
    this->player_ptr->action = ACTION_NONE;
    return true;
}
