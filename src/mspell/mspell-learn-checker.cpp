#include "mspell-learn-checker.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->blind && m_ptr->ml);

    bool maneable = player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx);
    return (seen && maneable && !w_ptr->timewalk_m_idx);
}
