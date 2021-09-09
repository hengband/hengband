#pragma once

#include "timed-effect/player-stun.h"
#include <memory>

class PlayerStun;
class TimedEffects {
public:
    TimedEffects() = default;
    virtual ~TimedEffects() = default;

    std::shared_ptr<PlayerStun> stun() const;

private:
    std::shared_ptr<PlayerStun> player_stun = std::shared_ptr<PlayerStun>(new PlayerStun());
};
