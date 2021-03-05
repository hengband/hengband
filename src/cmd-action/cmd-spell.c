﻿/*!
 * @brief 魔法のインターフェイスと発動 / Purpose: Do everything for each spell
 * @date 2013/12/31
 * @author
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "cmd-action/cmd-spell.h"
#include "action/action-limited.h"
#include "autopick/autopick-reader-writer.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-hook/hook-magic.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-info/avatar.h"
#include "player-info/self-info.h"
#include "player/attack-defense-types.h"
#include "player/eldritch-horror.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-names-table.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spell-info.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-summon.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#ifdef JP
#include "locale/japanese.h"
#endif

concptr KWD_DAM = _("損傷:", "dam ");
concptr KWD_RANGE = _("射程:", "rng ");
concptr KWD_DURATION = _("期間:", "dur ");
concptr KWD_SPHERE = _("範囲:", "range ");
concptr KWD_HEAL = _("回復:", "heal ");
concptr KWD_RANDOM = _("ランダム", "random");

/*!
 * 魔法領域フラグ管理テーブル /
 * Zangband uses this array instead of the spell flags table, as there
 * are 5 realms of magic, each with 4 spellbooks and 8 spells per book -- TY
 */
const u32b fake_spell_flags[4] = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

/*!
 * @brief
 * 魔法の効果を「キャプション:ダイス＋定数値」のフォーマットで出力する / Generate dice info string such as "foo 2d10"
 * @param str キャプション
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base)
{
    /* Fix value */
    if (!dice)
        return format("%s%d", str, base);

    /* Dice only */
    else if (!base)
        return format("%s%dd%d", str, dice, sides);

    /* Dice plus base value */
    else
        return format("%s%dd%d%+d", str, dice, sides, base);
}

/*!
 * @brief 魔法によるダメージを出力する / Generate damage-dice info string such as "dam 2d10"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_damage(DICE_NUMBER dice, DICE_SID sides, int base) { return info_string_dice(_("損傷:", "dam "), dice, sides, base); }

/*!
 * @brief 魔法の効果時間を出力する / Generate duration info string such as "dur 20+1d20"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_duration(int base, DICE_SID sides) { return format(_("期間:%d+1d%d", "dur %d+1d%d"), base, sides); }

/*!
 * @brief 魔法の効果範囲を出力する / Generate range info string such as "range 5"
 * @param range 効果範囲
 * @return フォーマットに従い整形された文字列
 */
concptr info_range(POSITION range) { return format(_("範囲:%d", "range %d"), range); }

/*!
 * @brief 魔法による回復量を出力する / Generate heal info string such as "heal 2d8"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_heal(DICE_NUMBER dice, DICE_SID sides, int base) { return info_string_dice(_("回復:", "heal "), dice, sides, base); }

/*!
 * @brief 魔法効果発動までの遅延ターンを出力する / Generate delay info string such as "delay 15+1d15"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_delay(int base, DICE_SID sides) { return format(_("遅延:%d+1d%d", "delay %d+1d%d"), base, sides); }

/*!
 * @brief 魔法によるダメージを出力する(固定値＆複数回処理) / Generate multiple-damage info string such as "dam 25 each"
 * @param dam 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_multi_damage(HIT_POINT dam) { return format(_("損傷:各%d", "dam %d each"), dam); }

/*!
 * @brief 魔法によるダメージを出力する(ダイスのみ＆複数回処理) / Generate multiple-damage-dice info string such as "dam 5d2 each"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
concptr info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides) { return format(_("損傷:各%dd%d", "dam %dd%d each"), dice, sides); }

/*!
 * @brief 魔法による一般的な効力値を出力する（固定値） / Generate power info string such as "power 100"
 * @param power 固定値
 * @return フォーマットに従い整形された文字列
 */
concptr info_power(int power) { return format(_("効力:%d", "power %d"), power); }

/*!
 * @brief 魔法による一般的な効力値を出力する（ダイス値） / Generate power info string such as "power 100"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
/*
 * Generate power info string such as "power 1d100"
 */
concptr info_power_dice(DICE_NUMBER dice, DICE_SID sides) { return format(_("効力:%dd%d", "power %dd%d"), dice, sides); }

/*!
 * @brief 魔法の効果半径を出力する / Generate radius info string such as "rad 100"
 * @param rad 効果半径
 * @return フォーマットに従い整形された文字列
 */
concptr info_radius(POSITION rad) { return format(_("半径:%d", "rad %d"), rad); }

/*!
 * @brief 魔法効果の限界重量を出力する / Generate weight info string such as "max wgt 15"
 * @param weight 最大重量
 * @return フォーマットに従い整形された文字列
 */
concptr info_weight(WEIGHT weight)
{
#ifdef JP
    return format("最大重量:%d.%dkg", lbtokg1(weight), lbtokg2(weight));
#else
    return format("max wgt %d", weight / 10);
#endif
}

/*!
 * @brief 魔法が利用可能かどうかを返す /
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 * @param spell 呪文ID
 * @param learned 使用可能な判定ならばTRUE、学習可能かどうかの判定ならばFALSE
 * @param study_pray 祈りの学習判定目的ならばTRUE
 * @param use_realm 魔法領域ID
 * @return 失敗率(%)
 */
static bool spell_okay(player_type *caster_ptr, int spell, bool learned, bool study_pray, int use_realm)
{
    const magic_type *s_ptr;

    /* Access the spell */
    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[use_realm - 1][spell];
    }

    /* Spell is illegal */
    if (s_ptr->slevel > caster_ptr->lev)
        return FALSE;

    /* Spell is forgotten */
    if ((use_realm == caster_ptr->realm2) ? (caster_ptr->spell_forgotten2 & (1UL << spell)) : (caster_ptr->spell_forgotten1 & (1UL << spell))) {
        /* Never okay */
        return FALSE;
    }

    if (caster_ptr->pclass == CLASS_SORCERER)
        return TRUE;
    if (caster_ptr->pclass == CLASS_RED_MAGE)
        return TRUE;

    /* Spell is learned */
    if ((use_realm == caster_ptr->realm2) ? (caster_ptr->spell_learned2 & (1UL << spell)) : (caster_ptr->spell_learned1 & (1UL << spell))) {
        /* Always true */
        return (!study_pray);
    }

    /* Okay to study, not to cast */
    return (!learned);
}

/*!
 * @brief 領域魔法の閲覧、学習、使用選択するインターフェイス処理
 * Allow user to choose a spell/prayer from the given book.
 * @param sn 選択した魔法IDを返す参照ポインタ
 * @param prompt 魔法を利用する際の動詞表記
 * @param sval 魔道書のsval
 * @param learned 閲覧/使用選択ならばTRUE、学習処理ならFALSE
 * @param use_realm 魔法領域ID
 * @return
 * <pre>
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 * </pre>
 */
static int get_spell(player_type *caster_ptr, SPELL_IDX *sn, concptr prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, REALM_IDX use_realm)
{
    int i;
    SPELL_IDX spell = -1;
    int num = 0;
    int ask = TRUE;
    MANA_POINT need_mana;
    SPELL_IDX spells[64];
    bool flag, redraw, okay;
    char choice;
    const magic_type *s_ptr;
    char out_val[160];
    concptr p;
    COMMAND_CODE code;
#ifdef JP
    char jverb_buf[128];
#endif
    int menu_line = (use_menu ? 1 : 0);

    /* Get the spell, if available */
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        /* Verify the spell */
        if (spell_okay(caster_ptr, *sn, learned, FALSE, use_realm)) {
            /* Success */
            return TRUE;
        }
    }

    p = spell_category_name(mp_ptr->spell_book);

    /* Extract spells */
    for (spell = 0; spell < 32; spell++) {
        /* Check for this spell */
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            /* Collect this spell */
            spells[num++] = spell;
        }
    }

    /* Assume no usable spells */
    okay = FALSE;

    /* Assume no spells available */
    (*sn) = -2;

    /* Check for "okay" spells */
    for (i = 0; i < num; i++) {
        /* Look for "okay" spells */
        if (spell_okay(caster_ptr, spells[i], learned, FALSE, use_realm))
            okay = TRUE;
    }

    /* No "okay" spells */
    if (!okay)
        return FALSE;
    if (((use_realm) != caster_ptr->realm1) && ((use_realm) != caster_ptr->realm2) && (caster_ptr->pclass != CLASS_SORCERER)
        && (caster_ptr->pclass != CLASS_RED_MAGE))
        return FALSE;
    if (((caster_ptr->pclass == CLASS_SORCERER) || (caster_ptr->pclass == CLASS_RED_MAGE)) && !is_magic(use_realm))
        return FALSE;
    if ((caster_ptr->pclass == CLASS_RED_MAGE) && ((use_realm) != REALM_ARCANE) && (sval > 1))
        return FALSE;

    /* Assume cancelled */
    *sn = (-1);

    flag = FALSE;
    redraw = FALSE;

    caster_ptr->window_flags |= (PW_SPELL);
    handle_stuff(caster_ptr);

    /* Build a prompt (accept all spells) */
#ifdef JP
    jverb(prompt, jverb_buf, JVERB_AND);
    (void)strnfmt(out_val, 78, "(%^s:%c-%c, '*'で一覧, ESCで中断) どの%sを%^sますか? ", p, I2A(0), I2A(num - 1), p, jverb_buf);
#else
    (void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ", p, I2A(0), I2A(num - 1), prompt, p);
#endif

    choice = (always_show_list || use_menu) ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE)
            choice = ' ';
        else if (!get_com(out_val, &choice, TRUE))
            break;

        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0': {
                screen_load();
                return FALSE;
            }

            case '8':
            case 'k':
            case 'K': {
                menu_line += (num - 1);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                menu_line++;
                break;
            }

            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                i = menu_line - 1;
                ask = FALSE;
                break;
            }
            }
            if (menu_line > num)
                menu_line -= num;
            /* Display a list of spells */
            print_spells(caster_ptr, menu_line, spells, num, 1, 15, use_realm);
            if (ask)
                continue;
        } else {
            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?')) {
                /* Show the list */
                if (!redraw) {
                    redraw = TRUE;
                    screen_save();

                    /* Display a list of spells */
                    print_spells(caster_ptr, menu_line, spells, num, 1, 15, use_realm);
                }

                /* Hide the list */
                else {
                    if (use_menu)
                        continue;

                    /* Hide list */
                    redraw = FALSE;
                    screen_load();
                }

                /* Redo asking */
                continue;
            }

            /* Note verify */
            ask = (isupper(choice));

            /* Lowercase */
            if (ask)
                choice = (char)tolower(choice);

            /* Extract request */
            i = (islower(choice) ? A2I(choice) : -1);
        }

        /* Totally Illegal */
        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = spells[i];

        /* Require "okay" spells */
        if (!spell_okay(caster_ptr, spell, learned, FALSE, use_realm)) {
            bell();
#ifdef JP
            msg_format("その%sを%sことはできません。", p, prompt);
#else
            msg_format("You may not %s that %s.", prompt, p);
#endif

            continue;
        }

        /* Verify it */
        if (ask) {
            char tmp_val[160];

            /* Access the spell */
            if (!is_magic(use_realm)) {
                s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
            } else {
                s_ptr = &mp_ptr->info[use_realm - 1][spell];
            }

            /* Extract mana consumption rate */
            if (use_realm == REALM_HISSATSU) {
                need_mana = s_ptr->smana;
            } else {
                need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, use_realm);
            }

            /* Prompt */
#ifdef JP
            jverb(prompt, jverb_buf, JVERB_AND);
            /* 英日切り替え機能に対応 */
            (void)strnfmt(tmp_val, 78, "%s(MP%d, 失敗率%d%%)を%sますか? ", exe_spell(caster_ptr, use_realm, spell, SPELL_NAME), need_mana,
                spell_chance(caster_ptr, spell, use_realm), jverb_buf);
#else
            (void)strnfmt(tmp_val, 78, "%^s %s (%d mana, %d%% fail)? ", prompt, exe_spell(caster_ptr, use_realm, spell, SPELL_NAME), need_mana,
                spell_chance(caster_ptr, spell, use_realm));
#endif

            /* Belay that order */
            if (!get_check(tmp_val))
                continue;
        }

        /* Stop the loop */
        flag = TRUE;
    }

    if (redraw)
        screen_load();

    caster_ptr->window_flags |= (PW_SPELL);
    handle_stuff(caster_ptr);

    /* Abort if needed */
    if (!flag)
        return FALSE;

    /* Save the choice */
    (*sn) = spell;

    repeat_push((COMMAND_CODE)spell);

    /* Success */
    return TRUE;
}

/*!
 * @brief プレイヤーの職業が練気術師の時、領域魔法と練気術を切り換える処理のインターフェイス
 * @param browse_only 魔法と技能の閲覧を行うならばTRUE
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
static void confirm_use_force(player_type *caster_ptr, bool browse_only)
{
    char which;
    COMMAND_CODE code;

    /* Get the item index */
    if (repeat_pull(&code) && (code == INVEN_FORCE)) {
        browse_only ? do_cmd_mind_browse(caster_ptr) : do_cmd_mind(caster_ptr);
        return;
    }

    /* Show the prompt */
    prt(_("('w'練気術, ESC) 'w'かESCを押してください。 ", "(w for the Force, ESC) Hit 'w' or ESC. "), 0, 0);

    while (TRUE) {
        /* Get a key */
        which = inkey();

        if (which == ESCAPE)
            break;
        else if (which == 'w') {
            repeat_push(INVEN_FORCE);
            break;
        }
    }

    /* Clear the prompt line */
    prt("", 0, 0);

    if (which == 'w') {
        browse_only ? do_cmd_mind_browse(caster_ptr) : do_cmd_mind(caster_ptr);
    }
}

/*!
 * @brief プレイヤーの魔法と技能を閲覧するコマンドのメインルーチン /
 * Peruse the spells/prayers in a book
 * @return なし
 * @details
 * <pre>
 * Note that *all* spells in the book are listed
 *
 * Note that browsing is allowed while confused or blind,
 * and in the dark, primarily to allow browsing in stores.
 * </pre>
 */
void do_cmd_browse(player_type *caster_ptr)
{
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    REALM_IDX use_realm = 0;
    int j, line;
    SPELL_IDX spell = -1;
    int num = 0;

    SPELL_IDX spells[64];
    char temp[62 * 4];

    object_type *o_ptr;

    concptr q, s;
    tval_type tval = TV_NONE;

    /* Warriors are illiterate */
    if (!(caster_ptr->realm1 || caster_ptr->realm2) && (caster_ptr->pclass != CLASS_SORCERER) && (caster_ptr->pclass != CLASS_RED_MAGE)) {
        msg_print(_("本を読むことができない！", "You cannot read books!"));
        return;
    }

    if (caster_ptr->special_defense & KATA_MUSOU) {
        set_action(caster_ptr, ACTION_NONE);
    }

    if (caster_ptr->pclass == CLASS_FORCETRAINER) {
        if (player_has_no_spellbooks(caster_ptr)) {
            confirm_use_force(caster_ptr, TRUE);
            return;
        }
    }

    /* Restrict choices to "useful" books */
    if (caster_ptr->realm2 == REALM_NONE)
        tval = mp_ptr->spell_book;
    else
        item_tester_hook = item_tester_learn_spell;

    q = _("どの本を読みますか? ", "Browse which book? ");
    s = _("読める本がない。", "You have no books that you can read.");

    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | (caster_ptr->pclass == CLASS_FORCETRAINER ? USE_FORCE : 0)), tval);

    item_tester_hook = NULL;
    if (!o_ptr) {
        if (item == INVEN_FORCE) /* the_force */
        {
            do_cmd_mind_browse(caster_ptr);
            return;
        }
        return;
    }

    /* Access the item's sval */
    sval = o_ptr->sval;

    use_realm = tval2realm(o_ptr->tval);

    /* Track the object kind */
    object_kind_track(caster_ptr, o_ptr->k_idx);
    handle_stuff(caster_ptr);

    /* Extract spells */
    for (spell = 0; spell < 32; spell++) {
        /* Check for this spell */
        if ((fake_spell_flags[sval] & (1UL << spell))) {
            /* Collect this spell */
            spells[num++] = spell;
        }
    }

    screen_save();
    prt("", 0, 0);

    /* Keep browsing spells.  Exit browsing on cancel. */
    while (TRUE) {
        /* Ask for a spell, allow cancel */
        if (!get_spell(caster_ptr, &spell, _("読む", "browse"), o_ptr->sval, TRUE, use_realm)) {
            /* If cancelled, leave immediately. */
            if (spell == -1)
                break;

            /* Display a list of spells */
            print_spells(caster_ptr, 0, spells, num, 1, 15, use_realm);

            /* Notify that there's nothing to see, and wait. */
            if (use_realm == REALM_HISSATSU)
                prt(_("読める技がない。", "No techniques to browse."), 0, 0);
            else
                prt(_("読める呪文がない。", "No spells to browse."), 0, 0);
            (void)inkey();

            screen_load();

            return;
        }

        /* Clear lines, position cursor  (really should use strlen here) */
        term_erase(14, 14, 255);
        term_erase(14, 13, 255);
        term_erase(14, 12, 255);
        term_erase(14, 11, 255);

        shape_buffer(exe_spell(caster_ptr, use_realm, spell, SPELL_DESC), 62, temp, sizeof(temp));

        for (j = 0, line = 11; temp[j]; j += 1 + strlen(&temp[j])) {
            prt(&temp[j], line, 15);
            line++;
        }
    }
    screen_load();
}

/*!
 * @brief プレイヤーの第二魔法領域を変更する /
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param next_realm 変更先の魔法領域ID
 * @return なし
 */
static void change_realm2(player_type *caster_ptr, REALM_IDX next_realm)
{
    int i, j = 0;
    char tmp[80];

    for (i = 0; i < 64; i++) {
        caster_ptr->spell_order[j] = caster_ptr->spell_order[i];
        if (caster_ptr->spell_order[i] < 32)
            j++;
    }
    for (; j < 64; j++)
        caster_ptr->spell_order[j] = 99;

    for (i = 32; i < 64; i++) {
        caster_ptr->spell_exp[i] = SPELL_EXP_UNSKILLED;
    }
    caster_ptr->spell_learned2 = 0L;
    caster_ptr->spell_worked2 = 0L;
    caster_ptr->spell_forgotten2 = 0L;

    sprintf(tmp, _("魔法の領域を%sから%sに変更した。", "changed magic realm from %s to %s."), realm_names[caster_ptr->realm2], realm_names[next_realm]);
    exe_write_diary(caster_ptr, DIARY_DESCRIPTION, 0, tmp);
    caster_ptr->old_realm |= 1U << (caster_ptr->realm2 - 1);
    caster_ptr->realm2 = next_realm;

    caster_ptr->update |= (PU_REORDER);
    caster_ptr->update |= (PU_SPELLS);
    handle_stuff(caster_ptr);

    /* Load an autopick preference file */
    autopick_load_pref(caster_ptr, FALSE);
}

/*!
 * @brief 魔法を学習するコマンドのメインルーチン /
 * Study a book to gain a new spell/prayer
 * @return なし
 */
void do_cmd_study(player_type *caster_ptr)
{
    int i;
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    int increment = 0;
    bool learned = FALSE;

    /* Spells of realm2 will have an increment of +32 */
    SPELL_IDX spell = -1;
    concptr p = spell_category_name(mp_ptr->spell_book);
    object_type *o_ptr;
    concptr q, s;
    tval_type tval = TV_NONE;

    if (!caster_ptr->realm1) {
        msg_print(_("本を読むことができない！", "You cannot read books!"));
        return;
    }

    if (cmd_limit_blind(caster_ptr))
        return;
    if (cmd_limit_confused(caster_ptr))
        return;

    if (!(caster_ptr->new_spells)) {
        msg_format(_("新しい%sを覚えることはできない！", "You cannot learn any new %ss!"), p);
        return;
    }

    if (caster_ptr->special_defense & KATA_MUSOU) {
        set_action(caster_ptr, ACTION_NONE);
    }

#ifdef JP
    if (caster_ptr->new_spells < 10) {
        msg_format("あと %d つの%sを学べる。", caster_ptr->new_spells, p);
    } else {
        msg_format("あと %d 個の%sを学べる。", caster_ptr->new_spells, p);
    }
#else
    msg_format("You can learn %d new %s%s.", caster_ptr->new_spells, p, (caster_ptr->new_spells == 1 ? "" : "s"));
#endif

    msg_print(NULL);

    /* Restrict choices to "useful" books */
    if (caster_ptr->realm2 == REALM_NONE)
        tval = mp_ptr->spell_book;
    else
        item_tester_hook = item_tester_learn_spell;

    q = _("どの本から学びますか? ", "Study which book? ");
    s = _("読める本がない。", "You have no books that you can read.");

    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), tval);

    if (!o_ptr)
        return;

    /* Access the item's sval */
    sval = o_ptr->sval;

    if (o_ptr->tval == get_realm2_book(caster_ptr))
        increment = 32;
    else if (o_ptr->tval != get_realm1_book(caster_ptr)) {
        if (!get_check(_("本当に魔法の領域を変更しますか？", "Really, change magic realm? ")))
            return;
        change_realm2(caster_ptr, tval2realm(o_ptr->tval));
        increment = 32;
    }

    /* Track the object kind */
    object_kind_track(caster_ptr, o_ptr->k_idx);
    handle_stuff(caster_ptr);

    /* Mage -- Learn a selected spell */
    if (mp_ptr->spell_book != TV_LIFE_BOOK) {
        /* Ask for a spell, allow cancel */
        if (!get_spell(caster_ptr, &spell, _("学ぶ", "study"), sval, FALSE, o_ptr->tval - TV_LIFE_BOOK + 1) && (spell == -1))
            return;
    }

    /* Priest -- Learn a random prayer */
    else {
        int k = 0;
        int gift = -1;

        /* Extract spells */
        for (spell = 0; spell < 32; spell++) {
            /* Check spells in the book */
            if ((fake_spell_flags[sval] & (1UL << spell))) {
                /* Skip non "okay" prayers */
                if (!spell_okay(caster_ptr, spell, FALSE, TRUE, (increment ? caster_ptr->realm2 : caster_ptr->realm1)))
                    continue;

                /* Hack -- Prepare the randomizer */
                k++;

                /* Hack -- Apply the randomizer */
                if (one_in_(k))
                    gift = spell;
            }
        }

        /* Accept gift */
        spell = gift;
    }

    /* Nothing to study */
    if (spell < 0) {
        msg_format(_("その本には学ぶべき%sがない。", "You cannot learn any %ss in that book."), p);

        /* Abort */
        return;
    }

    if (increment)
        spell += increment;

    /* Learn the spell */
    if (spell < 32) {
        if (caster_ptr->spell_learned1 & (1UL << spell))
            learned = TRUE;
        else
            caster_ptr->spell_learned1 |= (1UL << spell);
    } else {
        if (caster_ptr->spell_learned2 & (1UL << (spell - 32)))
            learned = TRUE;
        else
            caster_ptr->spell_learned2 |= (1UL << (spell - 32));
    }

    if (learned) {
        int max_exp = (spell < 32) ? SPELL_EXP_MASTER : SPELL_EXP_EXPERT;
        int old_exp = caster_ptr->spell_exp[spell];
        int new_rank = EXP_LEVEL_UNSKILLED;
        concptr name = exe_spell(caster_ptr, increment ? caster_ptr->realm2 : caster_ptr->realm1, spell % 32, SPELL_NAME);

        if (old_exp >= max_exp) {
            msg_format(_("その%sは完全に使いこなせるので学ぶ必要はない。", "You don't need to study this %s anymore."), p);
            return;
        }
#ifdef JP
        if (!get_check(format("%sの%sをさらに学びます。よろしいですか？", name, p)))
#else
        if (!get_check(format("You will study a %s of %s again. Are you sure? ", p, name)))
#endif
        {
            return;
        } else if (old_exp >= SPELL_EXP_EXPERT) {
            caster_ptr->spell_exp[spell] = SPELL_EXP_MASTER;
            new_rank = EXP_LEVEL_MASTER;
        } else if (old_exp >= SPELL_EXP_SKILLED) {
            if (spell >= 32)
                caster_ptr->spell_exp[spell] = SPELL_EXP_EXPERT;
            else
                caster_ptr->spell_exp[spell] += SPELL_EXP_EXPERT - SPELL_EXP_SKILLED;
            new_rank = EXP_LEVEL_EXPERT;
        } else if (old_exp >= SPELL_EXP_BEGINNER) {
            caster_ptr->spell_exp[spell] = SPELL_EXP_SKILLED + (old_exp - SPELL_EXP_BEGINNER) * 2 / 3;
            new_rank = EXP_LEVEL_SKILLED;
        } else {
            caster_ptr->spell_exp[spell] = SPELL_EXP_BEGINNER + old_exp / 3;
            new_rank = EXP_LEVEL_BEGINNER;
        }
        msg_format(_("%sの熟練度が%sに上がった。", "Your proficiency of %s is now %s rank."), name, exp_level_str[new_rank]);
    } else {
        /* Find the next open entry in "caster_ptr->spell_order[]" */
        for (i = 0; i < 64; i++) {
            /* Stop at the first empty space */
            if (caster_ptr->spell_order[i] == 99)
                break;
        }

        /* Add the spell to the known list */
        caster_ptr->spell_order[i++] = spell;

        /* Mention the result */
#ifdef JP
        /* 英日切り替え機能に対応 */
        if (mp_ptr->spell_book == TV_MUSIC_BOOK) {
            msg_format("%sを学んだ。", exe_spell(caster_ptr, increment ? caster_ptr->realm2 : caster_ptr->realm1, spell % 32, SPELL_NAME));
        } else {
            msg_format("%sの%sを学んだ。", exe_spell(caster_ptr, increment ? caster_ptr->realm2 : caster_ptr->realm1, spell % 32, SPELL_NAME), p);
        }
#else
        msg_format("You have learned the %s of %s.", p, exe_spell(caster_ptr, increment ? caster_ptr->realm2 : caster_ptr->realm1, spell % 32, SPELL_NAME));
#endif
    }

    take_turn(caster_ptr, 100);

    switch (mp_ptr->spell_book) {
    case TV_LIFE_BOOK:
        chg_virtue(caster_ptr, V_FAITH, 1);
        break;
    case TV_DEATH_BOOK:
        chg_virtue(caster_ptr, V_UNLIFE, 1);
        break;
    case TV_NATURE_BOOK:
        chg_virtue(caster_ptr, V_NATURE, 1);
        break;
    default:
        chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
        break;
    }

    sound(SOUND_STUDY);

    /* One less spell available */
    caster_ptr->learned_spells++;

    /* Update Study */
    caster_ptr->update |= (PU_SPELLS);
    update_creature(caster_ptr);

    /* Redraw object recall */
    caster_ptr->window_flags |= (PW_OBJECT);
}

/*!
 * @brief 魔法を詠唱するコマンドのメインルーチン /
 * Cast a spell
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_cast(player_type *caster_ptr)
{
    OBJECT_IDX item;
    OBJECT_SUBTYPE_VALUE sval;
    SPELL_IDX spell;
    REALM_IDX realm;
    int chance;
    int increment = 0;
    REALM_IDX use_realm;
    MANA_POINT need_mana;

    concptr prayer;
    object_type *o_ptr;
    const magic_type *s_ptr;
    concptr q, s;

    bool over_exerted = FALSE;

    /* Require spell ability */
    if (!caster_ptr->realm1 && (caster_ptr->pclass != CLASS_SORCERER) && (caster_ptr->pclass != CLASS_RED_MAGE)) {
        msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
        return;
    }

    if (caster_ptr->blind || no_lite(caster_ptr)) {
        if (caster_ptr->pclass == CLASS_FORCETRAINER)
            confirm_use_force(caster_ptr, FALSE);
        else {
            msg_print(_("目が見えない！", "You cannot see!"));
            flush();
        }
        return;
    }

    if (cmd_limit_confused(caster_ptr))
        return;
    if (caster_ptr->realm1 == REALM_HEX) {
        if (hex_spell_fully(caster_ptr)) {
            bool flag = FALSE;
            msg_print(_("これ以上新しい呪文を詠唱することはできない。", "Can not cast more spells."));
            flush();
            if (caster_ptr->lev >= 35)
                flag = stop_hex_spell(caster_ptr);
            if (!flag)
                return;
        }
    }

    if (caster_ptr->pclass == CLASS_FORCETRAINER) {
        if (player_has_no_spellbooks(caster_ptr)) {
            confirm_use_force(caster_ptr, FALSE);
            return;
        }
    }

    prayer = spell_category_name(mp_ptr->spell_book);

    q = _("どの呪文書を使いますか? ", "Use which book? ");
    s = _("呪文書がない！", "You have no spell books!");

    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | (caster_ptr->pclass == CLASS_FORCETRAINER ? USE_FORCE : 0)), mp_ptr->spell_book);
    if (!o_ptr) {
        if (item == INVEN_FORCE) /* the_force */
        {
            do_cmd_mind(caster_ptr);
            return;
        }
        return;
    }

    /* Access the item's sval */
    sval = o_ptr->sval;

    if ((caster_ptr->pclass != CLASS_SORCERER) && (caster_ptr->pclass != CLASS_RED_MAGE) && (o_ptr->tval == get_realm2_book(caster_ptr)))
        increment = 32;

    /* Track the object kind */
    object_kind_track(caster_ptr, o_ptr->k_idx);
    handle_stuff(caster_ptr);

    if ((caster_ptr->pclass == CLASS_SORCERER) || (caster_ptr->pclass == CLASS_RED_MAGE))
        realm = o_ptr->tval - TV_LIFE_BOOK + 1;
    else if (increment)
        realm = caster_ptr->realm2;
    else
        realm = caster_ptr->realm1;

        /* Ask for a spell */
#ifdef JP
    if (!get_spell(caster_ptr, &spell, ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "詠唱する" : (mp_ptr->spell_book == TV_MUSIC_BOOK) ? "歌う" : "唱える"), sval,
            TRUE, realm)) {
        if (spell == -2)
            msg_format("その本には知っている%sがない。", prayer);
        return;
    }
#else
    if (!get_spell(caster_ptr, &spell, ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"), sval, TRUE, realm)) {
        if (spell == -2)
            msg_format("You don't know any %ss in that book.", prayer);
        return;
    }
#endif

    use_realm = tval2realm(o_ptr->tval);
    if (use_realm == REALM_HEX) {
        if (hex_spelling(caster_ptr, spell)) {
            msg_print(_("その呪文はすでに詠唱中だ。", "You are already casting it."));
            return;
        }
    }

    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[realm - 1][spell];
    }

    /* Extract mana consumption rate */
    need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, realm);

    /* Verify "dangerous" spells */
    if (need_mana > caster_ptr->csp) {
        if (flush_failure)
            flush();

            /* Warning */
#ifdef JP
        msg_format("その%sを%sのに十分なマジックポイントがない。", prayer,
            ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "詠唱する" : (mp_ptr->spell_book == TV_LIFE_BOOK) ? "歌う" : "唱える"));
#else
        msg_format("You do not have enough mana to %s this %s.", ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"), prayer);
#endif

        if (!over_exert)
            return;

        /* Verify */
        if (!get_check_strict(caster_ptr, _("それでも挑戦しますか? ", "Attempt it anyway? "), CHECK_OKAY_CANCEL))
            return;
    }

    /* Spell failure chance */
    chance = spell_chance(caster_ptr, spell, use_realm);

    /* Sufficient mana */
    if (need_mana <= caster_ptr->csp) {
        /* Use some mana */
        caster_ptr->csp -= need_mana;
    } else
        over_exerted = TRUE;
    caster_ptr->redraw |= (PR_MANA);

    /* Failed spell */
    if (randint0(100) < chance) {
        if (flush_failure)
            flush();

        msg_format(_("%sをうまく唱えられなかった！", "You failed to get the %s off!"), prayer);
        sound(SOUND_FAIL);

        switch (realm) {
        case REALM_LIFE:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_VITALITY, -1);
            break;
        case REALM_DEATH:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_UNLIFE, -1);
            break;
        case REALM_NATURE:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_NATURE, -1);
            break;
        case REALM_DAEMON:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_JUSTICE, 1);
            break;
        case REALM_CRUSADE:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_JUSTICE, -1);
            break;
        case REALM_HEX:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_COMPASSION, -1);
            break;
        default:
            if (randint1(100) < chance)
                chg_virtue(caster_ptr, V_KNOWLEDGE, -1);
            break;
        }

        /* Failure casting may activate some side effect */
        exe_spell(caster_ptr, realm, spell, SPELL_FAIL);

        if ((o_ptr->tval == TV_CHAOS_BOOK) && (randint1(100) < spell)) {
            msg_print(_("カオス的な効果を発生した！", "You produce a chaotic effect!"));
            wild_magic(caster_ptr, spell);
        } else if ((o_ptr->tval == TV_DEATH_BOOK) && (randint1(100) < spell)) {
            if ((sval == 3) && one_in_(2)) {
                sanity_blast(caster_ptr, 0, TRUE);
            } else {
                msg_print(_("痛い！", "It hurts!"));
                take_hit(caster_ptr, DAMAGE_LOSELIFE, damroll(o_ptr->sval + 1, 6), _("暗黒魔法の逆流", "a miscast Death spell"), -1);

                if ((spell > 15) && one_in_(6) && !caster_ptr->hold_exp)
                    lose_exp(caster_ptr, spell * 250);
            }
        } else if ((o_ptr->tval == TV_MUSIC_BOOK) && (randint1(200) < spell)) {
            msg_print(_("いやな音が響いた", "An infernal sound echoed."));
            aggravate_monsters(caster_ptr, 0);
        }
        if (randint1(100) >= chance)
            chg_virtue(caster_ptr, V_CHANCE, -1);
    }

    /* Process spell */
    else {
        /* Canceled spells cost neither a turn nor mana */
        if (!exe_spell(caster_ptr, realm, spell, SPELL_CAST))
            return;

        if (randint1(100) < chance)
            chg_virtue(caster_ptr, V_CHANCE, 1);

        /* A spell was cast */
        if (!(increment ? (caster_ptr->spell_worked2 & (1UL << spell)) : (caster_ptr->spell_worked1 & (1UL << spell))) && (caster_ptr->pclass != CLASS_SORCERER)
            && (caster_ptr->pclass != CLASS_RED_MAGE)) {
            int e = s_ptr->sexp;

            /* The spell worked */
            if (realm == caster_ptr->realm1) {
                caster_ptr->spell_worked1 |= (1UL << spell);
            } else {
                caster_ptr->spell_worked2 |= (1UL << spell);
            }

            gain_exp(caster_ptr, e * s_ptr->slevel);
            caster_ptr->window_flags |= (PW_OBJECT);

            switch (realm) {
            case REALM_LIFE:
                chg_virtue(caster_ptr, V_TEMPERANCE, 1);
                chg_virtue(caster_ptr, V_COMPASSION, 1);
                chg_virtue(caster_ptr, V_VITALITY, 1);
                chg_virtue(caster_ptr, V_DILIGENCE, 1);
                break;
            case REALM_DEATH:
                chg_virtue(caster_ptr, V_UNLIFE, 1);
                chg_virtue(caster_ptr, V_JUSTICE, -1);
                chg_virtue(caster_ptr, V_FAITH, -1);
                chg_virtue(caster_ptr, V_VITALITY, -1);
                break;
            case REALM_DAEMON:
                chg_virtue(caster_ptr, V_JUSTICE, -1);
                chg_virtue(caster_ptr, V_FAITH, -1);
                chg_virtue(caster_ptr, V_HONOUR, -1);
                chg_virtue(caster_ptr, V_TEMPERANCE, -1);
                break;
            case REALM_CRUSADE:
                chg_virtue(caster_ptr, V_FAITH, 1);
                chg_virtue(caster_ptr, V_JUSTICE, 1);
                chg_virtue(caster_ptr, V_SACRIFICE, 1);
                chg_virtue(caster_ptr, V_HONOUR, 1);
                break;
            case REALM_NATURE:
                chg_virtue(caster_ptr, V_NATURE, 1);
                chg_virtue(caster_ptr, V_HARMONY, 1);
                break;
            case REALM_HEX:
                chg_virtue(caster_ptr, V_JUSTICE, -1);
                chg_virtue(caster_ptr, V_FAITH, -1);
                chg_virtue(caster_ptr, V_HONOUR, -1);
                chg_virtue(caster_ptr, V_COMPASSION, -1);
                break;
            default:
                chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
                break;
            }
        }
        switch (realm) {
        case REALM_LIFE:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_TEMPERANCE, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_COMPASSION, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_VITALITY, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_DILIGENCE, 1);
            break;
        case REALM_DEATH:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_UNLIFE, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_JUSTICE, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_FAITH, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_VITALITY, -1);
            break;
        case REALM_DAEMON:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_JUSTICE, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_FAITH, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_HONOUR, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_TEMPERANCE, -1);
            break;
        case REALM_CRUSADE:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_FAITH, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_JUSTICE, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_SACRIFICE, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_HONOUR, 1);
            break;
        case REALM_NATURE:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_NATURE, 1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_HARMONY, 1);
            break;
        case REALM_HEX:
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_JUSTICE, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_FAITH, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_HONOUR, -1);
            if (randint1(100 + caster_ptr->lev) < need_mana)
                chg_virtue(caster_ptr, V_COMPASSION, -1);
            break;
        }
        if (mp_ptr->spell_xtra & MAGIC_GAIN_EXP) {
            s16b cur_exp = caster_ptr->spell_exp[(increment ? 32 : 0) + spell];
            s16b exp_gain = 0;

            if (cur_exp < SPELL_EXP_BEGINNER)
                exp_gain += 60;
            else if (cur_exp < SPELL_EXP_SKILLED) {
                if ((caster_ptr->current_floor_ptr->dun_level > 4) && ((caster_ptr->current_floor_ptr->dun_level + 10) > caster_ptr->lev))
                    exp_gain = 8;
            } else if (cur_exp < SPELL_EXP_EXPERT) {
                if (((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && ((caster_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel))
                    exp_gain = 2;
            } else if ((cur_exp < SPELL_EXP_MASTER) && !increment) {
                if (((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && (caster_ptr->current_floor_ptr->dun_level > s_ptr->slevel))
                    exp_gain = 1;
            }
            caster_ptr->spell_exp[(increment ? 32 : 0) + spell] += exp_gain;
        }
    }

    take_turn(caster_ptr, 100);

    /* Over-exert the player */
    if (over_exerted) {
        int oops = need_mana;

        /* No mana left */
        caster_ptr->csp = 0;
        caster_ptr->csp_frac = 0;

        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));

        /* Hack -- Bypass free action */
        (void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(5 * oops + 1));

        switch (realm) {
        case REALM_LIFE:
            chg_virtue(caster_ptr, V_VITALITY, -10);
            break;
        case REALM_DEATH:
            chg_virtue(caster_ptr, V_UNLIFE, -10);
            break;
        case REALM_DAEMON:
            chg_virtue(caster_ptr, V_JUSTICE, 10);
            break;
        case REALM_NATURE:
            chg_virtue(caster_ptr, V_NATURE, -10);
            break;
        case REALM_CRUSADE:
            chg_virtue(caster_ptr, V_JUSTICE, -10);
            break;
        case REALM_HEX:
            chg_virtue(caster_ptr, V_COMPASSION, 10);
            break;
        default:
            chg_virtue(caster_ptr, V_KNOWLEDGE, -10);
            break;
        }

        /* Damage CON (possibly permanently) */
        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);

            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));

            /* Reduce constitution */
            (void)dec_stat(caster_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    caster_ptr->window_flags |= (PW_PLAYER);
    caster_ptr->window_flags |= (PW_SPELL);
}
