﻿/*!
 * @brief モンスターがプレーヤーへ攻撃する処理に関するユーティリティ
 * @date 2020/05/30
 * @author Hourier
 */

#include "monster-attack/monster-attack-util.h"
#include "system/floor-type-definition.h"

monap_type *initialize_monap_type(player_type *target_ptr, monap_type *monap_ptr, MONSTER_IDX m_idx)
{
#ifdef JP
    monap_ptr->abbreviate = 0;
#endif
    monap_ptr->m_idx = m_idx;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monap_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    monap_ptr->act = NULL;
    monap_ptr->touched = FALSE;
    monap_ptr->explode = FALSE;
    monap_ptr->do_silly_attack = one_in_(2) && target_ptr->image;
    monap_ptr->obvious = FALSE;
    monap_ptr->get_damage = 0;
    monap_ptr->alive = TRUE;
    monap_ptr->fear = FALSE;
    return monap_ptr;
}