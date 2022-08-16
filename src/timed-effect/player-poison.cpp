/*!
 * @brief プレイヤーの一時減速ステータス変更と判定
 * @date 2022/08/16
 * @author Hourier
 */

#include "timed-effect/player-poison.h"
#include <stdexcept>

short PlayerPoison::current() const
{
    return this->poison;
}

bool PlayerPoison::is_poisoned() const
{
    return this->poison > 0;
}

void PlayerPoison::set(short value)
{
    if (value < 0) {
        throw std::invalid_argument("Negative value can't be set in the player's poison parameter!");
    }

    this->poison = value;
}

void PlayerPoison::reset()
{
    this->poison = 0;
}
