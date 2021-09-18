#include "timed-effect/timed-effects.h"
#include "timed-effect/player-stun.h"

TimedEffects::TimedEffects()
    : player_stun(std::make_shared<PlayerStun>())
{
}

std::shared_ptr<PlayerStun> TimedEffects::stun() const
{
    return this->player_stun;
}
