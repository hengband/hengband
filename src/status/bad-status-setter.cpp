#include "status/bad-status-setter.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "mind/mind-sniper.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

BadStatusSetter::BadStatusSetter(player_type* player_ptr)
    : player_ptr(player_ptr)
{
}

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
bool BadStatusSetter::blindness(TIME_EFFECT v)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (!this->player_ptr->blind) {
            if (this->player_ptr->prace == player_race_type::ANDROID) {
                msg_print(_("センサーをやられた！", "The sensor broke!"));
            } else {
                msg_print(_("目が見えなくなってしまった！", "You are blind!"));
            }

            notice = true;
            chg_virtue(this->player_ptr, V_ENLIGHTEN, -1);
        }
    } else {
        if (this->player_ptr->blind) {
            if (this->player_ptr->prace == player_race_type::ANDROID) {
                msg_print(_("センサーが復旧した。", "The sensor has been restored."));
            } else {
                msg_print(_("やっと目が見えるようになった。", "You can see again."));
            }

            notice = true;
        }
    }

    this->player_ptr->blind = v;
    this->player_ptr->redraw |= PR_STATUS;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    this->player_ptr->update |= PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS | PU_MON_LITE;
    this->player_ptr->redraw |= PR_MAP;
    this->player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 混乱の継続時間をセットする / Set "confused", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::confusion(TIME_EFFECT v)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (!this->player_ptr->confused) {
            msg_print(_("あなたは混乱した！", "You are confused!"));

            if (this->player_ptr->action == ACTION_LEARN) {
                msg_print(_("学習が続けられない！", "You cannot continue learning!"));
                this->player_ptr->new_mane = false;

                this->player_ptr->redraw |= (PR_STATE);
                this->player_ptr->action = ACTION_NONE;
            }
            if (this->player_ptr->action == ACTION_KAMAE) {
                msg_print(_("構えがとけた。", "You lose your stance."));
                this->player_ptr->special_defense &= ~(KAMAE_MASK);
                this->player_ptr->update |= (PU_BONUS);
                this->player_ptr->redraw |= (PR_STATE);
                this->player_ptr->action = ACTION_NONE;
            } else if (this->player_ptr->action == ACTION_KATA) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                this->player_ptr->special_defense &= ~(KATA_MASK);
                this->player_ptr->update |= (PU_BONUS);
                this->player_ptr->update |= (PU_MONSTERS);
                this->player_ptr->redraw |= (PR_STATE);
                this->player_ptr->redraw |= (PR_STATUS);
                this->player_ptr->action = ACTION_NONE;
            }

            /* Sniper */
            if (this->player_ptr->concent)
                reset_concentration(this->player_ptr, true);

            SpellHex spell_hex(this->player_ptr);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            notice = true;
            this->player_ptr->counter = false;
            chg_virtue(this->player_ptr, V_HARMONY, -1);
        }
    } else {
        if (this->player_ptr->confused) {
            msg_print(_("やっと混乱がおさまった。", "You feel less confused now."));
            this->player_ptr->special_attack &= ~(ATTACK_SUIKEN);
            notice = true;
        }
    }

    this->player_ptr->confused = v;
    this->player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(this->player_ptr, false, false);
    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 毒の継続時間をセットする / Set "poisoned", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::poison(TIME_EFFECT v)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead)
        return false;

    if (v) {
        if (!this->player_ptr->poisoned) {
            msg_print(_("毒に侵されてしまった！", "You are poisoned!"));
            notice = true;
        }
    } else {
        if (this->player_ptr->poisoned) {
            msg_print(_("やっと毒の痛みがなくなった。", "You are no longer poisoned."));
            notice = true;
        }
    }

    this->player_ptr->poisoned = v;
    this->player_ptr->redraw |= (PR_STATUS);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 恐怖の継続時間をセットする / Set "afraid", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::afraidness(TIME_EFFECT v)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (!this->player_ptr->afraid) {
            msg_print(_("何もかも恐くなってきた！", "You are terrified!"));
            if (this->player_ptr->special_defense & KATA_MASK) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                this->player_ptr->special_defense &= ~(KATA_MASK);
                this->player_ptr->update |= (PU_BONUS);
                this->player_ptr->update |= (PU_MONSTERS);
                this->player_ptr->redraw |= (PR_STATE);
                this->player_ptr->redraw |= (PR_STATUS);
                this->player_ptr->action = ACTION_NONE;
            }

            notice = true;
            this->player_ptr->counter = false;
            chg_virtue(this->player_ptr, V_VALOUR, -1);
        }
    } else {
        if (this->player_ptr->afraid) {
            msg_print(_("やっと恐怖を振り払った。", "You feel bolder now."));
            notice = true;
        }
    }

    this->player_ptr->afraid = v;
    this->player_ptr->redraw |= PR_STATUS;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 麻痺の継続時間をセットする / Set "paralyzed", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::paralysis(TIME_EFFECT v)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (!this->player_ptr->paralyzed) {
            msg_print(_("体が麻痺してしまった！", "You are paralyzed!"));
            if (this->player_ptr->concent) {
                reset_concentration(this->player_ptr, true);
            }

            SpellHex spell_hex(this->player_ptr);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            this->player_ptr->counter = false;
            notice = true;
        }
    } else {
        if (this->player_ptr->paralyzed) {
            msg_print(_("やっと動けるようになった。", "You can move again."));
            notice = true;
        }
    }

    this->player_ptr->paralyzed = v;
    this->player_ptr->redraw |= PR_STATUS;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    this->player_ptr->redraw |= PR_STATE;
    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 幻覚の継続時間をセットする / Set "image", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details Note that we must redraw the map when hallucination changes.
 */
bool BadStatusSetter::hallucination(TIME_EFFECT v)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (is_chargeman(this->player_ptr)) {
        v = 0;
    }

    if (v) {
        set_tsuyoshi(this->player_ptr, 0, true);
        if (!this->player_ptr->hallucinated) {
            msg_print(_("ワーオ！何もかも虹色に見える！", "Oh, wow! Everything looks so cosmic now!"));
            if (this->player_ptr->concent) {
                reset_concentration(this->player_ptr, true);
            }

            this->player_ptr->counter = false;
            notice = true;
        }
    } else {
        if (this->player_ptr->hallucinated) {
            msg_print(_("やっとはっきりと物が見えるようになった。", "You can see clearly again."));
            notice = true;
        }
    }

    this->player_ptr->hallucinated = v;
    this->player_ptr->redraw |= PR_STATUS;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, true);
    }

    this->player_ptr->redraw |= PR_MAP | PR_HEALTH | PR_UHEALTH;
    this->player_ptr->update |= PU_MONSTERS;
    this->player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 減速の継続時間をセットする / Set "slow", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::slowness(TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (this->player_ptr->slow && !do_dec) {
            if (this->player_ptr->slow > v) {
                return false;
            }
        } else if (!this->player_ptr->slow) {
            msg_print(_("体の動きが遅くなってしまった！", "You feel yourself moving slower!"));
            notice = true;
        }
    } else {
        if (this->player_ptr->slow) {
            msg_print(_("動きの遅さがなくなったようだ。", "You feel yourself speed up."));
            notice = true;
        }
    }

    this->player_ptr->slow = v;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    this->player_ptr->update |= PU_BONUS;
    handle_stuff(this->player_ptr);
    return true;
}

/*!
 * @brief 朦朧の継続時間をセットする / Set "stun", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool set_stun(player_type *player_ptr, TIME_EFFECT v)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;
    if (PlayerRace(player_ptr).equals(player_race_type::GOLEM) || PlayerClass(player_ptr).can_resist_stun())
        v = 0;

    auto player_stun = player_ptr->effects()->stun();
    auto old_aux = player_stun->get_rank();
    auto new_aux = PlayerStun::get_rank(v);
    if (new_aux > old_aux) {
        auto stun_mes = PlayerStun::get_stun_mes(new_aux);
        msg_print(stun_mes.data());
        if (randint1(1000) < v || one_in_(16)) {
            msg_print(_("割れるような頭痛がする。", "A vicious blow hits your head."));

            if (one_in_(3)) {
                if (!has_sustain_int(player_ptr))
                    (void)do_dec_stat(player_ptr, A_INT);
                if (!has_sustain_wis(player_ptr))
                    (void)do_dec_stat(player_ptr, A_WIS);
            } else if (one_in_(2)) {
                if (!has_sustain_int(player_ptr))
                    (void)do_dec_stat(player_ptr, A_INT);
            } else {
                if (!has_sustain_wis(player_ptr))
                    (void)do_dec_stat(player_ptr, A_WIS);
            }
        }

        if (player_ptr->special_defense & KATA_MASK) {
            msg_print(_("型が崩れた。", "You lose your stance."));
            player_ptr->special_defense &= ~(KATA_MASK);
            player_ptr->update |= (PU_BONUS);
            player_ptr->update |= (PU_MONSTERS);
            player_ptr->redraw |= (PR_STATE);
            player_ptr->redraw |= (PR_STATUS);
            player_ptr->action = ACTION_NONE;
        }

        if (player_ptr->concent)
            reset_concentration(player_ptr, true);

        SpellHex spell_hex(player_ptr);
        if (spell_hex.is_spelling_any()) {
            (void)spell_hex.stop_all_spells();
        }

        notice = true;
    } else if (new_aux < old_aux) {
        if (new_aux == PlayerStunRank::NONE) {
            msg_print(_("やっと朦朧状態から回復した。", "You are no longer stunned."));
            if (disturb_state)
                disturb(player_ptr, false, false);
        }

        notice = true;
    }

    player_stun->set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    player_ptr->update |= PU_BONUS;
    player_ptr->redraw |= PR_STUN;
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 出血の継続時間をセットする / Set "cut", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool set_cut(player_type *player_ptr, TIME_EFFECT v)
{
    int old_aux, new_aux;
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;

    if ((player_ptr->prace == player_race_type::GOLEM || player_ptr->prace == player_race_type::SKELETON || player_ptr->prace == player_race_type::SPECTRE
            || (player_ptr->prace == player_race_type::ZOMBIE && player_ptr->lev > 11))
        && !player_ptr->mimic_form)
        v = 0;

    if (player_ptr->cut > 1000) {
        old_aux = 7;
    } else if (player_ptr->cut > 200) {
        old_aux = 6;
    } else if (player_ptr->cut > 100) {
        old_aux = 5;
    } else if (player_ptr->cut > 50) {
        old_aux = 4;
    } else if (player_ptr->cut > 25) {
        old_aux = 3;
    } else if (player_ptr->cut > 10) {
        old_aux = 2;
    } else if (player_ptr->cut > 0) {
        old_aux = 1;
    } else {
        old_aux = 0;
    }

    if (v > 1000) {
        new_aux = 7;
    } else if (v > 200) {
        new_aux = 6;
    } else if (v > 100) {
        new_aux = 5;
    } else if (v > 50) {
        new_aux = 4;
    } else if (v > 25) {
        new_aux = 3;
    } else if (v > 10) {
        new_aux = 2;
    } else if (v > 0) {
        new_aux = 1;
    } else {
        new_aux = 0;
    }

    if (new_aux > old_aux) {
        switch (new_aux) {
        case 1:
            msg_print(_("かすり傷を負ってしまった。", "You have been given a graze."));
            break;
        case 2:
            msg_print(_("軽い傷を負ってしまった。", "You have been given a light cut."));
            break;
        case 3:
            msg_print(_("ひどい傷を負ってしまった。", "You have been given a bad cut."));
            break;
        case 4:
            msg_print(_("大変な傷を負ってしまった。", "You have been given a nasty cut."));
            break;
        case 5:
            msg_print(_("重大な傷を負ってしまった。", "You have been given a severe cut."));
            break;
        case 6:
            msg_print(_("ひどい深手を負ってしまった。", "You have been given a deep gash."));
            break;
        case 7:
            msg_print(_("致命的な傷を負ってしまった。", "You have been given a mortal wound."));
            break;
        }

        notice = true;
        if (randint1(1000) < v || one_in_(16)) {
            if (!has_sustain_chr(player_ptr)) {
                msg_print(_("ひどい傷跡が残ってしまった。", "You have been horribly scarred."));
                do_dec_stat(player_ptr, A_CHR);
            }
        }
    } else if (new_aux < old_aux) {
        if (new_aux == 0) {
            msg_format(_("やっと%s。", "You are no longer %s."),
                player_ptr->prace == player_race_type::ANDROID ? _("怪我が直った", "leaking fluid") : _("出血が止まった", "bleeding"));
            if (disturb_state)
                disturb(player_ptr, false, false);
        }

        notice = true;
    }

    player_ptr->cut = v;
    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    player_ptr->redraw |= (PR_CUT);
    handle_stuff(player_ptr);
    return true;
}
