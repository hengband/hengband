#include "timed-effect/timed-effects.h"

std::shared_ptr<PlayerStun> TimedEffects::stun() const
{
    return this->player_stun;
}
