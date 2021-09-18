#include "system/player-type-definition.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 */
player_type p_body;

/*!
 * @brief プレイヤー構造体へのグローバル参照ポインタ / Pointer to the player info
 */
player_type *p_ptr = &p_body;

player_type::player_type()
    : timed_effects(std::make_shared<TimedEffects>())
{
}

std::shared_ptr<TimedEffects> player_type::effects() const
{
    return this->timed_effects;
}
