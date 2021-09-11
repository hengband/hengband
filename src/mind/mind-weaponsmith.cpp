﻿#include "mind/mind-weaponsmith.h"
#include "action/action-limited.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-smith.h"
#include "object-enchant/smith-types.h"
#include "object-enchant/tr-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-status/player-energy.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#include <algorithm>
#include <sstream>

static concptr const kaji_tips[5] = {
#ifdef JP
    "現在持っているエッセンスの一覧を表示する。",
    "アイテムからエッセンスを取り出す。エッセンスを取られたアイテムは全く魔法がかかっていない初期状態に戻る。",
    "既にエッセンスが付加されたアイテムからエッセンスのみ消し去る。エッセンスは手に入らない。",
    "アイテムにエッセンスを付加する。既にエッセンスが付加されたアイテムやアーティファクトには付加できない。",
    "武器や防具を強化したり、攻撃で傷つかないようにしたりする。エッセンスが付加されたアイテムやアーティファクトに対しても使用できる。",
#else
    "Display essences you have.",
    "Extract essences from an item. The item become non magical.",
    "Remove added essences from equipment which was improved before. The removed essence will be ruined.",
    "Add essences to an item. The improved items or artifacts cannot be reimprove.",
    "Enchant an item or make an item element-proofed. Improved items and artifacts can be enchanted too.",
#endif
};

/*!
 * @brief 所持しているエッセンス一覧を表示する
 */
static void display_essence(player_type *player_ptr)
{
    int i, num = 0;

    screen_save();
    for (i = 1; i < 22; i++) {
        prt("", i, 0);
    }
    prt(_("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "), 1, 8);
    Smith smith(player_ptr);
    for (const auto &essence : Smith::get_essence_list()) {
        auto name = Smith::get_essence_name(essence);
        auto amount = smith.get_essence_num_of_posessions(essence);
        prt(format("%-11s %5d", name, amount), 2 + num % 21, 8 + num / 21 * 22);
        num++;
    }
    prt(_("現在所持しているエッセンス", "List of all essences you have."), 0, 0);
    (void)inkey();
    screen_load();
    return;
}

/*!
 * @brief エッセンスの抽出処理
 * @param player_ptr プレーヤーへの参照ポインタ
 */
static void drain_essence(player_type *player_ptr)
{
    auto q = _("どのアイテムから抽出しますか？", "Extract from which item? ");
    auto s = _("抽出できるアイテムがありません。", "You have nothing you can extract from.");

    OBJECT_IDX item;
    auto o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), FuncItemTester(&object_type::is_weapon_armour_ammo));
    if (!o_ptr)
        return;

    if (o_ptr->is_known() && !o_ptr->is_nameless()) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        if (!get_check(format(_("本当に%sから抽出してよろしいですか？", "Really extract from %s? "), o_name)))
            return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    auto drain_result = Smith(player_ptr).drain_essence(o_ptr);

    if (drain_result.empty()) {
        msg_print(_("エッセンスは抽出できませんでした。", "You were not able to extract any essence."));
    } else {
        msg_print(_("抽出したエッセンス:", "Extracted essences:"));

        for (const auto &[essence, amount] : drain_result) {
            auto essence_name = Smith::get_essence_name(essence);
            msg_print(nullptr);
            msg_format("%s...%d%s", essence_name, amount, _("。", ". "));
        }
    }

    /* Apply autodestroy/inscription to the drained item */
    autopick_alter_item(player_ptr, item, true);
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief 付加するエッセンスの大別を選択する
 * @return 選んだエッセンスの大別ID
 */
static COMMAND_CODE choose_essence(void)
{
    COMMAND_CODE mode = 0;
    char choice;
    COMMAND_CODE menu_line = (use_menu ? 1 : 0);

#ifdef JP
    concptr menu_name[] = { "武器属性", "耐性", "能力", "数値", "スレイ", "ESP", "その他" };
#else
    concptr menu_name[] = { "Brand weapon", "Resistance", "Ability", "Magic number", "Slay", "ESP", "Others" };
#endif
    const COMMAND_CODE mode_max = 7;

    if (repeat_pull(&mode) && 1 <= mode && mode <= mode_max)
        return mode;
    mode = 0;
    if (use_menu) {
        screen_save();

        while (!mode) {
            int i;
            for (i = 0; i < mode_max; i++)
#ifdef JP
                prt(format(" %s %s", (menu_line == 1 + i) ? "》" : "  ", menu_name[i]), 2 + i, 14);
            prt("どの種類のエッセンス付加を行いますか？", 0, 0);
#else
                prt(format(" %s %s", (menu_line == 1 + i) ? "> " : "  ", menu_name[i]), 2 + i, 14);
            prt("Choose from menu.", 0, 0);
#endif

            choice = inkey();
            switch (choice) {
            case ESCAPE:
            case 'z':
            case 'Z':
                screen_load();
                return 0;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case '8':
            case 'k':
            case 'K':
                menu_line += mode_max - 1;
                break;
            case '\r':
            case '\n':
            case 'x':
            case 'X':
                mode = menu_line;
                break;
            }
            if (menu_line > mode_max)
                menu_line -= mode_max;
        }
        screen_load();
    } else {
        screen_save();
        while (!mode) {
            int i;

            for (i = 0; i < mode_max; i++)
                prt(format("  %c) %s", 'a' + i, menu_name[i]), 2 + i, 14);

            if (!get_com(_("何を付加しますか:", "Command :"), &choice, true)) {
                screen_load();
                return 0;
            }

            if (isupper(choice))
                choice = (char)tolower(choice);

            if ('a' <= choice && choice <= 'a' + (char)mode_max - 1)
                mode = (int)choice - 'a' + 1;
        }
        screen_load();
    }

    repeat_push(mode);
    return mode;
}

/*!
 * @brief エッセンスを実際に付加する
 * @param mode エッセンスの大別ID
 */
static void add_essence(player_type *player_ptr, SmithCategory mode)
{
    OBJECT_IDX item;
    COMMAND_CODE i;
    bool flag, redraw;
    char choice;
    concptr q, s;
    object_type *o_ptr;
    int ask = true;
    char out_val[160];
    GAME_TEXT o_name[MAX_NLEN];
    int menu_line = (use_menu ? 1 : 0);

    Smith smith(player_ptr);

    auto smith_effect_list = Smith::get_effect_list(mode);
    const auto smith_effect_list_max = static_cast<int>(smith_effect_list.size());

    if (!repeat_pull(&i) || i < 0 || i >= smith_effect_list_max) {
        flag = false;
        redraw = false;

        (void)strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの能力を付加しますか？", "(*=List, ESC=exit) Add which ability? "));
        if (use_menu)
            screen_save();

        choice = (always_show_list || use_menu) ? ESCAPE : 1;
        while (!flag) {
            if (choice == ESCAPE)
                choice = ' ';
            else if (!get_com(out_val, &choice, false))
                break;

            if (use_menu && choice != ' ') {
                switch (choice) {
                case '0': {
                    screen_load();
                    return;
                }

                case '8':
                case 'k':
                case 'K': {
                    menu_line += (smith_effect_list_max - 1);
                    break;
                }

                case '2':
                case 'j':
                case 'J': {
                    menu_line++;
                    break;
                }

                case '4':
                case 'h':
                case 'H': {
                    menu_line = 1;
                    break;
                }
                case '6':
                case 'l':
                case 'L': {
                    menu_line = smith_effect_list_max;
                    break;
                }

                case 'x':
                case 'X':
                case '\r':
                case '\n': {
                    i = menu_line - 1;
                    ask = false;
                    break;
                }
                }
                if (menu_line > smith_effect_list_max)
                    menu_line -= smith_effect_list_max;
            }
            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
                /* Show the list */
                if (!redraw || use_menu) {
                    byte y, x = 10;
                    int ctr;
                    byte col;

                    redraw = true;
                    if (!use_menu)
                        screen_save();

                    for (y = 1; y < 24; y++)
                        prt("", y, x);

                        /* Print header(s) */
#ifdef JP
                    prt(format("   %-45s %6s/%s", "能力(必要エッセンス)", "所持数", "必要数"), 1, x);

#else
                    prt(format("   %-44s %7s/%s", "Ability (needed essence)", "Possess", "Needs"), 1, x);
#endif
                    /* Print list */
                    for (ctr = 0; ctr < smith_effect_list_max; ctr++) {
                        auto effect = smith_effect_list[ctr];
                        std::stringstream title;

                        if (use_menu) {
                            if (ctr == (menu_line - 1))
                                title << _("》 ", ">  ");
                            else
                                title << "   ";

                        }
                        /* letter/number for power selection */
                        else {
                            title << static_cast<char>(I2A(ctr)) << ") ";
                        }

                        title << Smith::get_effect_name(effect);

                        col = TERM_WHITE;

                        title << "(" << Smith::get_need_essences_desc(effect) << ")";

                        if (smith.get_addable_count(effect, 1) <= 0)
                            col = TERM_RED;

                        char str[160];
                        auto consumption = Smith::get_essence_consumption(effect);
                        auto need_essences = Smith::get_need_essences(effect);
                        if (need_essences.size() == 1) {
                            auto essence = need_essences.front();
                            auto amount = smith.get_essence_num_of_posessions(essence);
                            snprintf(str, sizeof(str), "%-49s %5d/%d", title.str().c_str(), amount, consumption);
                        } else {
                            snprintf(str, sizeof(str), "%-49s  (\?\?)/%d", title.str().c_str(), consumption);
                        }

                        c_prt(col, str, ctr + 2, x);
                    }
                }

                /* Hide the list */
                else {
                    /* Hide list */
                    redraw = false;
                    screen_load();
                }

                /* Redo asking */
                continue;
            }

            if (!use_menu) {
                /* Note verify */
                ask = (isupper(choice));

                /* Lowercase */
                if (ask)
                    choice = (char)tolower(choice);

                /* Extract request */
                i = (islower(choice) ? A2I(choice) : -1);
            }

            /* Totally Illegal */
            if ((i < 0) || (i >= smith_effect_list_max) || smith.get_addable_count(smith_effect_list[i], 1) <= 0) {
                bell();
                continue;
            }

            /* Verify it */
            if (ask) {
                char tmp_val[160];

                /* Prompt */
                (void)strnfmt(tmp_val, 78, _("%sを付加しますか？ ", "Add the ability of %s? "), Smith::get_effect_name(smith_effect_list[i]));

                /* Belay that order */
                if (!get_check(tmp_val))
                    continue;
            }

            /* Stop the loop */
            flag = true;
        }
        if (redraw)
            screen_load();

        if (!flag)
            return;

        repeat_push(i);
    }

    auto effect = smith_effect_list[i];

    auto item_tester = Smith::get_item_tester(effect);

    q = _("どのアイテムを改良しますか？", "Improve which item? ");
    s = _("改良できるアイテムがありません。", "You have nothing to improve.");

    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), *item_tester);
    if (!o_ptr)
        return;

    if ((mode != SmithCategory::ENCHANT) && (o_ptr->is_artifact() || o_ptr->is_smith())) {
        msg_print(_("そのアイテムはこれ以上改良できない。", "This item can not be improved any further."));
        return;
    }

    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    const auto use_essence = Smith::get_essence_consumption(effect, o_ptr);
    if (o_ptr->number > 1) {
        msg_format(_("%d個あるのでエッセンスは%d必要です。", "For %d items, it will take %d essences."), o_ptr->number, use_essence);
    }

    if (smith.get_addable_count(effect, o_ptr->number) == 0) {
        msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
        return;
    }

    const auto effect_flags = Smith::get_effect_tr_flags(effect);
    auto add_essence_count = 1;
    if (effect_flags.has_any_of(TR_PVAL_FLAG_MASK)) {
        if (o_ptr->pval < 0) {
            msg_print(_("このアイテムの能力修正を強化することはできない。", "You cannot increase magic number of this item."));
            return;
        } else if (effect_flags.has(TR_BLOWS) && o_ptr->pval > 1) {
            if (!get_check(_("修正値は1になります。よろしいですか？", "The magic number of this weapon will become 1. Are you sure? "))) {
                return;
            }
            o_ptr->pval = 1;
        } else if (o_ptr->pval == 0) {
            char tmp[80];
            char tmp_val[8];
            auto limit = std::min(5, smith.get_addable_count(effect, o_ptr->number));

            sprintf(tmp, _("いくつ付加しますか？ (1-%d): ", "Enchant how many? (1-%d): "), limit);
            strcpy(tmp_val, "1");

            if (!get_string(tmp, tmp_val, 1))
                return;
            o_ptr->pval = static_cast<PARAMETER_VALUE>(std::clamp(atoi(tmp_val), 1, limit));
        }

        add_essence_count = o_ptr->pval;
    } else if (effect == SmithEffect::SLAY_GLOVE) {
        char tmp_val[8] = "1";
        const auto max_val = player_ptr->lev / 7 + 3;
        if (!get_string(format(_("いくつ付加しますか？ (1-%d):", "Enchant how many? (1-%d):"), max_val), tmp_val, 2)) {
            return;
        }
        add_essence_count = std::clamp(atoi(tmp_val), 1, max_val);
    }

    msg_format(_("エッセンスを%d個使用します。", "It will take %d essences."), use_essence * add_essence_count);

    if (smith.get_addable_count(effect, o_ptr->number) < add_essence_count) {
        msg_print(_("エッセンスが足りない。", "You don't have enough essences."));
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (!smith.add_essence(effect, o_ptr, add_essence_count)) {
        msg_print(_("改良に失敗した。", "You failed to enchant."));
        return;
    }

    auto effect_name = Smith::get_effect_name(effect);

    _(msg_format("%sに%sの能力を付加しました。", o_name, effect_name), msg_format("You have added ability of %s to %s.", effect_name, o_name));
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief エッセンスを消去する
 */
static void erase_essence(player_type *player_ptr)
{
    OBJECT_IDX item;
    concptr q, s;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    q = _("どのアイテムのエッセンスを消去しますか？", "Remove from which item? ");
    s = _("エッセンスを付加したアイテムがありません。", "You have nothing with added essence to remove.");

    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&object_type::is_smith));
    if (!o_ptr)
        return;

    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (!get_check(format(_("よろしいですか？ [%s]", "Are you sure? [%s]"), o_name)))
        return;

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    Smith(player_ptr).erase_essence(o_ptr);

    msg_print(_("エッセンスを取り去った。", "You removed all essence you have added."));
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN);
}

/*!
 * @brief 鍛冶コマンドのメインルーチン
 * @param only_browse TRUEならばエッセンス一覧の表示のみを行う
 */
void do_cmd_kaji(player_type *player_ptr, bool only_browse)
{
    COMMAND_CODE mode = 0;
    char choice;

    COMMAND_CODE menu_line = (use_menu ? 1 : 0);

    if (!only_browse) {
        if (cmd_limit_confused(player_ptr))
            return;
        if (cmd_limit_blind(player_ptr))
            return;
        if (cmd_limit_image(player_ptr))
            return;
    }

    if (!(repeat_pull(&mode) && 1 <= mode && mode <= 5)) {
        if (only_browse)
            screen_save();
        do {
            if (!only_browse)
                screen_save();
            if (use_menu) {
                while (!mode) {
#ifdef JP
                    prt(format(" %s エッセンス一覧", (menu_line == 1) ? "》" : "  "), 2, 14);
                    prt(format(" %s エッセンス抽出", (menu_line == 2) ? "》" : "  "), 3, 14);
                    prt(format(" %s エッセンス消去", (menu_line == 3) ? "》" : "  "), 4, 14);
                    prt(format(" %s エッセンス付加", (menu_line == 4) ? "》" : "  "), 5, 14);
                    prt(format(" %s 武器/防具強化", (menu_line == 5) ? "》" : "  "), 6, 14);
                    prt(format("どの種類の技術を%sますか？", only_browse ? "調べ" : "使い"), 0, 0);
#else
                    prt(format(" %s List essences", (menu_line == 1) ? "> " : "  "), 2, 14);
                    prt(format(" %s Extract essence", (menu_line == 2) ? "> " : "  "), 3, 14);
                    prt(format(" %s Remove essence", (menu_line == 3) ? "> " : "  "), 4, 14);
                    prt(format(" %s Add essence", (menu_line == 4) ? "> " : "  "), 5, 14);
                    prt(format(" %s Enchant weapon/armor", (menu_line == 5) ? "> " : "  "), 6, 14);
                    prt(format("Choose command from menu."), 0, 0);
#endif
                    choice = inkey();
                    switch (choice) {
                    case ESCAPE:
                    case 'z':
                    case 'Z':
                        screen_load();
                        return;
                    case '2':
                    case 'j':
                    case 'J':
                        menu_line++;
                        break;
                    case '8':
                    case 'k':
                    case 'K':
                        menu_line += 4;
                        break;
                    case '\r':
                    case '\n':
                    case 'x':
                    case 'X':
                        mode = menu_line;
                        break;
                    }
                    if (menu_line > 5)
                        menu_line -= 5;
                }
            }

            else {
                while (!mode) {
                    prt(_("  a) エッセンス一覧", "  a) List essences"), 2, 14);
                    prt(_("  b) エッセンス抽出", "  b) Extract essence"), 3, 14);
                    prt(_("  c) エッセンス消去", "  c) Remove essence"), 4, 14);
                    prt(_("  d) エッセンス付加", "  d) Add essence"), 5, 14);
                    prt(_("  e) 武器/防具強化", "  e) Enchant weapon/armor"), 6, 14);
#ifdef JP
                    if (!get_com(format("どの能力を%sますか:", only_browse ? "調べ" : "使い"), &choice, true))
#else
                    if (!get_com("Command :", &choice, true))
#endif
                    {
                        screen_load();
                        return;
                    }
                    switch (choice) {
                    case 'A':
                    case 'a':
                        mode = 1;
                        break;
                    case 'B':
                    case 'b':
                        mode = 2;
                        break;
                    case 'C':
                    case 'c':
                        mode = 3;
                        break;
                    case 'D':
                    case 'd':
                        mode = 4;
                        break;
                    case 'E':
                    case 'e':
                        mode = 5;
                        break;
                    }
                }
            }

            if (only_browse) {
                char temp[62 * 5];
                int line, j;

                /* Clear lines, position cursor  (really should use strlen here) */
                term_erase(14, 21, 255);
                term_erase(14, 20, 255);
                term_erase(14, 19, 255);
                term_erase(14, 18, 255);
                term_erase(14, 17, 255);
                term_erase(14, 16, 255);

                shape_buffer(kaji_tips[mode - 1], 62, temp, sizeof(temp));
                for (j = 0, line = 17; temp[j]; j += (1 + strlen(&temp[j]))) {
                    prt(&temp[j], line, 15);
                    line++;
                }
                mode = 0;
            }
            if (!only_browse)
                screen_load();
        } while (only_browse);
        repeat_push(mode);
    }
    switch (mode) {
    case 1:
        display_essence(player_ptr);
        break;
    case 2:
        drain_essence(player_ptr);
        break;
    case 3:
        erase_essence(player_ptr);
        break;
    case 4:
        mode = choose_essence();
        if (mode == 0)
            break;
        add_essence(player_ptr, static_cast<SmithCategory>(mode));
        break;
    case 5:
        add_essence(player_ptr, SmithCategory::ENCHANT);
        break;
    }
}
