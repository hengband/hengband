/*!
 * @brief モンスターがプレーヤーへ攻撃する処理に関するユーティリティ
 * @date 2020/05/30
 * @author Hourier
 */

#include "monster-attack/monster-attack-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

monap_type *initialize_monap_type(player_type *target_ptr, monap_type *monap_ptr, MONSTER_IDX m_idx)
{
#ifdef JP
    monap_ptr->abbreviate = 0;
#endif
    monap_ptr->m_idx = m_idx;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monap_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    monap_ptr->act = nullptr;
    monap_ptr->touched = false;
    monap_ptr->explode = false;
    monap_ptr->do_silly_attack = one_in_(2) && target_ptr->image;
    monap_ptr->obvious = false;
    monap_ptr->get_damage = 0;
    monap_ptr->alive = true;
    monap_ptr->fear = false;
    return monap_ptr;
}
