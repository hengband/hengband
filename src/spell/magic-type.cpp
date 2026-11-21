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

/*!
 * @brief 習得レベルを得る
 * @return 習得レベル
 * @details 習得できる魔法でだけ呼ぶ。習得できない魔法で呼ぶと tl::bad_optional_access を投げる。
 */
PLAYER_LEVEL magic_type::get_learn_level() const
{
    return this->slevel.value();
}

/*!
 * @brief 習得レベルを設定する (習得できる魔法にする)
 * @param level 習得レベル
 */
void magic_type::set_learn_level(PLAYER_LEVEL level)
{
    this->slevel = level;
}
