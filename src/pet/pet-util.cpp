#include "pet/pet-util.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "monster/monster-info.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの騎乗/下馬処理判定
 * @param g_ptr プレイヤーの移動先マスの構造体参照ポインタ
 * @param now_riding TRUEなら下馬処理、FALSEならば騎乗処理
 * @return 可能ならばTRUEを返す
 */
bool can_player_ride_pet(player_type *creature_ptr, grid_type *g_ptr, bool now_riding)
{
    bool old_character_xtra = current_world_ptr->character_xtra;
    MONSTER_IDX old_riding = creature_ptr->riding;
    bool old_riding_two_hands = creature_ptr->riding_ryoute;
    bool old_old_riding_two_hands = creature_ptr->old_riding_ryoute;
    bool old_pf_two_hands = (creature_ptr->pet_extra_flags & PF_TWO_HANDS) ? TRUE : FALSE;
    current_world_ptr->character_xtra = TRUE;

    if (now_riding)
        creature_ptr->riding = g_ptr->m_idx;
    else {
        creature_ptr->riding = 0;
        creature_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        creature_ptr->riding_ryoute = creature_ptr->old_riding_ryoute = FALSE;
    }

    creature_ptr->update |= PU_BONUS;
    handle_stuff(creature_ptr);

    bool p_can_enter = player_can_enter(creature_ptr, g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);
    creature_ptr->riding = old_riding;
    if (old_pf_two_hands)
        creature_ptr->pet_extra_flags |= (PF_TWO_HANDS);
    else
        creature_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);

    creature_ptr->riding_ryoute = old_riding_two_hands;
    creature_ptr->old_riding_ryoute = old_old_riding_two_hands;
    creature_ptr->update |= PU_BONUS;
    handle_stuff(creature_ptr);

    current_world_ptr->character_xtra = old_character_xtra;
    return p_can_enter;
}
