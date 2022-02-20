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
#include "player-info/bluemage-data-type.h"
#include "player-info/monk-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include <algorithm>

BadStatusSetter::BadStatusSetter(PlayerType *player_ptr)
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
bool BadStatusSetter::blindness(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }
    
    PlayerRace pr(player_ptr);    
    if (v > 0) {
        if (!this->player_ptr->blind) {
            if (pr.equals(PlayerRaceType::ANDROID)) {
                msg_print(_("センサーをやられた！", "The sensor broke!"));
            } else {
                msg_print(_("目が見えなくなってしまった！", "You are blind!"));
            }

            notice = true;
            chg_virtue(this->player_ptr, V_ENLIGHTEN, -1);
        }
    } else {
        if (this->player_ptr->blind) {
            if (pr.equals(PlayerRaceType::ANDROID)) {
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

bool BadStatusSetter::mod_blindness(const TIME_EFFECT tmp_v)
{
    return this->blindness(this->player_ptr->blind + tmp_v);
}

/*!
 * @brief 混乱の継続時間をセットする / Set "confused", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::confusion(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v > 0) {
        if (!this->player_ptr->confused) {
            msg_print(_("あなたは混乱した！", "You are confused!"));

            if (this->player_ptr->action == ACTION_LEARN) {
                msg_print(_("学習が続けられない！", "You cannot continue learning!"));
                auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
                bluemage_data->new_magic_learned = false;

                this->player_ptr->redraw |= PR_STATE;
                this->player_ptr->action = ACTION_NONE;
            }
            if (this->player_ptr->action == ACTION_MONK_STANCE) {
                msg_print(_("構えがとけた。", "You lose your stance."));
                PlayerClass(player_ptr).set_monk_stance(MonkStanceType::NONE);
                this->player_ptr->update |= PU_BONUS;
                this->player_ptr->redraw |= PR_STATE;
                this->player_ptr->action = ACTION_NONE;
            } else if (this->player_ptr->action == ACTION_SAMURAI_STANCE) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                PlayerClass(player_ptr).lose_balance();
            }

            /* Sniper */
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

bool BadStatusSetter::mod_confusion(const TIME_EFFECT tmp_v)
{
    return this->confusion(this->player_ptr->confused + tmp_v);
}

/*!
 * @brief 毒の継続時間をセットする / Set "poisoned", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::poison(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v > 0) {
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

bool BadStatusSetter::mod_poison(const TIME_EFFECT tmp_v)
{
    return this->poison(this->player_ptr->poisoned + tmp_v);
}

/*!
 * @brief 恐怖の継続時間をセットする / Set "afraid", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::afraidness(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v > 0) {
        if (!this->player_ptr->afraid) {
            msg_print(_("何もかも恐くなってきた！", "You are terrified!"));
            if (PlayerClass(this->player_ptr).lose_balance()) {
                msg_print(_("型が崩れた。", "You lose your stance."));
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

bool BadStatusSetter::mod_afraidness(const TIME_EFFECT tmp_v)
{
    return this->afraidness(this->player_ptr->afraid + tmp_v);
}

/*!
 * @brief 麻痺の継続時間をセットする / Set "paralyzed", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::paralysis(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v > 0) {
        if (!this->player_ptr->paralyzed) {
            msg_print(_("体が麻痺してしまった！", "You are paralyzed!"));
            reset_concentration(this->player_ptr, true);

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

bool BadStatusSetter::mod_paralysis(const TIME_EFFECT tmp_v)
{
    return this->paralysis(this->player_ptr->paralyzed + tmp_v);
}

/*!
 * @brief 幻覚の継続時間をセットする / Set "image", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details Note that we must redraw the map when hallucination changes.
 */
bool BadStatusSetter::hallucination(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (is_chargeman(this->player_ptr)) {
        v = 0;
    }

    if (v > 0) {
        set_tsuyoshi(this->player_ptr, 0, true);
        if (!this->player_ptr->hallucinated) {
            msg_print(_("ワーオ！何もかも虹色に見える！", "Oh, wow! Everything looks so cosmic now!"));
            reset_concentration(this->player_ptr, true);

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

bool BadStatusSetter::mod_hallucination(const TIME_EFFECT tmp_v)
{
    return this->hallucination(this->player_ptr->hallucinated + tmp_v);
}

/*!
 * @brief 減速の継続時間をセットする / Set "slow", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::slowness(const TIME_EFFECT tmp_v, bool do_dec)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (v > 0) {
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

bool BadStatusSetter::mod_slowness(const TIME_EFFECT tmp_v, bool do_dec)
{
    return this->slowness(this->player_ptr->slow + tmp_v, do_dec);
}

/*!
 * @brief 朦朧の継続時間をセットする / Set "stun", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool BadStatusSetter::stun(const TIME_EFFECT tmp_v)
{
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (PlayerRace(this->player_ptr).has_stun_immunity() || PlayerClass(this->player_ptr).has_stun_immunity()) {
        v = 0;
    }

    auto notice = this->process_stun_effect(v);
    this->player_ptr->effects()->stun()->set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    this->player_ptr->update |= PU_BONUS;
    this->player_ptr->redraw |= PR_STUN;
    handle_stuff(this->player_ptr);
    return true;
}

bool BadStatusSetter::mod_stun(const TIME_EFFECT tmp_v)
{
    return this->stun(this->player_ptr->effects()->stun()->current() + tmp_v);
}

/*!
 * @brief 出血の継続時間をセットする / Set "cut", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool BadStatusSetter::cut(const TIME_EFFECT tmp_v)
{
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->player_ptr->is_dead) {
        return false;
    }

    if (PlayerRace(this->player_ptr).has_cut_immunity()) {
        v = 0;
    }

    auto notice = this->process_cut_effect(v);
    this->player_ptr->effects()->cut()->set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }

    this->player_ptr->update |= PU_BONUS;
    this->player_ptr->redraw |= PR_CUT;
    handle_stuff(this->player_ptr);
    return true;
}

bool BadStatusSetter::mod_cut(const TIME_EFFECT tmp_v)
{
    return this->cut(this->player_ptr->effects()->cut()->current() + tmp_v);
}

bool BadStatusSetter::process_stun_effect(const short v)
{
    auto old_rank = this->player_ptr->effects()->stun()->get_rank();
    auto new_rank = PlayerStun::get_rank(v);
    if (new_rank > old_rank) {
        this->process_stun_status(new_rank, v);
        return true;
    }
    
    if (new_rank < old_rank) {
        this->clear_head();
        return true;
    }

    return false;
}

void BadStatusSetter::process_stun_status(const PlayerStunRank new_rank, const short v)
{
    auto stun_mes = PlayerStun::get_stun_mes(new_rank);
    msg_print(stun_mes.data());
    this->decrease_int_wis(v);
    if (PlayerClass(this->player_ptr).lose_balance()) {
        msg_print(_("型が崩れた。", "You lose your stance."));
    }

    reset_concentration(this->player_ptr, true);

    SpellHex spell_hex(this->player_ptr);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }
}

void BadStatusSetter::clear_head()
{
    if (this->player_ptr->effects()->stun()->is_stunned()) {
        return;
    }

    msg_print(_("やっと朦朧状態から回復した。", "You are no longer stunned."));
    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }
}

/*!
 * @todo 後で知能と賢さが両方減る確率を減らす.
 */
void BadStatusSetter::decrease_int_wis(const short v)
{
    if ((v <= randint1(1000)) && !one_in_(16)) {
        return;
    }

    msg_print(_("割れるような頭痛がする。", "A vicious blow hits your head."));
    auto rand = randint0(5);
    switch (rand) {
    case 0:
        if (has_sustain_int(this->player_ptr) == 0) {
            (void)do_dec_stat(this->player_ptr, A_INT);
        }

        if (has_sustain_wis(this->player_ptr) == 0) {
            (void)do_dec_stat(this->player_ptr, A_WIS);
        }

        return;
    case 1:
    case 2:
        if (has_sustain_int(this->player_ptr) == 0) {
            (void)do_dec_stat(this->player_ptr, A_INT);
        }
        
        return;
    case 3:
    case 4:
        if (has_sustain_wis(this->player_ptr) == 0) {
            (void)do_dec_stat(this->player_ptr, A_WIS);
        }

        return;
    default:
        throw("Invalid random number is specified!");
    }
}

bool BadStatusSetter::process_cut_effect(const short v)
{
    auto player_cut = this->player_ptr->effects()->cut();
    auto old_rank = player_cut->get_rank();
    auto new_rank = player_cut->get_rank(v);
    if (new_rank > old_rank) {
        this->decrease_charisma(new_rank, v);
        return true;
    }

    if (new_rank < old_rank) {
        this->stop_blooding(new_rank);
        return true;
    }

    return false;
}

void BadStatusSetter::decrease_charisma(const PlayerCutRank new_rank, const short v)
{
    auto player_cut = this->player_ptr->effects()->cut();
    auto cut_mes = player_cut->get_cut_mes(new_rank);
    msg_print(cut_mes.data());
    if (v <= randint1(1000) && !one_in_(16)) {
        return;
    }

    if (has_sustain_chr(this->player_ptr)) {
        return;
    }

    msg_print(_("ひどい傷跡が残ってしまった。", "You have been horribly scarred."));
    do_dec_stat(this->player_ptr, A_CHR);
}

void BadStatusSetter::stop_blooding(const PlayerCutRank new_rank)
{
    if (new_rank >= PlayerCutRank::GRAZING) {
        return;
    }

    auto blood_stop_mes = PlayerRace(this->player_ptr).equals(PlayerRaceType::ANDROID)
        ? _("怪我が直った", "leaking fluid")
        : _("出血が止まった", "bleeding");
    msg_format(_("やっと%s。", "You are no longer %s."), blood_stop_mes);
    if (disturb_state) {
        disturb(this->player_ptr, false, false);
    }
}
