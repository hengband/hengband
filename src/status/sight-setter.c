#include "status/sight-setter.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "realm/realm-song-numbers.h"
#include "view/display-messages.h"

/*!
 * @brief 時限ESPの継続時間をセットする / Set "tim_esp", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_esp(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_esp && !do_dec) {
            if (creature_ptr->tim_esp > v)
                return FALSE;
        } else if (!is_time_limit_esp(creature_ptr)) {
            msg_print(_("意識が広がった気がする！", "You feel your consciousness expand!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_esp && !music_singing(creature_ptr, MUSIC_MIND)) {
            msg_print(_("意識は元に戻った。", "Your consciousness contracts again."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_esp = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_MONSTERS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 時限透明視の継続時間をセットする / Set "tim_invis", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_invis(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_invis && !do_dec) {
            if (creature_ptr->tim_invis > v)
                return FALSE;
        } else if (!creature_ptr->tim_invis) {
            msg_print(_("目が非常に敏感になった気がする！", "Your eyes feel very sensitive!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_invis) {
            msg_print(_("目の敏感さがなくなったようだ。", "Your eyes feel less sensitive."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_invis = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_MONSTERS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 時限赤外線視力の継続時間をセットする / Set "tim_infra", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_infra(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_infra && !do_dec) {
            if (creature_ptr->tim_infra > v)
                return FALSE;
        } else if (!creature_ptr->tim_infra) {
            msg_print(_("目がランランと輝き始めた！", "Your eyes begin to tingle!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_infra) {
            msg_print(_("目の輝きがなくなった。", "Your eyes stop tingling."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_infra = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_MONSTERS);
    handle_stuff(creature_ptr);
    return TRUE;
}
