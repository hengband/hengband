/*!
 * @brief プレイヤーの時限効果を表すオブジェクト群を保持する
 * @date 2022/08/05
 * @author Hourier
 */

#include "timed-effect/timed-effects.h"

PlayerBlindness &TimedEffects::blindness()
{
    return this->player_blindness;
}

const PlayerBlindness &TimedEffects::blindness() const
{
    return this->player_blindness;
}

PlayerConfusion &TimedEffects::confusion()
{
    return this->player_confusion;
}

const PlayerConfusion &TimedEffects::confusion() const
{
    return this->player_confusion;
}

PlayerCut &TimedEffects::cut()
{
    return this->player_cut;
}

const PlayerCut &TimedEffects::cut() const
{
    return this->player_cut;
}

PlayerFear &TimedEffects::fear()
{
    return this->player_fear;
}

const PlayerFear &TimedEffects::fear() const
{
    return this->player_fear;
}

PlayerHallucination &TimedEffects::hallucination()
{
    return this->player_hallucination;
}

const PlayerHallucination &TimedEffects::hallucination() const
{
    return this->player_hallucination;
}

PlayerParalysis &TimedEffects::paralysis()
{
    return this->player_paralysis;
}

const PlayerParalysis &TimedEffects::paralysis() const
{
    return this->player_paralysis;
}

PlayerStun &TimedEffects::stun()
{
    return this->player_stun;
}

const PlayerStun &TimedEffects::stun() const
{
    return this->player_stun;
}

PlayerAcceleration &TimedEffects::acceleration()
{
    return this->player_acceleration;
}

const PlayerAcceleration &TimedEffects::acceleration() const
{
    return this->player_acceleration;
}

PlayerDeceleration &TimedEffects::deceleration()
{
    return this->player_deceleration;
}

const PlayerDeceleration &TimedEffects::deceleration() const
{
    return this->player_deceleration;
}

PlayerPoison &TimedEffects::poison()
{
    return this->player_poison;
}

const PlayerPoison &TimedEffects::poison() const
{
    return this->player_poison;
}

PlayerProtection &TimedEffects::protection()
{
    return this->player_protection;
}

const PlayerProtection &TimedEffects::protection() const
{
    return this->player_protection;
}
