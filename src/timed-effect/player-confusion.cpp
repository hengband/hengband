#include "timed-effect/player-confusion.h"

short PlayerConfusion::current() const
{
    return this->confusion;
}

bool PlayerConfusion::is_confused() const
{
    return this->confusion > 0;
}

void PlayerConfusion::set(short value)
{
    this->confusion = value;
}

void PlayerConfusion::reset()
{
    this->set(0);
}
