#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "player/player-status.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/baseitem-tracker.h"
#include "tracking/health-bar-tracker.h"

/*!
 * @brief 全更新処理をチェックして処理していく
 * @details 所持品の自動破壊・結合・並べ替えは行わない (handle_stuff_with_inventory_arrangement() で行う)。
 */
void handle_stuff(PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (rfu.any_stats()) {
        update_creature(player_ptr);
    }

    if (rfu.any_main()) {
        redraw_stuff(player_ptr);
    }

    if (rfu.any_sub()) {
        window_stuff(player_ptr);
    }
}

/*!
 * @brief 所持品の自動破壊・結合・並べ替えを行ってから、全更新処理を行う
 * @details メインループや店のループなど、コマンドの区切りにあたる位置で呼ぶ (update_inventory_arrangement() を参照)。
 */
void handle_stuff_with_inventory_arrangement(PlayerType *player_ptr)
{
    update_inventory_arrangement(player_ptr);
    handle_stuff(player_ptr);
}

/*
 * Track a new monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(PlayerType *player_ptr, short m_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    if (monster.is_riding()) {
        return;
    }

    HealthBarTracker::get_instance().set_trackee(m_idx);
}

bool update_player()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    return true;
}

bool redraw_player(PlayerType *player_ptr)
{
    if (player_ptr->csp > player_ptr->msp) {
        player_ptr->csp = player_ptr->msp;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    return true;
}
