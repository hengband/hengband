#include "status/sight-setter.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 時限ESPの継続時間をセットする / Set "tim_esp", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_esp(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_esp && !do_dec) {
            if (player_ptr->tim_esp > v) {
                return false;
            }
        } else if (!is_time_limit_esp(player_ptr)) {
            msg_print(_("意識が広がった気がする！", "You feel your consciousness expand!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_esp && !music_singing(player_ptr, MUSIC_MIND)) {
            msg_print(_("意識は元に戻った。", "Your consciousness contracts again."));
            notice = true;
        }
    }

    player_ptr->tim_esp = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_MONSTER_STATUSES);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 時限透明視の継続時間をセットする / Set "tim_invis", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_invis(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_invis && !do_dec) {
            if (player_ptr->tim_invis > v) {
                return false;
            }
        } else if (!player_ptr->tim_invis) {
            msg_print(_("目が非常に敏感になった気がする！", "Your eyes feel very sensitive!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_invis) {
            msg_print(_("目の敏感さがなくなったようだ。", "Your eyes feel less sensitive."));
            notice = true;
        }
    }

    player_ptr->tim_invis = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_MONSTER_STATUSES);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 時限赤外線視力の継続時間をセットする / Set "tim_infra", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_infra(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_infra && !do_dec) {
            if (player_ptr->tim_infra > v) {
                return false;
            }
        } else if (!player_ptr->tim_infra) {
            msg_print(_("目がランランと輝き始めた！", "Your eyes begin to tingle!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_infra) {
            msg_print(_("目の輝きがなくなった。", "Your eyes stop tingling."));
            notice = true;
        }
    }

    player_ptr->tim_infra = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_MONSTER_STATUSES);
    handle_stuff(player_ptr);
    return true;
}
