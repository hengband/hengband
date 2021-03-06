#include "mind/mind-magic-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "view/display-messages.h"

/*!
 * @brief 連奇術師の耐魔法防御 / 鏡使いの水鏡の盾 の継続時間をセットする / Set "resist_magic", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_resist_magic(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->resist_magic && !do_dec) {
            if (creature_ptr->resist_magic > v)
                return FALSE;
        } else if (!creature_ptr->resist_magic) {
            msg_print(_("魔法への耐性がついた。", "You have been protected from magic!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->resist_magic) {
            msg_print(_("魔法に弱くなった。", "You are no longer protected from magic."));
            notice = TRUE;
        }
    }

    creature_ptr->resist_magic = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}
