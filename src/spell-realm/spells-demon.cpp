#include "spell-realm/spells-demon.h"
#include "core/disturbance.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的火炎のオーラの継続時間をセットする / Set "tim_sh_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_fire(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_sh_fire && !do_dec) {
            if (player_ptr->tim_sh_fire > v) {
                return false;
            }
        } else if (!player_ptr->tim_sh_fire) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_sh_fire) {
            msg_print(_("炎のオーラが消えた。", "The fiery aura disappeared."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    player_ptr->tim_sh_fire = v;
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    dur.set_flag(StatusRecalculatingFlag::BONUS);
    finish_timed_effect_change(player_ptr);
    return true;
}
