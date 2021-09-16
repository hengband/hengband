#include "timed-effect/player-stun.h"

short PlayerStun::current() const
{
    return this->stun;
}

StunRank PlayerStun::get_rank() const
{
    return this->get_rank(this->stun);
}

StunRank PlayerStun::get_rank(short value) const
{
    if (value > 100) {
        return StunRank::UNCONSCIOUS;
    }

    if (value > 50) {
        return StunRank::HARD;
    }

    if (value > 0) {
        return StunRank::NORMAL;
    }

    return StunRank::NONE;
}

void PlayerStun::set(short value)
{
}
