﻿#include "spell-realm/spells-demon.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的火炎のオーラの継続時間をセットする / Set "tim_sh_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->tim_sh_fire && !do_dec) {
            if (creature_ptr->tim_sh_fire > v)
                return false;
        } else if (!creature_ptr->tim_sh_fire) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
            notice = true;
        }
    } else {
        if (creature_ptr->tim_sh_fire) {
            msg_print(_("炎のオーラが消えた。", "The fiery aura disappeared."));
            notice = true;
        }
    }

    creature_ptr->tim_sh_fire = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}
