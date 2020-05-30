/*!
 * @brief モンスターがプレーヤーへ攻撃する処理に関するユーティリティ
 * @date 2020/05/30
 * @author Hourier
 */

#include "combat/monster-attack-util.h"
#include "floor/floor.h"

monap_type *initialize_monap_type(player_type *target_ptr, monap_type *monap_ptr, MONSTER_IDX m_idx)
{
    monap_ptr->m_idx = m_idx;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monap_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    monap_ptr->act = NULL;
    return monap_ptr;
}