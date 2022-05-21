#include "timed-effect/player-acceleration.h"

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
    this->acceleration = value;
}

void PlayerAcceleration::add(short value)
{
    this->acceleration += value;
}

void PlayerAcceleration::reset()
{
    this->set(0);
}
