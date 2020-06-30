#include "status/form-changer.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "player/avatar.h"
#include "player/player-move.h"
#include "player/player-race.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "view/display-messages.h"

/*!
 * @brief 変身効果の継続時間と変身先をセットする / Set "tim_mimic", and "mimic_form", notice observable changes
 * @param v 継続時間
 * @param p 変身内容
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_mimic && (creature_ptr->mimic_form == p) && !do_dec) {
            if (creature_ptr->tim_mimic > v)
                return FALSE;
        } else if ((!creature_ptr->tim_mimic) || (creature_ptr->mimic_form != p)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            creature_ptr->mimic_form = p;
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->tim_mimic) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            if (creature_ptr->mimic_form == MIMIC_DEMON)
                set_oppose_fire(creature_ptr, 0, TRUE);
            creature_ptr->mimic_form = 0;
            notice = TRUE;
            p = 0;
        }
    }

    creature_ptr->tim_mimic = v;
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, TRUE);

    creature_ptr->redraw |= (PR_BASIC | PR_STATUS);
    creature_ptr->update |= (PU_BONUS | PU_HP);

    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 狂戦士化の継続時間をセットする / Set "shero", notice observable changes
 * @param v 継続時間/ 0ならば無条件にリセット
 * @param do_dec FALSEの場合現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (creature_ptr->pclass == CLASS_BERSERKER)
        v = 1;
    if (v) {
        if (creature_ptr->shero && !do_dec) {
            if (creature_ptr->shero > v)
                return FALSE;
        } else if (!creature_ptr->shero) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->shero) {
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = TRUE;
        }
    }

    creature_ptr->shero = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 幽体化の継続時間をセットする / Set "wraith_form", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->wraith_form && !do_dec) {
            if (creature_ptr->wraith_form > v)
                return FALSE;
        } else if (!creature_ptr->wraith_form) {
            msg_print(_("物質界を離れて幽鬼のような存在になった！", "You leave the physical world and turn into a wraith-being!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_UNLIFE, 3);
            chg_virtue(creature_ptr, V_HONOUR, -2);
            chg_virtue(creature_ptr, V_SACRIFICE, -2);
            chg_virtue(creature_ptr, V_VALOUR, -5);

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (creature_ptr->wraith_form) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            notice = TRUE;

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    creature_ptr->wraith_form = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief オクレ兄さんの継続時間をセットする / Set "tsuyoshi", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tsuyoshi && !do_dec) {
            if (creature_ptr->tsuyoshi > v)
                return FALSE;
        } else if (!creature_ptr->tsuyoshi) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_VITALITY, 2);
        }
    } else {
        if (creature_ptr->tsuyoshi) {
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(creature_ptr, A_CON, 20, TRUE);
            (void)dec_stat(creature_ptr, A_STR, 20, TRUE);

            notice = TRUE;
            chg_virtue(creature_ptr, V_VITALITY, -3);
        }
    }

    creature_ptr->tsuyoshi = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return TRUE;
}
