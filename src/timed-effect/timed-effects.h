#pragma once

#include <memory>

class PlayerStun;
class TimedEffects {
public:
    TimedEffects();
    virtual ~TimedEffects() = default;

    std::shared_ptr<PlayerStun> stun() const;

private:
    std::shared_ptr<PlayerStun> player_stun;
};
