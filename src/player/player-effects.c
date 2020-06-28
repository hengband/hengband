/*!
 * @brief プレイヤーのステータス管理 / effects of various "objects"
 * @date 2014/01/01
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "player/player-effects.h"
#include "autopick/autopick-reader-writer.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "birth/character-builder.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "io/input-key-acceptor.h"
#include "io/report.h"
#include "io/save.h"
#include "locale/vowel-checker.h"
#include "mind/stances-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-sniper.h"
#include "monster/monster-status.h"
#include "mutation/mutation.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells-status.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの継続行動を設定する。
 * @param typ 継続行動のID\n
 * #ACTION_NONE / #ACTION_SEARCH / #ACTION_REST / #ACTION_LEARN / #ACTION_FISH / #ACTION_KAMAE / #ACTION_KATA / #ACTION_SING / #ACTION_HAYAGAKE / #ACTION_SPELL
 * から選択。
 * @return なし
 */
void set_action(player_type *creature_ptr, ACTION_IDX typ)
{
    int prev_typ = creature_ptr->action;

    if (typ == prev_typ) {
        return;
    } else {
        switch (prev_typ) {
        case ACTION_SEARCH: {
            msg_print(_("探索をやめた。", "You no longer walk carefully."));
            creature_ptr->redraw |= (PR_SPEED);
            break;
        }
        case ACTION_REST: {
            creature_ptr->resting = 0;
            break;
        }
        case ACTION_LEARN: {
            msg_print(_("学習をやめた。", "You stop learning."));
            creature_ptr->new_mane = FALSE;
            break;
        }
        case ACTION_KAMAE: {
            msg_print(_("構えをといた。", "You stop assuming the special stance."));
            creature_ptr->special_defense &= ~(KAMAE_MASK);
            break;
        }
        case ACTION_KATA: {
            msg_print(_("型を崩した。", "You stop assuming the special stance."));
            creature_ptr->special_defense &= ~(KATA_MASK);
            creature_ptr->update |= (PU_MONSTERS);
            creature_ptr->redraw |= (PR_STATUS);
            break;
        }
        case ACTION_SING: {
            msg_print(_("歌うのをやめた。", "You stop singing."));
            break;
        }
        case ACTION_HAYAGAKE: {
            msg_print(_("足が重くなった。", "You are no longer walking extremely fast."));
            take_turn(creature_ptr, 100);
            break;
        }
        case ACTION_SPELL: {
            msg_print(_("呪文の詠唱を中断した。", "You stopped casting."));
            break;
        }
        }
    }

    creature_ptr->action = typ;

    /* If we are requested other action, stop singing */
    if (prev_typ == ACTION_SING)
        stop_singing(creature_ptr);
    if (prev_typ == ACTION_SPELL)
        stop_hex_spell(creature_ptr);

    switch (creature_ptr->action) {
    case ACTION_SEARCH: {
        msg_print(_("注意深く歩き始めた。", "You begin to walk carefully."));
        creature_ptr->redraw |= (PR_SPEED);
        break;
    }
    case ACTION_LEARN: {
        msg_print(_("学習を始めた。", "You begin learning"));
        break;
    }
    case ACTION_FISH: {
        msg_print(_("水面に糸を垂らした．．．", "You begin fishing..."));
        break;
    }
    case ACTION_HAYAGAKE: {
        msg_print(_("足が羽のように軽くなった。", "You begin to walk extremely fast."));
        break;
    }
    default: {
        break;
    }
    }
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_STATE);
}

/*!
 * @brief プレイヤーに魔力消去効果を与える。
 * @return なし
 */
void dispel_player(player_type *creature_ptr)
{
    (void)set_fast(creature_ptr, 0, TRUE);
    set_lightspeed(creature_ptr, 0, TRUE);
    (void)set_slow(creature_ptr, 0, TRUE);
    (void)set_shield(creature_ptr, 0, TRUE);
    (void)set_blessed(creature_ptr, 0, TRUE);
    (void)set_tsuyoshi(creature_ptr, 0, TRUE);
    (void)set_hero(creature_ptr, 0, TRUE);
    (void)set_shero(creature_ptr, 0, TRUE);
    (void)set_protevil(creature_ptr, 0, TRUE);
    (void)set_invuln(creature_ptr, 0, TRUE);
    (void)set_wraith_form(creature_ptr, 0, TRUE);
    (void)set_kabenuke(creature_ptr, 0, TRUE);
    (void)set_tim_res_nether(creature_ptr, 0, TRUE);
    (void)set_tim_res_time(creature_ptr, 0, TRUE);
    (void)set_tim_reflect(creature_ptr, 0, TRUE);
    (void)set_multishadow(creature_ptr, 0, TRUE);
    (void)set_dustrobe(creature_ptr, 0, TRUE);

    (void)set_tim_invis(creature_ptr, 0, TRUE);
    (void)set_tim_infra(creature_ptr, 0, TRUE);
    (void)set_tim_esp(creature_ptr, 0, TRUE);
    (void)set_tim_regen(creature_ptr, 0, TRUE);
    (void)set_tim_stealth(creature_ptr, 0, TRUE);
    (void)set_tim_levitation(creature_ptr, 0, TRUE);
    (void)set_tim_sh_touki(creature_ptr, 0, TRUE);
    (void)set_tim_sh_fire(creature_ptr, 0, TRUE);
    (void)set_tim_sh_holy(creature_ptr, 0, TRUE);
    (void)set_tim_eyeeye(creature_ptr, 0, TRUE);
    (void)set_magicdef(creature_ptr, 0, TRUE);
    (void)set_resist_magic(creature_ptr, 0, TRUE);
    (void)set_oppose_acid(creature_ptr, 0, TRUE);
    (void)set_oppose_elec(creature_ptr, 0, TRUE);
    (void)set_oppose_fire(creature_ptr, 0, TRUE);
    (void)set_oppose_cold(creature_ptr, 0, TRUE);
    (void)set_oppose_pois(creature_ptr, 0, TRUE);
    (void)set_ultimate_res(creature_ptr, 0, TRUE);
    (void)set_mimic(creature_ptr, 0, 0, TRUE);
    (void)set_ele_attack(creature_ptr, 0, 0);
    (void)set_ele_immune(creature_ptr, 0, 0);

    if (creature_ptr->special_attack & ATTACK_CONFUSE) {
        creature_ptr->special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
    }

    if (music_singing_any(creature_ptr) || hex_spelling_any(creature_ptr)) {
        concptr str = (music_singing_any(creature_ptr)) ? _("歌", "singing") : _("呪文", "casting");
        INTERUPTING_SONG_EFFECT(creature_ptr) = SINGING_SONG_EFFECT(creature_ptr);
        SINGING_SONG_EFFECT(creature_ptr) = MUSIC_NONE;
        msg_format(_("%sが途切れた。", "Your %s is interrupted."), str);

        creature_ptr->action = ACTION_NONE;
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
        creature_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
        creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        creature_ptr->energy_need += ENERGY_NEED();
    }
}

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
 * @brief 対邪悪結界の継続時間をセットする / Set "protevil", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_protevil(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->protevil && !do_dec) {
            if (creature_ptr->protevil > v)
                return FALSE;
        } else if (!creature_ptr->protevil) {
            msg_print(_("邪悪なる存在から守られているような感じがする！", "You feel safe from evil!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->protevil) {
            msg_print(_("邪悪なる存在から守られている感じがなくなった。", "You no longer feel safe from evil."));
            notice = TRUE;
        }
    }

    creature_ptr->protevil = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
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
 * @brief 無傷球の継続時間をセットする / Set "invuln", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_invuln(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->invuln && !do_dec) {
            if (creature_ptr->invuln > v)
                return FALSE;
        } else if (!is_invuln(creature_ptr)) {
            msg_print(_("無敵だ！", "Invulnerability!"));
            notice = TRUE;

            chg_virtue(creature_ptr, V_UNLIFE, -2);
            chg_virtue(creature_ptr, V_HONOUR, -2);
            chg_virtue(creature_ptr, V_SACRIFICE, -3);
            chg_virtue(creature_ptr, V_VALOUR, -5);

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (creature_ptr->invuln && !music_singing(creature_ptr, MUSIC_INVULN)) {
            msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));
            notice = TRUE;

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

            creature_ptr->energy_need += ENERGY_NEED();
        }
    }

    creature_ptr->invuln = v;
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
 * @brief 時限急回復の継続時間をセットする / Set "tim_regen", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_regen(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_regen && !do_dec) {
            if (creature_ptr->tim_regen > v)
                return FALSE;
        } else if (!creature_ptr->tim_regen) {
            msg_print(_("回復力が上がった！", "You feel yourself regenerating quickly!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_regen) {
            msg_print(_("素早く回復する感じがなくなった。", "You feel yourself regenerating slowly."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_regen = v;
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
 * @brief 隠密の歌の継続時間をセットする / Set "tim_stealth", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_stealth(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_stealth && !do_dec) {
            if (creature_ptr->tim_stealth > v)
                return FALSE;
        } else if (!is_time_limit_stealth(creature_ptr)) {
            msg_print(_("足音が小さくなった！", "You begin to walk silently!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_stealth && !music_singing(creature_ptr, MUSIC_STEALTH)) {
            msg_print(_("足音が大きくなった。", "You no longer walk silently."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_stealth = v;
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
 * @brief 一時的浮遊の継続時間をセットする / Set "tim_levitation", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_levitation(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_levitation && !do_dec) {
            if (creature_ptr->tim_levitation > v)
                return FALSE;
        } else if (!creature_ptr->tim_levitation) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_levitation) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_levitation = v;
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
 * @brief 一時的闘気のオーラの継続時間をセットする / Set "tim_sh_touki", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_touki(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_sh_touki && !do_dec) {
            if (creature_ptr->tim_sh_touki > v)
                return FALSE;
        } else if (!creature_ptr->tim_sh_touki) {
            msg_print(_("体が闘気のオーラで覆われた。", "You are enveloped by an aura of the Force!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_sh_touki) {
            msg_print(_("闘気が消えた。", "The aura of the Force disappeared."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_sh_touki = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的火炎のオーラの継続時間をセットする / Set "tim_sh_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_sh_fire && !do_dec) {
            if (creature_ptr->tim_sh_fire > v)
                return FALSE;
        } else if (!creature_ptr->tim_sh_fire) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_sh_fire) {
            msg_print(_("炎のオーラが消えた。", "The fiery aura disappeared."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_sh_fire = v;
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
 * @brief 一時的聖なるのオーラの継続時間をセットする / Set "tim_sh_holy", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_holy(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_sh_holy && !do_dec) {
            if (creature_ptr->tim_sh_holy > v)
                return FALSE;
        } else if (!creature_ptr->tim_sh_holy) {
            msg_print(_("体が聖なるオーラで覆われた。", "You are enveloped by a holy aura!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_sh_holy) {
            msg_print(_("聖なるオーラが消えた。", "The holy aura disappeared."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_sh_holy = v;
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
 * @brief 目には目をの残り時間をセットする / Set "tim_eyeeye", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_eyeeye(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_eyeeye && !do_dec) {
            if (creature_ptr->tim_eyeeye > v)
                return FALSE;
        } else if (!creature_ptr->tim_eyeeye) {
            msg_print(_("法の守り手になった気がした！", "You feel like a keeper of commandments!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_eyeeye) {
            msg_print(_("懲罰を執行することができなくなった。", "You no longer feel like a keeper."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_eyeeye = v;
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
 * @brief 一時的魔法防御の継続時間をセットする / Set "resist_magic", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_resist_magic(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->resist_magic && !do_dec) {
            if (creature_ptr->resist_magic > v)
                return FALSE;
        } else if (!creature_ptr->resist_magic) {
            msg_print(_("魔法への耐性がついた。", "You have been protected from magic!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->resist_magic) {
            msg_print(_("魔法に弱くなった。", "You are no longer protected from magic."));
            notice = TRUE;
        }
    }

    creature_ptr->resist_magic = v;
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
 * @brief 一時的反射の継続時間をセットする / Set "tim_reflect", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_reflect(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_reflect && !do_dec) {
            if (creature_ptr->tim_reflect > v)
                return FALSE;
        } else if (!creature_ptr->tim_reflect) {
            msg_print(_("体の表面が滑かになった気がする。", "Your body becames smooth."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_reflect) {
            msg_print(_("体の表面が滑かでなくなった。", "Your body is no longer smooth."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_reflect = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*
 * Set "multishadow", notice observable changes
 */
bool set_multishadow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->multishadow && !do_dec) {
            if (creature_ptr->multishadow > v)
                return FALSE;
        } else if (!creature_ptr->multishadow) {
            msg_print(_("あなたの周りに幻影が生まれた。", "Your Shadow enveloped you."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->multishadow) {
            msg_print(_("幻影が消えた。", "Your Shadow disappears."));
            notice = TRUE;
        }
    }

    creature_ptr->multishadow = v;
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
 * @brief 一時的破片のオーラの継続時間をセットする / Set "dustrobe", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_dustrobe(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->dustrobe && !do_dec) {
            if (creature_ptr->dustrobe > v)
                return FALSE;
        } else if (!creature_ptr->dustrobe) {
            msg_print(_("体が鏡のオーラで覆われた。", "You were enveloped by mirror shards."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->dustrobe) {
            msg_print(_("鏡のオーラが消えた。", "The mirror shards disappear."));
            notice = TRUE;
        }
    }

    creature_ptr->dustrobe = v;
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
 * @brief 一時的壁抜けの継続時間をセットする / Set "kabenuke", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_kabenuke(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->kabenuke && !do_dec) {
            if (creature_ptr->kabenuke > v)
                return FALSE;
        } else if (!creature_ptr->kabenuke) {
            msg_print(_("体が半物質の状態になった。", "You became ethereal."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->kabenuke) {
            msg_print(_("体が物質化した。", "You are no longer ethereal."));
            notice = TRUE;
        }
    }

    creature_ptr->kabenuke = v;
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

/*!
 * @brief 一時的元素スレイの継続時間をセットする / Set a temporary elemental brand. Clear all other brands. Print status messages. -LM-
 * @param attack_type スレイのタイプID
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_ele_attack(player_type *creature_ptr, u32b attack_type, TIME_EFFECT v)
{
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if ((creature_ptr->special_attack & (ATTACK_ACID)) && (attack_type != ATTACK_ACID)) {
        creature_ptr->special_attack &= ~(ATTACK_ACID);
        msg_print(_("酸で攻撃できなくなった。", "Your temporary acidic brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_ELEC)) && (attack_type != ATTACK_ELEC)) {
        creature_ptr->special_attack &= ~(ATTACK_ELEC);
        msg_print(_("電撃で攻撃できなくなった。", "Your temporary electrical brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_FIRE)) && (attack_type != ATTACK_FIRE)) {
        creature_ptr->special_attack &= ~(ATTACK_FIRE);
        msg_print(_("火炎で攻撃できなくなった。", "Your temporary fiery brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_COLD)) && (attack_type != ATTACK_COLD)) {
        creature_ptr->special_attack &= ~(ATTACK_COLD);
        msg_print(_("冷気で攻撃できなくなった。", "Your temporary frost brand fades away."));
    }

    if ((creature_ptr->special_attack & (ATTACK_POIS)) && (attack_type != ATTACK_POIS)) {
        creature_ptr->special_attack &= ~(ATTACK_POIS);
        msg_print(_("毒で攻撃できなくなった。", "Your temporary poison brand fades away."));
    }

    if ((v) && (attack_type)) {
        creature_ptr->special_attack |= (attack_type);
        creature_ptr->ele_attack = v;
#ifdef JP
        msg_format("%sで攻撃できるようになった！",
            ((attack_type == ATTACK_ACID)
                    ? "酸"
                    : ((attack_type == ATTACK_ELEC)
                            ? "電撃"
                            : ((attack_type == ATTACK_FIRE) ? "火炎"
                                                            : ((attack_type == ATTACK_COLD) ? "冷気" : ((attack_type == ATTACK_POIS) ? "毒" : "(なし)"))))));
#else
        msg_format("For a while, the blows you deal will %s",
            ((attack_type == ATTACK_ACID)
                    ? "melt with acid!"
                    : ((attack_type == ATTACK_ELEC)
                            ? "shock your foes!"
                            : ((attack_type == ATTACK_FIRE)
                                    ? "burn with fire!"
                                    : ((attack_type == ATTACK_COLD) ? "chill to the bone!"
                                                                    : ((attack_type == ATTACK_POIS) ? "poison your enemies!" : "do nothing special."))))));
#endif
    }

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->redraw |= (PR_STATUS);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return TRUE;
}

/*!
 * @brief 一時的元素免疫の継続時間をセットする / Set a temporary elemental brand.  Clear all other brands.  Print status messages. -LM-
 * @param immune_type 免疫のタイプID
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_ele_immune(player_type *creature_ptr, u32b immune_type, TIME_EFFECT v)
{
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if ((creature_ptr->special_defense & (DEFENSE_ACID)) && (immune_type != DEFENSE_ACID)) {
        creature_ptr->special_defense &= ~(DEFENSE_ACID);
        msg_print(_("酸の攻撃で傷つけられるようになった。。", "You are no longer immune to acid."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_ELEC)) && (immune_type != DEFENSE_ELEC)) {
        creature_ptr->special_defense &= ~(DEFENSE_ELEC);
        msg_print(_("電撃の攻撃で傷つけられるようになった。。", "You are no longer immune to electricity."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_FIRE)) && (immune_type != DEFENSE_FIRE)) {
        creature_ptr->special_defense &= ~(DEFENSE_FIRE);
        msg_print(_("火炎の攻撃で傷つけられるようになった。。", "You are no longer immune to fire."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_COLD)) && (immune_type != DEFENSE_COLD)) {
        creature_ptr->special_defense &= ~(DEFENSE_COLD);
        msg_print(_("冷気の攻撃で傷つけられるようになった。。", "You are no longer immune to cold."));
    }

    if ((creature_ptr->special_defense & (DEFENSE_POIS)) && (immune_type != DEFENSE_POIS)) {
        creature_ptr->special_defense &= ~(DEFENSE_POIS);
        msg_print(_("毒の攻撃で傷つけられるようになった。。", "You are no longer immune to poison."));
    }

    if ((v) && (immune_type)) {
        creature_ptr->special_defense |= (immune_type);
        creature_ptr->ele_immune = v;
        msg_format(_("%sの攻撃を受けつけなくなった！", "For a while, You are immune to %s"),
            ((immune_type == DEFENSE_ACID)
                    ? _("酸", "acid!")
                    : ((immune_type == DEFENSE_ELEC)
                            ? _("電撃", "electricity!")
                            : ((immune_type == DEFENSE_FIRE)
                                    ? _("火炎", "fire!")
                                    : ((immune_type == DEFENSE_COLD)
                                            ? _("冷気", "cold!")
                                            : ((immune_type == DEFENSE_POIS) ? _("毒", "poison!") : _("(なし)", "do nothing special.")))))));
    }

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->redraw |= (PR_STATUS);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return TRUE;
}

/*!
 * @brief 一時的酸耐性の継続時間をセットする / Set "oppose_acid", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_acid(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->oppose_acid && !do_dec) {
            if (creature_ptr->oppose_acid > v)
                return FALSE;
        } else if (!is_oppose_acid(creature_ptr)) {
            msg_print(_("酸への耐性がついた気がする！", "You feel resistant to acid!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->oppose_acid && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            notice = TRUE;
        }
    }

    creature_ptr->oppose_acid = v;

    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的電撃耐性の継続時間をセットする / Set "oppose_elec", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_elec(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->oppose_elec && !do_dec) {
            if (creature_ptr->oppose_elec > v)
                return FALSE;
        } else if (!is_oppose_elec(creature_ptr)) {
            msg_print(_("電撃への耐性がついた気がする！", "You feel resistant to electricity!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->oppose_elec && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to electricity."));
            notice = TRUE;
        }
    }

    creature_ptr->oppose_elec = v;

    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的火炎耐性の継続時間をセットする / Set "oppose_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return FALSE;

    if ((is_specific_player_race(creature_ptr, RACE_BALROG) && (creature_ptr->lev > 44)) || (creature_ptr->mimic_form == MIMIC_DEMON))
        v = 1;
    if (v) {
        if (creature_ptr->oppose_fire && !do_dec) {
            if (creature_ptr->oppose_fire > v)
                return FALSE;
        } else if (!is_oppose_fire(creature_ptr)) {
            msg_print(_("火への耐性がついた気がする！", "You feel resistant to fire!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->oppose_fire && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            notice = TRUE;
        }
    }

    creature_ptr->oppose_fire = v;

    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的冷気耐性の継続時間をセットする / Set "oppose_cold", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_cold(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->oppose_cold && !do_dec) {
            if (creature_ptr->oppose_cold > v)
                return FALSE;
        } else if (!is_oppose_cold(creature_ptr)) {
            msg_print(_("冷気への耐性がついた気がする！", "You feel resistant to cold!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->oppose_cold && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            notice = TRUE;
        }
    }

    creature_ptr->oppose_cold = v;

    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的毒耐性の継続時間をセットする / Set "oppose_pois", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_pois(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if ((creature_ptr->pclass == CLASS_NINJA) && (creature_ptr->lev > 44))
        v = 1;
    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->oppose_pois && !do_dec) {
            if (creature_ptr->oppose_pois > v)
                return FALSE;
        } else if (!is_oppose_pois(creature_ptr)) {
            msg_print(_("毒への耐性がついた気がする！", "You feel resistant to poison!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->oppose_pois && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to poison."));
            notice = TRUE;
        }
    }

    creature_ptr->oppose_pois = v;
    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 空腹状態をセットする / Set "food", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Set "", notice observable changes\n
 *\n
 * The "food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.\n
 *\n
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  \n
 *\n
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.\n
 *\n
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).\n
 */
bool set_food(player_type *creature_ptr, TIME_EFFECT v)
{
    int old_aux, new_aux;

    bool notice = FALSE;
    v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;
    if (creature_ptr->food < PY_FOOD_FAINT) {
        old_aux = 0;
    } else if (creature_ptr->food < PY_FOOD_WEAK) {
        old_aux = 1;
    } else if (creature_ptr->food < PY_FOOD_ALERT) {
        old_aux = 2;
    } else if (creature_ptr->food < PY_FOOD_FULL) {
        old_aux = 3;
    } else if (creature_ptr->food < PY_FOOD_MAX) {
        old_aux = 4;
    } else {
        old_aux = 5;
    }

    if (v < PY_FOOD_FAINT) {
        new_aux = 0;
    } else if (v < PY_FOOD_WEAK) {
        new_aux = 1;
    } else if (v < PY_FOOD_ALERT) {
        new_aux = 2;
    } else if (v < PY_FOOD_FULL) {
        new_aux = 3;
    } else if (v < PY_FOOD_MAX) {
        new_aux = 4;
    } else {
        new_aux = 5;
    }

    if (old_aux < 1 && new_aux > 0)
        chg_virtue(creature_ptr, V_PATIENCE, 2);
    else if (old_aux < 3 && (old_aux != new_aux))
        chg_virtue(creature_ptr, V_PATIENCE, 1);
    if (old_aux == 2)
        chg_virtue(creature_ptr, V_TEMPERANCE, 1);
    if (old_aux == 0)
        chg_virtue(creature_ptr, V_TEMPERANCE, -1);

    if (new_aux > old_aux) {
        switch (new_aux) {
        case 1:
            msg_print(_("まだ空腹で倒れそうだ。", "You are still weak."));
            break;
        case 2:
            msg_print(_("まだ空腹だ。", "You are still hungry."));
            break;
        case 3:
            msg_print(_("空腹感がおさまった。", "You are no longer hungry."));
            break;
        case 4:
            msg_print(_("満腹だ！", "You are full!"));
            break;

        case 5:
            msg_print(_("食べ過ぎだ！", "You have gorged yourself!"));
            chg_virtue(creature_ptr, V_HARMONY, -1);
            chg_virtue(creature_ptr, V_PATIENCE, -1);
            chg_virtue(creature_ptr, V_TEMPERANCE, -2);
            break;
        }

        notice = TRUE;
    } else if (new_aux < old_aux) {
        switch (new_aux) {
        case 0:
            msg_print(_("あまりにも空腹で気を失ってしまった！", "You are getting faint from hunger!"));
            break;
        case 1:
            msg_print(_("お腹が空いて倒れそうだ。", "You are getting weak from hunger!"));
            break;
        case 2:
            msg_print(_("お腹が空いてきた。", "You are getting hungry."));
            break;
        case 3:
            msg_print(_("満腹感がなくなった。", "You are no longer full."));
            break;
        case 4:
            msg_print(_("やっとお腹がきつくなくなった。", "You are no longer gorged."));
            break;
        }

        if (creature_ptr->wild_mode && (new_aux < 2)) {
            change_wild_mode(creature_ptr, FALSE);
        }

        notice = TRUE;
    }

    creature_ptr->food = v;
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_HUNGER);
    handle_stuff(creature_ptr);

    return TRUE;
}

/*!
 * @brief プレイヤーの基本能力値を増加させる / Increases a stat by one randomized level -RAK-
 * @param stat 上昇させるステータスID
 * @return 実際に上昇した場合TRUEを返す。
 * @details
 * Note that this function (used by stat potions) now restores\n
 * the stat BEFORE increasing it.\n
 */
bool inc_stat(player_type *creature_ptr, int stat)
{
    BASE_STATUS gain;
    BASE_STATUS value = creature_ptr->stat_cur[stat];

    if (value < creature_ptr->stat_max_max[stat]) {
        if (value < 18) {
            gain = ((randint0(100) < 75) ? 1 : 2);
            value += gain;
        } else if (value < (creature_ptr->stat_max_max[stat] - 2)) {
            gain = (((creature_ptr->stat_max_max[stat]) - value) / 2 + 3) / 2;
            if (gain < 1)
                gain = 1;

            value += randint1(gain) + gain / 2;
            if (value > (creature_ptr->stat_max_max[stat] - 1))
                value = creature_ptr->stat_max_max[stat] - 1;
        } else {
            value++;
        }

        creature_ptr->stat_cur[stat] = value;
        if (value > creature_ptr->stat_max[stat]) {
            creature_ptr->stat_max[stat] = value;
        }

        creature_ptr->update |= (PU_BONUS);
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief プレイヤーの基本能力値を減少させる / Decreases a stat by an amount indended to vary from 0 to 100 percent.
 * @param stat 減少させるステータスID
 * @param amount 減少させる基本量
 * @param permanent TRUEならば現在の最大値を減少させる
 * @return 実際に減少した場合TRUEを返す。
 * @details
 *\n
 * Amount could be a little higher in extreme cases to mangle very high\n
 * stats from massive assaults.  -CWS\n
 *\n
 * Note that "permanent" means that the *given* amount is permanent,\n
 * not that the new value becomes permanent.  This may not work exactly\n
 * as expected, due to "weirdness" in the algorithm, but in general,\n
 * if your stat is already drained, the "max" value will not drop all\n
 * the way down to the "cur" value.\n
 */
bool dec_stat(player_type *creature_ptr, int stat, int amount, int permanent)
{
    int loss;
    bool res = FALSE;

    BASE_STATUS cur = creature_ptr->stat_cur[stat];
    BASE_STATUS max = creature_ptr->stat_max[stat];
    int same = (cur == max);
    if (cur > 3) {
        if (cur <= 18) {
            if (amount > 90)
                cur--;
            if (amount > 50)
                cur--;
            if (amount > 20)
                cur--;
            cur--;
        } else {
            loss = (((cur - 18) / 2 + 1) / 2 + 1);
            if (loss < 1)
                loss = 1;

            loss = ((randint1(loss) + loss) * amount) / 100;
            if (loss < amount / 2)
                loss = amount / 2;

            cur = cur - loss;
            if (cur < 18)
                cur = (amount <= 20) ? 18 : 17;
        }

        if (cur < 3)
            cur = 3;

        if (cur != creature_ptr->stat_cur[stat])
            res = TRUE;
    }

    if (permanent && (max > 3)) {
        chg_virtue(creature_ptr, V_SACRIFICE, 1);
        if (stat == A_WIS || stat == A_INT)
            chg_virtue(creature_ptr, V_ENLIGHTEN, -2);

        if (max <= 18) {
            if (amount > 90)
                max--;
            if (amount > 50)
                max--;
            if (amount > 20)
                max--;
            max--;
        } else {
            loss = (((max - 18) / 2 + 1) / 2 + 1);
            loss = ((randint1(loss) + loss) * amount) / 100;
            if (loss < amount / 2)
                loss = amount / 2;

            max = max - loss;
            if (max < 18)
                max = (amount <= 20) ? 18 : 17;
        }

        if (same || (max < cur))
            max = cur;

        if (max != creature_ptr->stat_max[stat])
            res = TRUE;
    }

    if (res) {
        creature_ptr->stat_cur[stat] = cur;
        creature_ptr->stat_max[stat] = max;
        creature_ptr->redraw |= (PR_STATS);
        creature_ptr->update |= (PU_BONUS);
    }

    return (res);
}

/*!
 * @brief プレイヤーの基本能力値を回復させる / Restore a stat.  Return TRUE only if this actually makes a difference.
 * @param stat 回復ステータスID
 * @return 実際に回復した場合TRUEを返す。
 */
bool res_stat(player_type *creature_ptr, int stat)
{
    if (creature_ptr->stat_cur[stat] != creature_ptr->stat_max[stat]) {
        creature_ptr->stat_cur[stat] = creature_ptr->stat_max[stat];
        creature_ptr->update |= (PU_BONUS);
        creature_ptr->redraw |= (PR_STATS);
        return TRUE;
    }

    return FALSE;
}

/*
 * Increase players hit points, notice effects
 */
bool hp_player(player_type *creature_ptr, int num)
{
    int vir;
    vir = virtue_number(creature_ptr, V_VITALITY);

    if (num <= 0)
        return FALSE;

    if (vir) {
        num = num * (creature_ptr->virtues[vir - 1] + 1250) / 1250;
    }

    if (creature_ptr->chp < creature_ptr->mhp) {
        if ((num > 0) && (creature_ptr->chp < (creature_ptr->mhp / 3)))
            chg_virtue(creature_ptr, V_TEMPERANCE, 1);

        creature_ptr->chp += num;
        if (creature_ptr->chp >= creature_ptr->mhp) {
            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->chp_frac = 0;
        }

        creature_ptr->redraw |= (PR_HP);
        creature_ptr->window |= (PW_PLAYER);
        if (num < 5) {
            msg_print(_("少し気分が良くなった。", "You feel a little better."));
        } else if (num < 15) {
            msg_print(_("気分が良くなった。", "You feel better."));
        } else if (num < 35) {
            msg_print(_("とても気分が良くなった。", "You feel much better."));
        } else {
            msg_print(_("ひじょうに気分が良くなった。", "You feel very good."));
        }

        return TRUE;
    }

    return FALSE;
}

/*
 * Array of stat "descriptions"
 */
static concptr desc_stat_pos[]
    = { _("強く", "stronger"), _("知的に", "smarter"), _("賢く", "wiser"), _("器用に", "more dextrous"), _("健康に", "healthier"), _("美しく", "cuter") };

/*
 * Array of stat "descriptions"
 */
static concptr desc_stat_neg[]
    = { _("弱く", "weaker"), _("無知に", "stupider"), _("愚かに", "more naive"), _("不器用に", "clumsier"), _("不健康に", "more sickly"), _("醜く", "uglier") };

/*
 * Lose a "point"
 */
bool do_dec_stat(player_type *creature_ptr, int stat)
{
    bool sust = FALSE;

    /* Access the "sustain" */
    switch (stat) {
    case A_STR:
        if (creature_ptr->sustain_str)
            sust = TRUE;
        break;
    case A_INT:
        if (creature_ptr->sustain_int)
            sust = TRUE;
        break;
    case A_WIS:
        if (creature_ptr->sustain_wis)
            sust = TRUE;
        break;
    case A_DEX:
        if (creature_ptr->sustain_dex)
            sust = TRUE;
        break;
    case A_CON:
        if (creature_ptr->sustain_con)
            sust = TRUE;
        break;
    case A_CHR:
        if (creature_ptr->sustain_chr)
            sust = TRUE;
        break;
    }

    if (sust && (!ironman_nightmare || randint0(13))) {
        msg_format(_("%sなった気がしたが、すぐに元に戻った。", "You feel %s for a moment, but the feeling passes."), desc_stat_neg[stat]);

        return TRUE;
    }

    if (dec_stat(creature_ptr, stat, 10, (ironman_nightmare && !randint0(13)))) {
        msg_format(_("ひどく%sなった気がする。", "You feel %s."), desc_stat_neg[stat]);

        return TRUE;
    }

    return FALSE;
}

/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(player_type *creature_ptr, int stat)
{
    if (res_stat(creature_ptr, stat)) {
        msg_format(_("元通りに%sなった気がする。", "You feel %s."), desc_stat_pos[stat]);
        return TRUE;
    }

    return FALSE;
}

/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(player_type *creature_ptr, int stat)
{
    bool res = res_stat(creature_ptr, stat);
    if (inc_stat(creature_ptr, stat)) {
        if (stat == A_WIS) {
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
            chg_virtue(creature_ptr, V_FAITH, 1);
        } else if (stat == A_INT) {
            chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
        } else if (stat == A_CON)
            chg_virtue(creature_ptr, V_VITALITY, 1);

        msg_format(_("ワーオ！とても%sなった！", "Wow! You feel %s!"), desc_stat_pos[stat]);
        return TRUE;
    }

    if (res) {
        msg_format(_("元通りに%sなった気がする。", "You feel %s."), desc_stat_pos[stat]);
        return TRUE;
    }

    return FALSE;
}

/*
 * Restores any drained experience
 */
bool restore_level(player_type *creature_ptr)
{
    if (creature_ptr->exp < creature_ptr->max_exp) {
        msg_print(_("経験値が戻ってきた気がする。", "You feel your experience returning."));
        creature_ptr->exp = creature_ptr->max_exp;
        check_experience(creature_ptr);
        return TRUE;
    }

    return FALSE;
}

/*
 * Forget everything
 */
bool lose_all_info(player_type *creature_ptr)
{
    chg_virtue(creature_ptr, V_KNOWLEDGE, -5);
    chg_virtue(creature_ptr, V_ENLIGHTEN, -5);
    for (int i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (object_is_fully_known(o_ptr))
            continue;

        o_ptr->feeling = FEEL_NONE;
        o_ptr->ident &= ~(IDENT_EMPTY);
        o_ptr->ident &= ~(IDENT_KNOWN);
        o_ptr->ident &= ~(IDENT_SENSE);
    }

    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    wiz_dark(creature_ptr);
    return TRUE;
}

void do_poly_wounds(player_type *creature_ptr)
{
    s16b wounds = creature_ptr->cut;
    s16b hit_p = (creature_ptr->mhp - creature_ptr->chp);
    s16b change = damroll(creature_ptr->lev, 5);
    bool Nasty_effect = one_in_(5);
    if (!(wounds || hit_p || Nasty_effect))
        return;

    msg_print(_("傷がより軽いものに変化した。", "Your wounds are polymorphed into less serious ones."));
    hp_player(creature_ptr, change);
    if (Nasty_effect) {
        msg_print(_("新たな傷ができた！", "A new wound was created!"));
        take_hit(creature_ptr, DAMAGE_LOSELIFE, change / 2, _("変化した傷", "a polymorphed wound"), -1);
        set_cut(creature_ptr, change);
    } else {
        set_cut(creature_ptr, creature_ptr->cut - (change / 2));
    }
}

/*
 * Change player race
 */
void change_race(player_type *creature_ptr, player_race_type new_race, concptr effect_msg)
{
    concptr title = race_info[new_race].title;
    int old_race = creature_ptr->prace;
#ifdef JP
    msg_format("あなたは%s%sに変化した！", effect_msg, title);
#else
    msg_format("You turn into %s %s%s!", (!effect_msg[0] && is_a_vowel(title[0]) ? "an" : "a"), effect_msg, title);
#endif

    chg_virtue(creature_ptr, V_CHANCE, 2);
    if (creature_ptr->prace < 32) {
        creature_ptr->old_race1 |= 1L << creature_ptr->prace;
    } else {
        creature_ptr->old_race2 |= 1L << (creature_ptr->prace - 32);
    }

    creature_ptr->prace = new_race;
    rp_ptr = &race_info[creature_ptr->prace];
    creature_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

    bool is_special_class = creature_ptr->pclass == CLASS_MONK;
    is_special_class |= creature_ptr->pclass == CLASS_FORCETRAINER;
    is_special_class |= creature_ptr->pclass == CLASS_NINJA;
    bool is_special_race = creature_ptr->prace == RACE_KLACKON;
    is_special_race |= creature_ptr->prace == RACE_SPRITE;
    if (is_special_class && is_special_race)
        creature_ptr->expfact -= 15;

    get_height_weight(creature_ptr);

    if (creature_ptr->pclass == CLASS_SORCERER)
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
    else
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

    roll_hitdice(creature_ptr, 0L);
    check_experience(creature_ptr);
    creature_ptr->redraw |= (PR_BASIC);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    if (old_race != creature_ptr->prace)
        autopick_load_pref(creature_ptr, FALSE);

    lite_spot(creature_ptr, creature_ptr->y, creature_ptr->x);
}

void do_poly_self(player_type *creature_ptr)
{
    int power = creature_ptr->lev;

    msg_print(_("あなたは変化の訪れを感じた...", "You feel a change coming over you..."));
    chg_virtue(creature_ptr, V_CHANCE, 1);

    if ((power > randint0(20)) && one_in_(3) && (creature_ptr->prace != RACE_ANDROID)) {
        char effect_msg[80] = "";
        player_race_type new_race;

        power -= 10;
        if ((power > randint0(5)) && one_in_(4)) {
            power -= 2;
            if (creature_ptr->psex == SEX_MALE) {
                creature_ptr->psex = SEX_FEMALE;
                sp_ptr = &sex_info[creature_ptr->psex];
                sprintf(effect_msg, _("女性の", "female "));
            } else {
                creature_ptr->psex = SEX_MALE;
                sp_ptr = &sex_info[creature_ptr->psex];
                sprintf(effect_msg, _("男性の", "male "));
            }
        }

        if ((power > randint0(30)) && one_in_(5)) {
            int tmp = 0;
            power -= 15;
            while (tmp < A_MAX) {
                if (one_in_(2)) {
                    (void)dec_stat(creature_ptr, tmp, randint1(6) + 6, one_in_(3));
                    power -= 1;
                }
                tmp++;
            }

            (void)dec_stat(creature_ptr, A_CHR, randint1(6), TRUE);

            if (effect_msg[0]) {
                char tmp_msg[10];
                sprintf(tmp_msg, _("%s", "%s "), effect_msg);
                sprintf(effect_msg, _("奇形の%s", "deformed %s "), tmp_msg);
            } else {
                sprintf(effect_msg, _("奇形の", "deformed "));
            }
        }

        while ((power > randint0(20)) && one_in_(10)) {
            power -= 10;

            if (!lose_mutation(creature_ptr, 0))
                msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }

        do {
            new_race = (player_race_type)randint0(MAX_RACES);
        } while ((new_race == creature_ptr->prace) || (new_race == RACE_ANDROID));

        change_race(creature_ptr, new_race, effect_msg);
    }

    if ((power > randint0(30)) && one_in_(6)) {
        int tmp = 0;
        power -= 20;
        msg_format(_("%sの構成が変化した！", "Your internal organs are rearranged!"), creature_ptr->prace == RACE_ANDROID ? "機械" : "内臓");

        while (tmp < A_MAX) {
            (void)dec_stat(creature_ptr, tmp, randint1(6) + 6, one_in_(3));
            tmp++;
        }
        if (one_in_(6)) {
            msg_print(_("現在の姿で生きていくのは困難なようだ！", "You find living difficult in your present form!"));
            take_hit(creature_ptr, DAMAGE_LOSELIFE, damroll(randint1(10), creature_ptr->lev), _("致命的な突然変異", "a lethal mutation"), -1);

            power -= 10;
        }
    }

    if ((power > randint0(20)) && one_in_(4)) {
        power -= 10;

        get_max_stats(creature_ptr);
        roll_hitdice(creature_ptr, 0L);
    }

    while ((power > randint0(15)) && one_in_(3)) {
        power -= 7;
        (void)gain_mutation(creature_ptr, 0);
    }

    if (power > randint0(5)) {
        power -= 5;
        do_poly_wounds(creature_ptr);
    }

    while (power > 0) {
        status_shuffle(creature_ptr);
        power--;
    }
}

/*
 * Gain experience
 */
void gain_exp_64(player_type *creature_ptr, s32b amount, u32b amount_frac)
{
    if (creature_ptr->is_dead)
        return;
    if (creature_ptr->prace == RACE_ANDROID)
        return;

    s64b_add(&(creature_ptr->exp), &(creature_ptr->exp_frac), amount, amount_frac);

    if (creature_ptr->exp < creature_ptr->max_exp) {
        creature_ptr->max_exp += amount / 5;
    }

    check_experience(creature_ptr);
}

/*
 * Gain experience
 */
void gain_exp(player_type *creature_ptr, s32b amount) { gain_exp_64(creature_ptr, amount, 0L); }

void calc_android_exp(player_type *creature_ptr)
{
    u32b total_exp = 0;
    if (creature_ptr->is_dead)
        return;
    if (creature_ptr->prace != RACE_ANDROID)
        return;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        object_type forge;
        object_type *q_ptr = &forge;
        u32b value, exp;
        DEPTH level = MAX(k_info[o_ptr->k_idx].level - 8, 1);

        if ((i == INVEN_RIGHT) || (i == INVEN_LEFT) || (i == INVEN_NECK) || (i == INVEN_LITE))
            continue;
        if (!o_ptr->k_idx)
            continue;

        object_wipe(q_ptr);
        object_copy(q_ptr, o_ptr);
        q_ptr->discount = 0;
        q_ptr->curse_flags = 0L;

        if (object_is_fixed_artifact(o_ptr)) {
            level = (level + MAX(a_info[o_ptr->name1].level - 8, 5)) / 2;
            level += MIN(20, a_info[o_ptr->name1].rarity / (a_info[o_ptr->name1].gen_flags & TRG_INSTA_ART ? 10 : 3));
        } else if (object_is_ego(o_ptr)) {
            level += MAX(3, (e_info[o_ptr->name2].rating - 5) / 2);
        } else if (o_ptr->art_name) {
            s32b total_flags = flag_cost(creature_ptr, o_ptr, o_ptr->pval);
            int fake_level;

            if (!object_is_weapon_ammo(o_ptr)) {
                if (total_flags < 15000)
                    fake_level = 10;
                else if (total_flags < 35000)
                    fake_level = 25;
                else
                    fake_level = 40;
            } else {
                if (total_flags < 20000)
                    fake_level = 10;
                else if (total_flags < 45000)
                    fake_level = 25;
                else
                    fake_level = 40;
            }

            level = MAX(level, (level + MAX(fake_level - 8, 5)) / 2 + 3);
        }

        value = object_value_real(creature_ptr, q_ptr);
        if (value <= 0)
            continue;
        if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI) && (creature_ptr->pseikaku != PERSONALITY_SEXY))
            value /= 32;
        if (value > 5000000L)
            value = 5000000L;
        if ((o_ptr->tval == TV_DRAG_ARMOR) || (o_ptr->tval == TV_CARD))
            level /= 2;

        if (object_is_artifact(o_ptr) || object_is_ego(o_ptr) || (o_ptr->tval == TV_DRAG_ARMOR) || ((o_ptr->tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM))
            || ((o_ptr->tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD)) || ((o_ptr->tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES))
            || ((o_ptr->tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)) || ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE))) {
            if (level > 65)
                level = 35 + (level - 65) / 5;
            else if (level > 35)
                level = 25 + (level - 35) / 3;
            else if (level > 15)
                level = 15 + (level - 15) / 2;
            exp = MIN(100000L, value) / 2 * level * level;
            if (value > 100000L)
                exp += (value - 100000L) / 8 * level * level;
        } else {
            exp = MIN(100000L, value) * level;
            if (value > 100000L)
                exp += (value - 100000L) / 4 * level;
        }
        if ((((i == INVEN_RARM) || (i == INVEN_LARM)) && (has_melee_weapon(creature_ptr, i))) || (i == INVEN_BOW))
            total_exp += exp / 48;
        else
            total_exp += exp / 16;
        if (i == INVEN_BODY)
            total_exp += exp / 32;
    }

    creature_ptr->exp = creature_ptr->max_exp = total_exp;
    check_experience(creature_ptr);
}

/*
 * Lose experience
 */
void lose_exp(player_type *creature_ptr, s32b amount)
{
    if (creature_ptr->prace == RACE_ANDROID)
        return;
    if (amount > creature_ptr->exp)
        amount = creature_ptr->exp;

    creature_ptr->exp -= amount;

    check_experience(creature_ptr);
}

/*
 * Drain experience
 * If resisted to draining, return FALSE
 */
bool drain_exp(player_type *creature_ptr, s32b drain, s32b slip, int hold_exp_prob)
{
    if (creature_ptr->prace == RACE_ANDROID)
        return FALSE;

    if (creature_ptr->hold_exp && (randint0(100) < hold_exp_prob)) {
        msg_print(_("しかし自己の経験値を守りきった！", "You keep hold of your experience!"));
        return FALSE;
    }

    if (creature_ptr->hold_exp) {
        msg_print(_("経験値を少し吸い取られた気がする！", "You feel your experience slipping away!"));
        lose_exp(creature_ptr, slip);
    } else {
        msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away!"));
        lose_exp(creature_ptr, drain);
    }

    return TRUE;
}

bool set_ultimate_res(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->ult_res && !do_dec) {
            if (creature_ptr->ult_res > v)
                return FALSE;
        } else if (!creature_ptr->ult_res) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->ult_res) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->ult_res = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    return TRUE;
}

bool set_tim_res_nether(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_res_nether && !do_dec) {
            if (creature_ptr->tim_res_nether > v)
                return FALSE;
        } else if (!creature_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether resistant!"));
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->tim_res_nether = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

bool set_tim_res_time(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_res_time && !do_dec) {
            if (creature_ptr->tim_res_time > v)
                return FALSE;
        } else if (!creature_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time resistant!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time resistant"));
            notice = TRUE;
        }
    }

    creature_ptr->tim_res_time = v;
    creature_ptr->redraw |= (PR_STATUS);
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*
 * Choose a warrior-mage elemental attack. -LM-
 */
bool choose_ele_attack(player_type *creature_ptr)
{
    if (!has_melee_weapon(creature_ptr, INVEN_RARM) && !has_melee_weapon(creature_ptr, INVEN_LARM)) {
        msg_format(_("武器を持たないと魔法剣は使えない。", "You cannot use temporary branding with no weapon."));
        return FALSE;
    }

    screen_save();
    int num = (creature_ptr->lev - 20) / 5;
    c_prt(TERM_RED, _("        a) 焼棄", "        a) Fire Brand"), 2, 14);

    if (num >= 2)
        c_prt(TERM_L_WHITE, _("        b) 凍結", "        b) Cold Brand"), 3, 14);
    else
        prt("", 3, 14);

    if (num >= 3)
        c_prt(TERM_GREEN, _("        c) 毒殺", "        c) Poison Brand"), 4, 14);
    else
        prt("", 4, 14);

    if (num >= 4)
        c_prt(TERM_L_DARK, _("        d) 溶解", "        d) Acid Brand"), 5, 14);
    else
        prt("", 5, 14);

    if (num >= 5)
        c_prt(TERM_BLUE, _("        e) 電撃", "        e) Elec Brand"), 6, 14);
    else
        prt("", 6, 14);

    prt("", 7, 14);
    prt("", 8, 14);
    prt("", 9, 14);

    prt("", 1, 0);
    prt(_("        どの元素攻撃をしますか？", "        Choose a temporary elemental brand "), 1, 14);

    char choice = inkey();

    if ((choice == 'a') || (choice == 'A'))
        set_ele_attack(creature_ptr, ATTACK_FIRE, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'b') || (choice == 'B')) && (num >= 2))
        set_ele_attack(creature_ptr, ATTACK_COLD, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'c') || (choice == 'C')) && (num >= 3))
        set_ele_attack(creature_ptr, ATTACK_POIS, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'd') || (choice == 'D')) && (num >= 4))
        set_ele_attack(creature_ptr, ATTACK_ACID, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else if (((choice == 'e') || (choice == 'E')) && (num >= 5))
        set_ele_attack(creature_ptr, ATTACK_ELEC, creature_ptr->lev / 2 + randint1(creature_ptr->lev / 2));
    else {
        msg_print(_("魔法剣を使うのをやめた。", "You cancel the temporary branding."));
        screen_load();
        return FALSE;
    }

    screen_load();
    return TRUE;
}

/*
 * Choose a elemental immune. -LM-
 */
bool choose_ele_immune(player_type *creature_ptr, TIME_EFFECT immune_turn)
{
    screen_save();

    c_prt(TERM_RED, _("        a) 火炎", "        a) Immunity to fire"), 2, 14);
    c_prt(TERM_L_WHITE, _("        b) 冷気", "        b) Immunity to cold"), 3, 14);
    c_prt(TERM_L_DARK, _("        c) 酸", "        c) Immunity to acid"), 4, 14);
    c_prt(TERM_BLUE, _("        d) 電撃", "        d) Immunity to elec"), 5, 14);

    prt("", 6, 14);
    prt("", 7, 14);
    prt("", 8, 14);
    prt("", 9, 14);

    prt("", 1, 0);
    prt(_("        どの元素の免疫をつけますか？", "        Choose a temporary elemental immunity "), 1, 14);

    char choice = inkey();

    if ((choice == 'a') || (choice == 'A'))
        set_ele_immune(creature_ptr, DEFENSE_FIRE, immune_turn);
    else if ((choice == 'b') || (choice == 'B'))
        set_ele_immune(creature_ptr, DEFENSE_COLD, immune_turn);
    else if ((choice == 'c') || (choice == 'C'))
        set_ele_immune(creature_ptr, DEFENSE_ACID, immune_turn);
    else if ((choice == 'd') || (choice == 'D'))
        set_ele_immune(creature_ptr, DEFENSE_ELEC, immune_turn);
    else {
        msg_print(_("免疫を付けるのをやめた。", "You cancel the temporary immunity."));
        screen_load();
        return FALSE;
    }

    screen_load();
    return TRUE;
}

bool drop_weapons(player_type *creature_ptr)
{
    INVENTORY_IDX slot = 0;
    object_type *o_ptr = NULL;

    if (creature_ptr->wild_mode)
        return FALSE;

    msg_print(NULL);
    if (has_melee_weapon(creature_ptr, INVEN_RARM)) {
        slot = INVEN_RARM;
        o_ptr = &creature_ptr->inventory_list[INVEN_RARM];

        if (has_melee_weapon(creature_ptr, INVEN_LARM) && one_in_(2)) {
            o_ptr = &creature_ptr->inventory_list[INVEN_LARM];
            slot = INVEN_LARM;
        }
    } else if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
        o_ptr = &creature_ptr->inventory_list[INVEN_LARM];
        slot = INVEN_LARM;
    }

    if (slot && !object_is_cursed(o_ptr)) {
        msg_print(_("武器を落としてしまった！", "You drop your weapon!"));
        drop_from_inventory(creature_ptr, slot, 1);
        return TRUE;
    }

    return FALSE;
}
