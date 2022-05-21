#include "timed-effect/timed-effects.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-stun.h"

TimedEffects::TimedEffects()
    : player_confusion(std::make_shared<PlayerConfusion>())
    , player_cut(std::make_shared<PlayerCut>())
    , player_fear(std::make_shared<PlayerFear>())
    , player_hallucination(std::make_shared<PlayerHallucination>())
    , player_paralysis(std::make_shared<PlayerParalysis>())
    , player_stun(std::make_shared<PlayerStun>())
    , player_acceleration(std::make_shared<PlayerAcceleration>())
{
}

std::shared_ptr<PlayerConfusion> TimedEffects::confusion() const
{
    return this->player_confusion;
}

std::shared_ptr<PlayerCut> TimedEffects::cut() const
{
    return this->player_cut;
}

std::shared_ptr<PlayerFear> TimedEffects::fear() const
{
    return this->player_fear;
}

std::shared_ptr<PlayerHallucination> TimedEffects::hallucination() const
{
    return this->player_hallucination;
}

std::shared_ptr<PlayerParalysis> TimedEffects::paralysis() const
{
    return this->player_paralysis;
}

std::shared_ptr<PlayerStun> TimedEffects::stun() const
{
    return this->player_stun;
}

std::shared_ptr<PlayerAcceleration> TimedEffects::acceleration() const
{
    return this->player_acceleration;
}
