#include "timed-effect/player-wide-spread.h"
#include "system/angband-exceptions.h"

short PlayerWideSpread::current() const
{
    return this->wide_spread;
}

bool PlayerWideSpread::is_wide_spreaded() const
{
    return this->current() > 0;
}

void PlayerWideSpread::set(short value)
{
    if (value < 0) {
        THROW_EXCEPTION(std::invalid_argument, "Negative value can't be set in the player's wide-spread parameter!");
    }

    this->wide_spread = value;
}
