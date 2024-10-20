#include "timed-effect/player-protection.h"

short PlayerProtection::current() const
{
    return this->protection;
}

bool PlayerProtection::is_protected() const
{
    return this->protection > 0;
}

bool PlayerProtection::is_larger_than(short value) const
{
    return this->protection > value;
}

void PlayerProtection::set(short value)
{
    this->protection = value;
}

void PlayerProtection::reset()
{
    this->protection = 0;
}
