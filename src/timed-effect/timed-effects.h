#pragma once

#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effect.h"

using PlayerBlindness = TimedEffect<struct TagPlayerBlindness>;
using PlayerConfusion = TimedEffect<struct TagPlayerConfusion>;
using PlayerFear = TimedEffect<struct TagPlayerFear>;
using PlayerHallucination = TimedEffect<struct TagPlayerHallucination>;
using PlayerParalysis = TimedEffect<struct TagPlayerParalysis>;
using PlayerPoison = TimedEffect<struct TagPlayerPoison>;
using PlayerAcceleration = TimedEffect<struct TagPlayerAcceleration>;
using PlayerDeceleration = TimedEffect<struct TagPlayerDeceleration>;
using PlayerProtection = TimedEffect<struct TagPlayerProtection>;

class TimedEffects {
public:
    TimedEffects() = default;
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
    PlayerAcceleration &acceleration();
    const PlayerAcceleration &acceleration() const;
    PlayerDeceleration &deceleration();
    const PlayerDeceleration &deceleration() const;
    PlayerPoison &poison();
    const PlayerPoison &poison() const;
    PlayerProtection &protection();
    const PlayerProtection &protection() const;

private:
    PlayerBlindness player_blindness{};
    PlayerConfusion player_confusion{};
    PlayerCut player_cut{};
    PlayerFear player_fear{};
    PlayerHallucination player_hallucination{};
    PlayerParalysis player_paralysis{};
    PlayerStun player_stun{};
    PlayerAcceleration player_acceleration{};
    PlayerDeceleration player_deceleration{};
    PlayerPoison player_poison{};
    PlayerProtection player_protection{};
};
