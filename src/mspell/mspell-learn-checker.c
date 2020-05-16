#include "system/angband.h"
#include "mspell-learn-checker.h"
#include "world/world.h"

/*!
* @brief モンスターの唱えた呪文を青魔法で学習できるか判定する /
* @param target_ptr プレーヤーへの参照ポインタ
* @param m_idx モンスターID
* @return プレイヤーが青魔法で学習できるならTRUE、そうでなければFALSEを返す。
*/
bool spell_learnable(player_type* target_ptr, MONSTER_IDX m_idx)
{
    monster_type* m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    /* Extract the "see-able-ness" */
    bool seen = (!target_ptr->blind && m_ptr->ml);

    bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
    return (seen && maneable && !current_world_ptr->timewalk_m_idx);
}
