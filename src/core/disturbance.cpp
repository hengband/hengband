#include "core/disturbance.h"
#include "action/travel-execution.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "io/input-key-requester.h"
#include "player/attack-defense-types.h"
#include "status/action-setter.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"

/*!
 * @brief 走っていれば走るのをやめる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 走っている間は画面をプレイヤーに合わせない (center_running がオフのとき) ので、合わせ直す。
 * 足元・発見済みのアイテムの一覧も描き直していないので、描き直しを要求する
 */
void stop_running(PlayerType *player_ptr)
{
    if (!player_ptr->running) {
        return;
    }

    player_ptr->running = 0;
    if (center_player && !center_running) {
        verify_panel(player_ptr);
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::FLOW,
    };
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.set_flags(flags);
    dur.request_floor_item_lists_redrawing();
}

/*
 * Something has happened to disturb the player.
 * The first arg indicates a major disturbance, which affects search.
 * The second arg is currently unused, but could induce output flush.
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(PlayerType *player_ptr, bool stop_search, bool stop_travel)
{
    auto &dur = DeferredUpdateRequests::get_instance();
    if (command_rep) {
        command_rep = 0;
        dur.set_flag(MainWindowRedrawingFlag::ACTION);
    }

    if ((player_ptr->action == ACTION_REST) || (player_ptr->action == ACTION_FISH) || (stop_search && (player_ptr->action == ACTION_SEARCH))) {
        set_action(player_ptr, ACTION_NONE);
    }

    stop_running(player_ptr);

    if (stop_travel) {
        Travel::get_instance().stop();
        if (center_player && !center_running) {
            verify_panel(player_ptr);
        }

        dur.set_flag(StatusRecalculatingFlag::TORCH);
    }

    if (flush_disturb) {
        flush();
    }
}
