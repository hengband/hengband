/*!
 * @brief プレイヤーの一時加速ステータス変更と判定
 * @date 2022/08/15
 * @author Hourier
 */

#include "timed-effect/player-acceleration.h"
#include "system/angband-exceptions.h"

short PlayerAcceleration::current() const
{
    return this->acceleration;
}

bool PlayerAcceleration::is_fast() const
{
    return this->acceleration > 0;
}

void PlayerAcceleration::set(short value)
{
    if (value < 0) {
        THROW_EXCEPTION(std::invalid_argument, "Negative value can't be set in the player's acceleration parameter!");
    }

    this->acceleration = value;
}

void PlayerAcceleration::reset()
{
    this->set(0);
}
