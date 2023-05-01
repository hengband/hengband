#include "status/body-improvement.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 対邪悪結界の継続時間をセットする / Set "protevil", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_protevil(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->protevil && !do_dec) {
            if (player_ptr->protevil > v) {
                return false;
            }
        } else if (!player_ptr->protevil) {
            msg_print(_("邪悪なる存在から守られているような感じがする！", "You feel safe from evil!"));
            notice = true;
        }
    } else {
        if (player_ptr->protevil) {
            msg_print(_("邪悪なる存在から守られている感じがなくなった。", "You no longer feel safe from evil."));
            notice = true;
        }
    }

    player_ptr->protevil = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 無傷球の継続時間をセットする / Set "invuln", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_invuln(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->invuln && !do_dec) {
            if (player_ptr->invuln > v) {
                return false;
            }
        } else if (!is_invuln(player_ptr)) {
            msg_print(_("無敵だ！", "Invulnerability!"));
            notice = true;

            chg_virtue(player_ptr, Virtue::UNLIFE, -2);
            chg_virtue(player_ptr, Virtue::HONOUR, -2);
            chg_virtue(player_ptr, Virtue::SACRIFICE, -3);
            chg_virtue(player_ptr, Virtue::VALOUR, -5);

            player_ptr->redraw |= (PR_MAP);
            player_ptr->update |= (PU_MONSTER_STATUSES);

            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (player_ptr->invuln && !music_singing(player_ptr, MUSIC_INVULN)) {
            msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));
            notice = true;

            player_ptr->redraw |= (PR_MAP);
            player_ptr->update |= (PU_MONSTER_STATUSES);

            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

            player_ptr->energy_need += ENERGY_NEED();
        }
    }

    player_ptr->invuln = v;
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

/*!
 * @brief 時限急回復の継続時間をセットする / Set "tim_regen", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_regen(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_regen && !do_dec) {
            if (player_ptr->tim_regen > v) {
                return false;
            }
        } else if (!player_ptr->tim_regen) {
            msg_print(_("回復力が上がった！", "You feel yourself regenerating quickly!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_regen) {
            msg_print(_("素早く回復する感じがなくなった。", "You feel you are no longer regenerating quickly."));
            notice = true;
        }
    }

    player_ptr->tim_regen = v;
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

/*!
 * @brief 一時的反射の継続時間をセットする / Set "tim_reflect", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_reflect(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_reflect && !do_dec) {
            if (player_ptr->tim_reflect > v) {
                return false;
            }
        } else if (!player_ptr->tim_reflect) {
            msg_print(_("体の表面が滑かになった気がする。", "Your body becames smooth."));
            notice = true;
        }
    } else {
        if (player_ptr->tim_reflect) {
            msg_print(_("体の表面が滑かでなくなった。", "Your body is no longer smooth."));
            notice = true;
        }
    }

    player_ptr->tim_reflect = v;
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

/*!
 * @brief 一時的壁抜けの継続時間をセットする / Set "tim_pass_wall", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_pass_wall(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_pass_wall && !do_dec) {
            if (player_ptr->tim_pass_wall > v) {
                return false;
            }
        } else if (!player_ptr->tim_pass_wall) {
            msg_print(_("体が半物質の状態になった。", "You became ethereal."));
            notice = true;
        }
    } else {
        if (player_ptr->tim_pass_wall) {
            msg_print(_("体が物質化した。", "You are no longer ethereal."));
            notice = true;
        }
    }

    player_ptr->tim_pass_wall = v;
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
