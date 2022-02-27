#include "timed-effect/player-paralysis.h"

short PlayerParalysis::current() const
{
    return this->paralysis;
}

bool PlayerParalysis::is_paralyzed() const
{
    return this->paralysis > 0;
}

void PlayerParalysis::set(short value)
{
    this->paralysis = value;
}

void PlayerParalysis::reset()
{
    this->set(0);
}
