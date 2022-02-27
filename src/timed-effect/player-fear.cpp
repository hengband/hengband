#include "timed-effect/player-fear.h"

short PlayerFear::current() const
{
    return this->fear;
}

bool PlayerFear::is_fearful() const
{
    return this->fear > 0;
}

void PlayerFear::set(short value)
{
    this->fear = value;
}

void PlayerFear::reset()
{
    this->set(0);
}
