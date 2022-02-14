#include "system/player-type-definition.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "market/arena-info-table.h"
#include "world/world.h"

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 */
PlayerType p_body;

/*!
 * @brief プレイヤー構造体へのグローバル参照ポインタ / Pointer to the player info
 */
PlayerType *p_ptr = &p_body;

PlayerType::PlayerType()
    : timed_effects(std::make_shared<TimedEffects>())
{
}

bool PlayerType::is_true_winner() const
{
    return (w_ptr->total_winner > 0) && (this->arena_number > MAX_ARENA_MONS + 2);
}

std::shared_ptr<TimedEffects> PlayerType::effects() const
{
    return this->timed_effects;
}
