#pragma once

#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include <memory>

class PlayerAcceleration;
class PlayerDeceleration;
class PlayerFear;
class PlayerHallucination;
class PlayerParalysis;
class PlayerPoison;
class PlayerStun;
class TimedEffects {
public:
    TimedEffects();
    ~TimedEffects() = default;
    TimedEffects(const TimedEffects &) = delete;
    TimedEffects(TimedEffects &&) = delete;
    TimedEffects &operator=(const TimedEffects &) = delete;
    TimedEffects &operator=(TimedEffects &&) = delete;

    PlayerBlindness &blindness();
    const PlayerBlindness &blindness() const;
    PlayerConfusion &confusion();
    const PlayerConfusion &confusion() const;
    PlayerCut &cut();
    const PlayerCut &cut() const;
    std::shared_ptr<PlayerFear> fear() const;
    std::shared_ptr<PlayerHallucination> hallucination() const;
    std::shared_ptr<PlayerParalysis> paralysis() const;
    std::shared_ptr<PlayerStun> stun() const;
    std::shared_ptr<PlayerAcceleration> acceleration() const;
    std::shared_ptr<PlayerDeceleration> deceleration() const;
    std::shared_ptr<PlayerPoison> poison() const;

private:
    PlayerBlindness player_blindness{};
    PlayerConfusion player_confusion{};
    PlayerCut player_cut{};
    std::shared_ptr<PlayerFear> player_fear;
    std::shared_ptr<PlayerHallucination> player_hallucination;
    std::shared_ptr<PlayerParalysis> player_paralysis;
    std::shared_ptr<PlayerStun> player_stun;
    std::shared_ptr<PlayerAcceleration> player_acceleration;
    std::shared_ptr<PlayerDeceleration> player_deceleration;
    std::shared_ptr<PlayerPoison> player_poison;
};
