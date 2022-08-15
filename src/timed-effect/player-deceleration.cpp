/*!
 * @brief プレイヤーの一時減速ステータス変更と判定
 * @date 2022/08/05
 * @author Hourier
 */

#include "timed-effect/player-deceleration.h"
#include <stdexcept>

short PlayerDeceleration::current() const
{
    return this->deceleration;
}

bool PlayerDeceleration::is_slow() const
{
    return this->deceleration > 0;
}

void PlayerDeceleration::set(short value)
{
    if (value < 0) {
        throw std::invalid_argument("Negative value can't be set in the player's deceleration parameter!");
    }

    this->deceleration = value;
}

void PlayerDeceleration::reset()
{
    this->deceleration = 0;
}
