#pragma once

#include <memory>

class PlayerCut;
class PlayerStun;
class TimedEffects {
public:
    TimedEffects();
    virtual ~TimedEffects() = default;

    std::shared_ptr<PlayerCut> cut() const;
    std::shared_ptr<PlayerStun> stun() const;

private:
    std::shared_ptr<PlayerCut> player_cut;
    std::shared_ptr<PlayerStun> player_stun;
};
