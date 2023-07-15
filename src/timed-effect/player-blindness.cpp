/*!
 * @brief プレイヤーの盲目ステータス変更と判定
 * @date 2022/08/30
 * @author Hourier
 */

#include "timed-effect/player-blindness.h"
#include "system/angband-exceptions.h"

short PlayerBlindness::current() const
{
    return this->blindness;
}

bool PlayerBlindness::is_blind() const
{
    return this->blindness > 0;
}

void PlayerBlindness::set(short value)
{
    if (value < 0) {
        THROW_EXCEPTION(std::invalid_argument, "Negative value can't be set in the player's blindness parameter!");
    }

    this->blindness = value;
}

void PlayerBlindness::reset()
{
    this->set(0);
}
