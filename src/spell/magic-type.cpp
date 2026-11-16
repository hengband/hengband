#include "spell/magic-type.h"

/*!
 * @brief 習得できる魔法かどうか
 * @return 習得レベルが定まっていれば true
 */
bool magic_type::is_learnable() const
{
    return this->slevel.has_value();
}

/*!
 * @brief 指定のレベルで習得できる魔法かどうか
 * @param level プレイヤーのレベル
 * @return 習得できる魔法で、習得レベルが level 以下なら true
 */
bool magic_type::can_learn_at(PLAYER_LEVEL level) const
{
    return this->slevel && (*this->slevel <= level);
}
