#include "timed-effect/player-hallucination.h"

short PlayerHallucination::current() const
{
    return this->hallucination;
}

bool PlayerHallucination::is_hallucinated() const
{
    return this->hallucination > 0;
}

void PlayerHallucination::set(short value)
{
    this->hallucination = value;
}

void PlayerHallucination::reset()
{
    this->set(0);
}
