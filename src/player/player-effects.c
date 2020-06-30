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
#include "mind/mind-force-trainer.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-sniper.h"
#include "mind/stances-table.h"
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
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
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
    (void)set_tim_sh_force(creature_ptr, 0, TRUE);
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

