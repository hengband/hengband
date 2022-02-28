#pragma once

#include <memory>

class PlayerConfusion;
class PlayerHallucination;
class PlayerParalysis;
class PlayerCut;
class PlayerStun;
class TimedEffects {
public:
    TimedEffects();
    virtual ~TimedEffects() = default;

    std::shared_ptr<PlayerCut> cut() const;
    std::shared_ptr<PlayerStun> stun() const;
    std::shared_ptr<PlayerConfusion> confusion() const;
    std::shared_ptr<PlayerHallucination> hallucination() const;
    std::shared_ptr<PlayerParalysis> paralysis() const;

private:
    std::shared_ptr<PlayerCut> player_cut;
    std::shared_ptr<PlayerStun> player_stun;
    std::shared_ptr<PlayerConfusion> player_confusion;
    std::shared_ptr<PlayerHallucination> player_hallucination;
    std::shared_ptr<PlayerParalysis> player_paralysis;
};
