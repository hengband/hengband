﻿#include "status/buff-setter.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "monster/monster-status-setter.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-race.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの全ての時限効果をリセットする。 / reset timed flags
 */
void reset_tim_flags(player_type *creature_ptr)
{
    creature_ptr->fast = 0; /* Timed -- Fast */
    creature_ptr->lightspeed = 0;
    creature_ptr->slow = 0; /* Timed -- Slow */
    creature_ptr->blind = 0; /* Timed -- Blindness */
    creature_ptr->paralyzed = 0; /* Timed -- Paralysis */
    creature_ptr->confused = 0; /* Timed -- Confusion */
    creature_ptr->afraid = 0; /* Timed -- Fear */
    creature_ptr->image = 0; /* Timed -- Hallucination */
    creature_ptr->poisoned = 0; /* Timed -- Poisoned */
    creature_ptr->cut = 0; /* Timed -- Cut */
    creature_ptr->stun = 0; /* Timed -- Stun */

    creature_ptr->protevil = 0; /* Timed -- Protection */
    creature_ptr->invuln = 0; /* Timed -- Invulnerable */
    creature_ptr->ult_res = 0;
    creature_ptr->hero = 0; /* Timed -- Heroism */
    creature_ptr->shero = 0; /* Timed -- Super Heroism */
    creature_ptr->shield = 0; /* Timed -- Shield Spell */
    creature_ptr->blessed = 0; /* Timed -- Blessed */
    creature_ptr->tim_invis = 0; /* Timed -- Invisibility */
    creature_ptr->tim_infra = 0; /* Timed -- Infra Vision */
    creature_ptr->tim_regen = 0; /* Timed -- Regeneration */
    creature_ptr->tim_stealth = 0; /* Timed -- Stealth */
    creature_ptr->tim_esp = 0;
    creature_ptr->wraith_form = 0; /* Timed -- Wraith Form */
    creature_ptr->tim_levitation = 0;
    creature_ptr->tim_sh_touki = 0;
    creature_ptr->tim_sh_fire = 0;
    creature_ptr->tim_sh_holy = 0;
    creature_ptr->tim_eyeeye = 0;
    creature_ptr->magicdef = 0;
    creature_ptr->resist_magic = 0;
    creature_ptr->tsuyoshi = 0;
    creature_ptr->tim_pass_wall = 0;
    creature_ptr->tim_res_nether = 0;
    creature_ptr->tim_res_time = 0;
    creature_ptr->tim_mimic = 0;
    creature_ptr->mimic_form = 0;
    creature_ptr->tim_reflect = 0;
    creature_ptr->multishadow = 0;
    creature_ptr->dustrobe = 0;
    creature_ptr->action = ACTION_NONE;

    creature_ptr->oppose_acid = 0; /* Timed -- oppose acid */
    creature_ptr->oppose_elec = 0; /* Timed -- oppose lightning */
    creature_ptr->oppose_fire = 0; /* Timed -- oppose heat */
    creature_ptr->oppose_cold = 0; /* Timed -- oppose cold */
    creature_ptr->oppose_pois = 0; /* Timed -- oppose poison */

    creature_ptr->word_recall = 0;
    creature_ptr->alter_reality = 0;
    creature_ptr->sutemi = false;
    creature_ptr->counter = false;
    creature_ptr->ele_attack = 0;
    creature_ptr->ele_immune = 0;
    creature_ptr->special_attack = 0L;
    creature_ptr->special_defense = 0L;

    while (creature_ptr->energy_need < 0)
        creature_ptr->energy_need += ENERGY_NEED();

    creature_ptr->timewalk = false;

    if (creature_ptr->riding) {
        (void)set_monster_fast(creature_ptr, creature_ptr->riding, 0);
        (void)set_monster_slow(creature_ptr, creature_ptr->riding, 0);
        (void)set_monster_invulner(creature_ptr, creature_ptr->riding, 0, false);
    }

    if (creature_ptr->pclass == CLASS_BARD) {
        set_singing_song_effect(creature_ptr, MUSIC_NONE);
        set_singing_song_id(creature_ptr, 0);
    }
}

/*!
 * @brief 加速の継続時間をセットする / Set "fast", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_fast(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->fast && !do_dec) {
            if (creature_ptr->fast > v)
                return false;
        } else if (!is_fast(creature_ptr) && !creature_ptr->lightspeed) {
            msg_print(_("素早く動けるようになった！", "You feel yourself moving much faster!"));
            notice = true;
            chg_virtue(creature_ptr, V_PATIENCE, -1);
            chg_virtue(creature_ptr, V_DILIGENCE, 1);
        }
    } else {
        if (creature_ptr->fast && !creature_ptr->lightspeed && !music_singing(creature_ptr, MUSIC_SPEED) && !music_singing(creature_ptr, MUSIC_SHERO)) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = true;
        }
    }

    creature_ptr->fast = v;
    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 肌石化の継続時間をセットする / Set "shield", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shield(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->shield && !do_dec) {
            if (creature_ptr->shield > v)
                return false;
        } else if (!creature_ptr->shield) {
            msg_print(_("肌が石になった。", "Your skin turns to stone."));
            notice = true;
        }
    } else {
        if (creature_ptr->shield) {
            msg_print(_("肌が元に戻った。", "Your skin returns to normal."));
            notice = true;
        }
    }

    creature_ptr->shield = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 魔法の鎧の継続時間をセットする / Set "magicdef", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_magicdef(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->magicdef && !do_dec) {
            if (creature_ptr->magicdef > v)
                return false;
        } else if (!creature_ptr->magicdef) {
            msg_print(_("魔法の防御力が増したような気がする。", "You feel more resistant to magic."));
            notice = true;
        }
    } else {
        if (creature_ptr->magicdef) {
            msg_print(_("魔法の防御力が元に戻った。", "You feel less resistant to magic."));
            notice = true;
        }
    }

    creature_ptr->magicdef = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 祝福の継続時間をセットする / Set "blessed", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_blessed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->blessed && !do_dec) {
            if (creature_ptr->blessed > v)
                return false;
        } else if (!is_blessed(creature_ptr)) {
            msg_print(_("高潔な気分になった！", "You feel righteous!"));
            notice = true;
        }
    } else {
        if (creature_ptr->blessed && !music_singing(creature_ptr, MUSIC_BLESS)) {
            msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            notice = true;
        }
    }

    creature_ptr->blessed = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 士気高揚の継続時間をセットする / Set "hero", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_hero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->hero && !do_dec) {
            if (creature_ptr->hero > v)
                return false;
        } else if (!is_hero(creature_ptr)) {
            msg_print(_("ヒーローになった気がする！", "You feel like a hero!"));
            notice = true;
        }
    } else {
        if (creature_ptr->hero && !music_singing(creature_ptr, MUSIC_HERO) && !music_singing(creature_ptr, MUSIC_SHERO)) {
            msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
            notice = true;
        }
    }

    creature_ptr->hero = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 変身効果の継続時間と変身先をセットする / Set "tim_mimic", and "mimic_form", notice observable changes
 * @param v 継続時間
 * @param p 変身内容
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->tim_mimic && (creature_ptr->mimic_form == p) && !do_dec) {
            if (creature_ptr->tim_mimic > v)
                return false;
        } else if ((!creature_ptr->tim_mimic) || (creature_ptr->mimic_form != p)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            creature_ptr->mimic_form = p;
            notice = true;
        }
    }

    else {
        if (creature_ptr->tim_mimic) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            if (creature_ptr->mimic_form == MIMIC_DEMON)
                set_oppose_fire(creature_ptr, 0, true);
            creature_ptr->mimic_form = 0;
            notice = true;
            p = 0;
        }
    }

    creature_ptr->tim_mimic = v;
    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, true);

    creature_ptr->redraw |= (PR_BASIC | PR_STATUS);
    creature_ptr->update |= (PU_BONUS | PU_HP);

    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 狂戦士化の継続時間をセットする / Set "shero", notice observable changes
 * @param v 継続時間/ 0ならば無条件にリセット
 * @param do_dec FALSEの場合現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        v = 1;
        return false;
    }

    if (v) {
        if (creature_ptr->shero && !do_dec) {
            if (creature_ptr->shero > v)
                return false;
        } else if (!creature_ptr->shero) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = true;
        }
    } else {
        if (creature_ptr->shero) {
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = true;
        }
    }

    creature_ptr->shero = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 幽体化の継続時間をセットする / Set "wraith_form", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->wraith_form && !do_dec) {
            if (creature_ptr->wraith_form > v)
                return false;
        } else if (!creature_ptr->wraith_form) {
            msg_print(_("物質界を離れて幽鬼のような存在になった！", "You leave the physical world and turn into a wraith-being!"));
            notice = true;
            chg_virtue(creature_ptr, V_UNLIFE, 3);
            chg_virtue(creature_ptr, V_HONOUR, -2);
            chg_virtue(creature_ptr, V_SACRIFICE, -2);
            chg_virtue(creature_ptr, V_VALOUR, -5);

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (creature_ptr->wraith_form) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            notice = true;

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    creature_ptr->wraith_form = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief オクレ兄さんの継続時間をセットする / Set "tsuyoshi", notice observable changes
 * @param v 継続時間
 * @param do_dec 魔力消去や時間経過による残り時間の減少処理ならばtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->tsuyoshi && !do_dec) {
            if (creature_ptr->tsuyoshi > v)
                return false;
        } else if (!creature_ptr->tsuyoshi) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = true;
            chg_virtue(creature_ptr, V_VITALITY, 2);
        }
    } else {
        if (creature_ptr->tsuyoshi) {
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(creature_ptr, A_CON, 20, true);
            (void)dec_stat(creature_ptr, A_STR, 20, true);

            notice = true;
            chg_virtue(creature_ptr, V_VITALITY, -3);
        }
    }

    creature_ptr->tsuyoshi = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return true;
}
