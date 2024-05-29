#pragma once

#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-stun.h"
#include <memory>

class PlayerAcceleration;
class PlayerDeceleration;
class PlayerPoison;
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
    PlayerFear &fear();
    const PlayerFear &fear() const;
    PlayerHallucination &hallucination();
    const PlayerHallucination &hallucination() const;
    PlayerParalysis &paralysis();
    const PlayerParalysis &paralysis() const;
    PlayerStun &stun();
    const PlayerStun &stun() const;
    std::shared_ptr<PlayerAcceleration> acceleration() const;
    std::shared_ptr<PlayerDeceleration> deceleration() const;
    std::shared_ptr<PlayerPoison> poison() const;

private:
    PlayerBlindness player_blindness{};
    PlayerConfusion player_confusion{};
    PlayerCut player_cut{};
    PlayerFear player_fear{};
    PlayerHallucination player_hallucination{};
    PlayerParalysis player_paralysis{};
    PlayerStun player_stun{};
    std::shared_ptr<PlayerAcceleration> player_acceleration;
    std::shared_ptr<PlayerDeceleration> player_deceleration;
    std::shared_ptr<PlayerPoison> player_poison;
};
