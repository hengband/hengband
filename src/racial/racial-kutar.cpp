#include "racial/racial-kutar.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief つぶれるの継続時間をセットする / Set "tsubureru", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_leveling(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tsubureru && !do_dec) {
            if (player_ptr->tsubureru > v) {
                return false;
            }
        } else if (!player_ptr->tsubureru) {
            msg_print(_("横に伸びた。", "Your body expands horizontally."));
            notice = true;
        }
    } else {
        if (player_ptr->tsubureru) {
            msg_print(_("もう横に伸びていない。", "Your body returns to normal."));
            notice = true;
        }
    }

    player_ptr->tsubureru = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}
