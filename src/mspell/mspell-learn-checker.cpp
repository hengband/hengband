#include "mspell/mspell-learn-checker.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"

/*!
 * @brief モンスターの唱えた呪文を青魔法で学習できるか判定する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return プレイヤーが青魔法で学習できるならTRUE、そうでなければFALSEを返す。
 *
 * モンスターが特技を使う前にプレイヤーがモンスターを視認できているかどうかの判定用。
 */
bool spell_learnable(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto seen = (!player_ptr->effects()->blindness()->is_blind() && monster.ml);
    const auto maneable = floor.has_los({ monster.fy, monster.fx });
    return seen && maneable && (w_ptr->timewalk_m_idx == 0);
}
