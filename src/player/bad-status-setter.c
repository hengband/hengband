#include "player/bad-status-setter.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "mind/mind-sniper.h"
#include "player/avatar.h"
#include "player/player-effects.h" // つよし状態、後で依存を消す.
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "view/display-messages.h"

/*!
 * @brief 盲目の継続時間をセットする / Set "blind", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the use of "PU_UN_LITE" and "PU_UN_VIEW", which is needed to\n
 * memorize any terrain features which suddenly become "visible".\n
 * Note that blindness is currently the only thing which can affect\n
 * "player_can_see_bold()".\n
 */
bool set_blind(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (!creature_ptr->blind) {
            if (creature_ptr->prace == RACE_ANDROID) {
                msg_print(_("センサーをやられた！", "You are blind!"));
            } else {
                msg_print(_("目が見えなくなってしまった！", "You are blind!"));
            }

            notice = TRUE;
            chg_virtue(creature_ptr, V_ENLIGHTEN, -1);
        }
    }

    else {
        if (creature_ptr->blind) {
            if (creature_ptr->prace == RACE_ANDROID) {
                msg_print(_("センサーが復旧した。", "You can see again."));
            } else {
                msg_print(_("やっと目が見えるようになった。", "You can see again."));
            }

            notice = TRUE;
        }
    }

    creature_ptr->blind = v;
    creature_ptr->redraw |= (PR_STATUS);
    if (!notice)
        return FALSE;
    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);

    creature_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS | PU_MON_LITE);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 混乱の継続時間をセットする / Set "confused", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_confused(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (!creature_ptr->confused) {
            msg_print(_("あなたは混乱した！", "You are confused!"));

            if (creature_ptr->action == ACTION_LEARN) {
                msg_print(_("学習が続けられない！", "You cannot continue learning!"));
                creature_ptr->new_mane = FALSE;

                creature_ptr->redraw |= (PR_STATE);
                creature_ptr->action = ACTION_NONE;
            }
            if (creature_ptr->action == ACTION_KAMAE) {
                msg_print(_("構えがとけた。", "You lose your stance."));
                creature_ptr->special_defense &= ~(KAMAE_MASK);
                creature_ptr->update |= (PU_BONUS);
                creature_ptr->redraw |= (PR_STATE);
                creature_ptr->action = ACTION_NONE;
            } else if (creature_ptr->action == ACTION_KATA) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                creature_ptr->special_defense &= ~(KATA_MASK);
                creature_ptr->update |= (PU_BONUS);
                creature_ptr->update |= (PU_MONSTERS);
                creature_ptr->redraw |= (PR_STATE);
                creature_ptr->redraw |= (PR_STATUS);
                creature_ptr->action = ACTION_NONE;
            }

            /* Sniper */
            if (creature_ptr->concent)
                reset_concentration(creature_ptr, TRUE);

            if (hex_spelling_any(creature_ptr))
                stop_hex_spell_all(creature_ptr);

            notice = TRUE;
            creature_ptr->counter = FALSE;
            chg_virtue(creature_ptr, V_HARMONY, -1);
        }
    } else {
        if (creature_ptr->confused) {
            msg_print(_("やっと混乱がおさまった。", "You feel less confused now."));
            creature_ptr->special_attack &= ~(ATTACK_SUIKEN);
            notice = TRUE;
        }
    }

    creature_ptr->confused = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 毒の継続時間をセットする / Set "poisoned", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_poisoned(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (!creature_ptr->poisoned) {
            msg_print(_("毒に侵されてしまった！", "You are poisoned!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->poisoned) {
            msg_print(_("やっと毒の痛みがなくなった。", "You are no longer poisoned."));
            notice = TRUE;
        }
    }

    creature_ptr->poisoned = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 恐怖の継続時間をセットする / Set "afraid", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_afraid(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (!creature_ptr->afraid) {
            msg_print(_("何もかも恐くなってきた！", "You are terrified!"));

            if (creature_ptr->special_defense & KATA_MASK) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                creature_ptr->special_defense &= ~(KATA_MASK);
                creature_ptr->update |= (PU_BONUS);
                creature_ptr->update |= (PU_MONSTERS);
                creature_ptr->redraw |= (PR_STATE);
                creature_ptr->redraw |= (PR_STATUS);
                creature_ptr->action = ACTION_NONE;
            }

            notice = TRUE;
            creature_ptr->counter = FALSE;
            chg_virtue(creature_ptr, V_VALOUR, -1);
        }
    } else {
        if (creature_ptr->afraid) {
            msg_print(_("やっと恐怖を振り払った。", "You feel bolder now."));
            notice = TRUE;
        }
    }

    creature_ptr->afraid = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 麻痺の継続時間をセットする / Set "paralyzed", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_paralyzed(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (!creature_ptr->paralyzed) {
            msg_print(_("体が麻痺してしまった！", "You are paralyzed!"));
            if (creature_ptr->concent)
                reset_concentration(creature_ptr, TRUE);
            if (hex_spelling_any(creature_ptr))
                stop_hex_spell_all(creature_ptr);

            creature_ptr->counter = FALSE;
            notice = TRUE;
        }
    } else {
        if (creature_ptr->paralyzed) {
            msg_print(_("やっと動けるようになった。", "You can move again."));
            notice = TRUE;
        }
    }

    creature_ptr->paralyzed = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->redraw |= (PR_STATE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 幻覚の継続時間をセットする / Set "image", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details Note that we must redraw the map when hallucination changes.
 */
bool set_image(player_type *creature_ptr, TIME_EFFECT v)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;
    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
        v = 0;

    if (v) {
        set_tsuyoshi(creature_ptr, 0, TRUE);
        if (!creature_ptr->image) {
            msg_print(_("ワーオ！何もかも虹色に見える！", "Oh, wow! Everything looks so cosmic now!"));

            /* Sniper */
            if (creature_ptr->concent)
                reset_concentration(creature_ptr, TRUE);

            creature_ptr->counter = FALSE;
            notice = TRUE;
        }
    } else {
        if (creature_ptr->image) {
            msg_print(_("やっとはっきりと物が見えるようになった。", "You can see clearly again."));
            notice = TRUE;
        }
    }

    creature_ptr->image = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, TRUE);

    creature_ptr->redraw |= (PR_MAP | PR_HEALTH | PR_UHEALTH);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 減速の継続時間をセットする / Set "slow", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_slow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->slow && !do_dec) {
            if (creature_ptr->slow > v)
                return FALSE;
        } else if (!creature_ptr->slow) {
            msg_print(_("体の動きが遅くなってしまった！", "You feel yourself moving slower!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->slow) {
            msg_print(_("動きの遅さがなくなったようだ。", "You feel yourself speed up."));
            notice = TRUE;
        }
    }

    creature_ptr->slow = v;
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}
