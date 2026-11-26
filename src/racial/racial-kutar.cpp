#include "racial/racial-kutar.h"
#include "core/disturbance.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 横に伸びるの継続時間をセットする / Set "expand", notice observable changes
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

    const auto effects = player_ptr->effects();
    if (v) {
        if (effects->expand().is_active() && !do_dec) {
            if (effects->expand().current() > v) {
                return false;
            }
        } else if (!effects->expand().is_active()) {
            msg_print(_("横に伸びた。", "Your body expands horizontally."));
            notice = true;
        }
    } else {
        if (effects->expand().is_active()) {
            msg_print(_("もう横に伸びていない。", "Your body returns to normal."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    effects->expand().set(v);
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    dur.set_flag(StatusRecalculatingFlag::BONUS);
    finish_timed_effect_change(player_ptr);
    return true;
}
