#pragma once

#include <memory>

class PlayerConfusion;
class PlayerCut;
class PlayerStun;
class TimedEffects {
public:
    TimedEffects();
    virtual ~TimedEffects() = default;

    std::shared_ptr<PlayerCut> cut() const;
    std::shared_ptr<PlayerStun> stun() const;
    std::shared_ptr<PlayerConfusion> confusion() const;

private:
    std::shared_ptr<PlayerCut> player_cut;
    std::shared_ptr<PlayerStun> player_stun;
    std::shared_ptr<PlayerConfusion> player_confusion;
};
