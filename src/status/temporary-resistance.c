#include "status/temporary-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的浮遊の継続時間をセットする / Set "tim_levitation", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_levitation(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_levitation && !do_dec) {
            if (creature_ptr->tim_levitation > v)
                return FALSE;
        } else if (!creature_ptr->tim_levitation) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_levitation) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_levitation = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

bool set_ultimate_res(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->ult_res && !do_dec) {
            if (creature_ptr->ult_res > v)
                return FALSE;
        } else if (!creature_ptr->ult_res) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->ult_res) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->ult_res = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return TRUE;
}

bool set_tim_res_nether(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_res_nether && !do_dec) {
            if (creature_ptr->tim_res_nether > v)
                return FALSE;
        } else if (!creature_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether resistant!"));
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->tim_res_nether = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

bool set_tim_res_time(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_res_time && !do_dec) {
            if (creature_ptr->tim_res_time > v)
                return FALSE;
        } else if (!creature_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time resistant!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->tim_res_time = v;
    creature_ptr->redraw |= (PR_STATUS);
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}
