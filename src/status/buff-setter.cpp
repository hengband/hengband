#include "status/buff-setter.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "monster/monster-status-setter.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-deceleration.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの全ての時限効果をリセットする。 / reset timed flags
 */
void reset_tim_flags(PlayerType *player_ptr)
{
    auto effects = player_ptr->effects();
    effects->acceleration()->reset();
    player_ptr->lightspeed = 0;
    effects->deceleration()->reset();
    effects->blindness()->reset();
    effects->paralysis()->reset();
    effects->confusion()->reset();
    effects->fear()->reset();
    effects->hallucination()->reset();
    effects->poison()->reset();
    effects->cut()->reset();
    effects->stun()->reset();

    player_ptr->protevil = 0; /* Timed -- Protection */
    player_ptr->invuln = 0; /* Timed -- Invulnerable */
    player_ptr->ult_res = 0;
    player_ptr->hero = 0; /* Timed -- Heroism */
    player_ptr->shero = 0; /* Timed -- Super Heroism */
    player_ptr->shield = 0; /* Timed -- Shield Spell */
    player_ptr->blessed = 0; /* Timed -- Blessed */
    player_ptr->tim_invis = 0; /* Timed -- Invisibility */
    player_ptr->tim_infra = 0; /* Timed -- Infra Vision */
    player_ptr->tim_regen = 0; /* Timed -- Regeneration */
    player_ptr->tim_stealth = 0; /* Timed -- Stealth */
    player_ptr->tim_esp = 0;
    player_ptr->wraith_form = 0; /* Timed -- Wraith Form */
    player_ptr->tim_levitation = 0;
    player_ptr->tim_sh_touki = 0;
    player_ptr->tim_sh_fire = 0;
    player_ptr->tim_sh_holy = 0;
    player_ptr->tim_eyeeye = 0;
    player_ptr->magicdef = 0;
    player_ptr->resist_magic = 0;
    player_ptr->tsuyoshi = 0;
    player_ptr->tim_pass_wall = 0;
    player_ptr->tim_res_nether = 0;
    player_ptr->tim_res_time = 0;
    player_ptr->tim_mimic = 0;
    player_ptr->mimic_form = MimicKindType::NONE;
    player_ptr->tim_reflect = 0;
    player_ptr->multishadow = 0;
    player_ptr->dustrobe = 0;
    player_ptr->action = ACTION_NONE;

    player_ptr->oppose_acid = 0; /* Timed -- oppose acid */
    player_ptr->oppose_elec = 0; /* Timed -- oppose lightning */
    player_ptr->oppose_fire = 0; /* Timed -- oppose heat */
    player_ptr->oppose_cold = 0; /* Timed -- oppose cold */
    player_ptr->oppose_pois = 0; /* Timed -- oppose poison */

    player_ptr->word_recall = 0;
    player_ptr->alter_reality = 0;
    player_ptr->sutemi = false;
    player_ptr->counter = false;
    player_ptr->ele_attack = 0;
    player_ptr->ele_immune = 0;
    player_ptr->special_attack = 0L;
    player_ptr->special_defense = 0L;

    while (player_ptr->energy_need < 0) {
        player_ptr->energy_need += ENERGY_NEED();
    }

    player_ptr->timewalk = false;

    if (player_ptr->riding) {
        (void)set_monster_fast(player_ptr, player_ptr->riding, 0);
        (void)set_monster_slow(player_ptr, player_ptr->riding, 0);
        (void)set_monster_invulner(player_ptr, player_ptr->riding, 0, false);
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::BARD)) {
        set_singing_song_effect(player_ptr, MUSIC_NONE);
        set_singing_song_id(player_ptr, 0);
    }
}

/*!
 * @brief 加速の継続時間をセットする / Set "fast", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_acceleration(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    auto acceleration = player_ptr->effects()->acceleration();
    if (v) {
        if (acceleration->is_fast() && !do_dec) {
            if (acceleration->current() > v) {
                return false;
            }
        } else if (!is_fast(player_ptr) && !player_ptr->lightspeed) {
            msg_print(_("素早く動けるようになった！", "You feel yourself moving much faster!"));
            notice = true;
            chg_virtue(player_ptr, Virtue::PATIENCE, -1);
            chg_virtue(player_ptr, Virtue::DILIGENCE, 1);
        }
    } else {
        if (acceleration->is_fast() && !player_ptr->lightspeed && !music_singing(player_ptr, MUSIC_SPEED) && !music_singing(player_ptr, MUSIC_SHERO)) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = true;
        }
    }

    acceleration->set(v);
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

bool mod_acceleration(PlayerType *player_ptr, const TIME_EFFECT v, const bool do_dec)
{
    return set_acceleration(player_ptr, player_ptr->effects()->acceleration()->current() + v, do_dec);
}

/*!
 * @brief 肌石化の継続時間をセットする / Set "shield", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shield(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->shield && !do_dec) {
            if (player_ptr->shield > v) {
                return false;
            }
        } else if (!player_ptr->shield) {
            msg_print(_("肌が石になった。", "Your skin turns to stone."));
            notice = true;
        }
    } else {
        if (player_ptr->shield) {
            msg_print(_("肌が元に戻った。", "Your skin returns to normal."));
            notice = true;
        }
    }

    player_ptr->shield = v;
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
 * @brief 魔法の鎧の継続時間をセットする / Set "magicdef", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_magicdef(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->magicdef && !do_dec) {
            if (player_ptr->magicdef > v) {
                return false;
            }
        } else if (!player_ptr->magicdef) {
            msg_print(_("魔法の防御力が増したような気がする。", "You feel more resistant to magic."));
            notice = true;
        }
    } else {
        if (player_ptr->magicdef) {
            msg_print(_("魔法の防御力が元に戻った。", "You feel less resistant to magic."));
            notice = true;
        }
    }

    player_ptr->magicdef = v;
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
 * @brief 祝福の継続時間をセットする / Set "blessed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_blessed(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->blessed && !do_dec) {
            if (player_ptr->blessed > v) {
                return false;
            }
        } else if (!is_blessed(player_ptr)) {
            msg_print(_("高潔な気分になった！", "You feel righteous!"));
            notice = true;
        }
    } else {
        if (player_ptr->blessed && !music_singing(player_ptr, MUSIC_BLESS)) {
            msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            notice = true;
        }
    }

    player_ptr->blessed = v;
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
 * @brief 士気高揚の継続時間をセットする / Set "hero", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_hero(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->hero && !do_dec) {
            if (player_ptr->hero > v) {
                return false;
            }
        } else if (!is_hero(player_ptr)) {
            msg_print(_("ヒーローになった気がする！", "You feel like a hero!"));
            notice = true;
        }
    } else {
        if (player_ptr->hero && !music_singing(player_ptr, MUSIC_HERO) && !music_singing(player_ptr, MUSIC_SHERO)) {
            msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
            notice = true;
        }
    }

    player_ptr->hero = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_HP);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 変身効果の継続時間と変身先をセットする / Set "tim_mimic", and "mimic_form", notice observable changes
 * @param v 継続時間
 * @param mimic_race_idx 変身内容
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_mimic(PlayerType *player_ptr, TIME_EFFECT v, MimicKindType mimic_race_idx, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_mimic && (player_ptr->mimic_form == mimic_race_idx) && !do_dec) {
            if (player_ptr->tim_mimic > v) {
                return false;
            }
        } else if ((!player_ptr->tim_mimic) || (player_ptr->mimic_form != mimic_race_idx)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            player_ptr->mimic_form = mimic_race_idx;
            notice = true;
        }
    }

    else {
        if (player_ptr->tim_mimic) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            if (player_ptr->mimic_form == MimicKindType::DEMON) {
                set_oppose_fire(player_ptr, 0, true);
            }
            player_ptr->mimic_form = MimicKindType::NONE;
            notice = true;
            mimic_race_idx = MimicKindType::NONE;
        }
    }

    player_ptr->tim_mimic = v;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, true);
    }

    player_ptr->redraw |= (PR_BASIC | PR_TIMED_EFFECT);
    player_ptr->update |= (PU_BONUS | PU_HP);

    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 狂戦士化の継続時間をセットする / Set "shero", notice observable changes
 * @param v 継続時間/ 0ならば無条件にリセット
 * @param do_dec FALSEの場合現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shero(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        v = 1;
        return false;
    }

    if (v) {
        if (player_ptr->shero && !do_dec) {
            if (player_ptr->shero > v) {
                return false;
            }
        } else if (!player_ptr->shero) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = true;
        }
    } else {
        if (player_ptr->shero) {
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = true;
        }
    }

    player_ptr->shero = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_HP);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 幽体化の継続時間をセットする / Set "wraith_form", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_wraith_form(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->wraith_form && !do_dec) {
            if (player_ptr->wraith_form > v) {
                return false;
            }
        } else if (!player_ptr->wraith_form) {
            msg_print(_("物質界を離れて幽鬼のような存在になった！", "You leave the physical world and turn into a wraith-being!"));
            notice = true;
            chg_virtue(player_ptr, Virtue::UNLIFE, 3);
            chg_virtue(player_ptr, Virtue::HONOUR, -2);
            chg_virtue(player_ptr, Virtue::SACRIFICE, -2);
            chg_virtue(player_ptr, Virtue::VALOUR, -5);

            player_ptr->redraw |= (PR_MAP);
            player_ptr->update |= (PU_MONSTER_STATUSES);

            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (player_ptr->wraith_form) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            notice = true;

            player_ptr->redraw |= (PR_MAP);
            player_ptr->update |= (PU_MONSTER_STATUSES);

            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    player_ptr->wraith_form = v;
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
 * @brief オクレ兄さんの継続時間をセットする / Set "tsuyoshi", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tsuyoshi(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tsuyoshi && !do_dec) {
            if (player_ptr->tsuyoshi > v) {
                return false;
            }
        } else if (!player_ptr->tsuyoshi) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = true;
            chg_virtue(player_ptr, Virtue::VITALITY, 2);
        }
    } else {
        if (player_ptr->tsuyoshi) {
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(player_ptr, A_CON, 20, true);
            (void)dec_stat(player_ptr, A_STR, 20, true);

            notice = true;
            chg_virtue(player_ptr, Virtue::VITALITY, -3);
        }
    }

    player_ptr->tsuyoshi = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_HP);
    handle_stuff(player_ptr);
    return true;
}
