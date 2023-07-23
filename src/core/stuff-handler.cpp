#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"

/*!
 * @brief 全更新処理をチェックして処理していく
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

/*
 * Track the given monster race
 */
void monster_race_track(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    player_ptr->monster_race_idx = r_idx;
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
}

/*
 * Track the given object kind
 */
void object_kind_track(PlayerType *player_ptr, short bi_id)
{
    player_ptr->tracking_bi_id = bi_id;
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
}

/*
 * Track a new monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx && m_idx == player_ptr->riding) {
        return;
    }

    player_ptr->health_who = m_idx;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::HEALTH);
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
