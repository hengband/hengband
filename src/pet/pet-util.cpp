#include "pet/pet-util.h"
#include "core/stuff-handler.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

int total_friends = 0;

/*!
 * @brief プレイヤーの騎乗/下馬処理判定
 * @param g_ptr プレイヤーの移動先マスの構造体参照ポインタ
 * @param now_riding trueなら下馬処理、falseならば騎乗処理
 * @return 可能ならばtrueを返す
 */
bool can_player_ride_pet(PlayerType *player_ptr, grid_type *g_ptr, bool now_riding)
{
    bool old_character_xtra = w_ptr->character_xtra;
    MONSTER_IDX old_riding = player_ptr->riding;
    bool old_riding_two_hands = player_ptr->riding_ryoute;
    bool old_old_riding_two_hands = player_ptr->old_riding_ryoute;
    bool old_pf_two_hands = any_bits(player_ptr->pet_extra_flags, PF_TWO_HANDS);
    w_ptr->character_xtra = true;

    if (now_riding) {
        player_ptr->riding = g_ptr->m_idx;
    } else {
        player_ptr->riding = 0;
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);

    bool p_can_enter = player_can_enter(player_ptr, g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);
    player_ptr->riding = old_riding;
    if (old_pf_two_hands) {
        player_ptr->pet_extra_flags |= (PF_TWO_HANDS);
    } else {
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
    }

    player_ptr->riding_ryoute = old_riding_two_hands;
    player_ptr->old_riding_ryoute = old_old_riding_two_hands;
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);

    w_ptr->character_xtra = old_character_xtra;
    return p_can_enter;
}

/*!
 * @brief ペットの維持コスト計算
 * @return 維持コスト(%)
 */
PERCENTAGE calculate_upkeep(PlayerType *player_ptr)
{
    bool has_a_unique = false;
    DEPTH total_friend_levels = 0;
    total_friends = 0;
    for (auto m_idx = player_ptr->current_floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
        if (!m_ptr->is_valid()) {
            continue;
        }
        auto *r_ptr = &monraces_info[m_ptr->r_idx];

        if (!m_ptr->is_pet()) {
            continue;
        }

        total_friends++;
        if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            total_friend_levels += r_ptr->level;
            continue;
        }

        if (player_ptr->pclass != PlayerClassType::CAVALRY) {
            total_friend_levels += (r_ptr->level + 5) * 10;
            continue;
        }

        if (player_ptr->riding == m_idx) {
            total_friend_levels += (r_ptr->level + 5) * 2;
        } else if (!has_a_unique && any_bits(monraces_info[m_ptr->r_idx].flags7, RF7_RIDING)) {
            total_friend_levels += (r_ptr->level + 5) * 7 / 2;
        } else {
            total_friend_levels += (r_ptr->level + 5) * 10;
        }

        has_a_unique = true;
    }

    if (total_friends == 0) {
        return 0;
    }

    int upkeep_factor = (total_friend_levels - (player_ptr->lev * 80 / (cp_ptr->pet_upkeep_div)));
    if (upkeep_factor < 0) {
        upkeep_factor = 0;
    }

    if (upkeep_factor > 1000) {
        upkeep_factor = 1000;
    }

    return upkeep_factor;
}
