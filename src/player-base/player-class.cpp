/*!
 * @brief プレイヤーの職業クラスに基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details 本クラス作成時点で責務に対する余裕はかなりあるので、適宜ここへ移してくること.
 */
#include "player-base/player-class.h"
#include "system/player-type-definition.h"

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
