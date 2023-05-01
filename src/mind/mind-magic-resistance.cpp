#include "mind/mind-magic-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 連奇術師の耐魔法防御 / 鏡使いの水鏡の盾 の継続時間をセットする / Set "resist_magic", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_resist_magic(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->resist_magic && !do_dec) {
            if (player_ptr->resist_magic > v) {
                return false;
            }
        } else if (!player_ptr->resist_magic) {
            msg_print(_("魔法への耐性がついた。", "You have been protected from magic!"));
            notice = true;
        }
    } else {
        if (player_ptr->resist_magic) {
            msg_print(_("魔法に弱くなった。", "You are no longer protected from magic."));
            notice = true;
        }
    }

    player_ptr->resist_magic = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}
