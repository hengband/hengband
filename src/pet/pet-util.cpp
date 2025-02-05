#include "pet/pet-util.h"
#include "core/stuff-handler.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
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
bool can_player_ride_pet(PlayerType *player_ptr, const Grid &grid, bool now_riding)
{
    auto &world = AngbandWorld::get_instance();
    const auto old_character_xtra = world.character_xtra;
    const auto old_riding = player_ptr->riding;
    const auto old_riding_two_hands = player_ptr->riding_ryoute;
    const auto old_old_riding_two_hands = player_ptr->old_riding_ryoute;
    const auto old_pf_two_hands = any_bits(player_ptr->pet_extra_flags, PF_TWO_HANDS);
    world.character_xtra = true;

    if (now_riding) {
        player_ptr->ride_monster(grid.m_idx);
    } else {
        player_ptr->ride_monster(0);
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);

    bool p_can_enter = player_can_enter(player_ptr, grid.feat, CEM_P_CAN_ENTER_PATTERN);
    player_ptr->ride_monster(old_riding);
    if (old_pf_two_hands) {
        player_ptr->pet_extra_flags |= (PF_TWO_HANDS);
    } else {
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
    }

    player_ptr->riding_ryoute = old_riding_two_hands;
    player_ptr->old_riding_ryoute = old_old_riding_two_hands;
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);

    world.character_xtra = old_character_xtra;
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
        auto *r_ptr = &m_ptr->get_monrace();

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

        if (m_ptr->is_riding()) {
            total_friend_levels += (r_ptr->level + 5) * 2;
        } else if (!has_a_unique && m_ptr->get_monrace().misc_flags.has(MonsterMiscType::RIDING)) {
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
