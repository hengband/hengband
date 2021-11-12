#include "status/temporary-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的浮遊の継続時間をセットする / Set "tim_levitation", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_levitation(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_levitation && !do_dec) {
            if (player_ptr->tim_levitation > v)
                return false;
        } else if (!player_ptr->tim_levitation) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_levitation) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = true;
        }
    }

    player_ptr->tim_levitation = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_ultimate_res(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->ult_res && !do_dec) {
            if (player_ptr->ult_res > v)
                return false;
        } else if (!player_ptr->ult_res) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = true;
        }
    }

    else {
        if (player_ptr->ult_res) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = true;
        }
    }

    player_ptr->ult_res = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);

    return true;
}

bool set_tim_res_nether(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_res_nether && !do_dec) {
            if (player_ptr->tim_res_nether > v)
                return false;
        } else if (!player_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether-resistant!"));
            notice = true;
        }
    }

    else {
        if (player_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_nether = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_time(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_res_time && !do_dec) {
            if (player_ptr->tim_res_time > v)
                return false;
        } else if (!player_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_time = v;
    player_ptr->redraw |= (PR_STATUS);
    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}
