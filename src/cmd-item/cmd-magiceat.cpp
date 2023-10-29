/*!
 * @brief プレイヤーのアイテムに関するコマンドの実装2 / Spell/Prayer commands
 * @date 2014/01/27
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 * @details
 * <pre>
 * This file includes code for eating food, drinking potions,
 * reading scrolls, aiming wands, using staffs, zapping rods,
 * and activating artifacts.
 *
 * In all cases, if the player becomes "aware" of the item's use
 * by testing it, mark it as "aware" and reward some experience
 * based on the object's level, always rounding up.  If the player
 * remains "unaware", mark that object "kind" as "tried".
 *
 * This code now correctly handles the unstacking of wands, staffs,
 * and rods.  Note the overly paranoid warning about potential pack
 * overflow, which allows the player to use and drop a stacked item.
 *
 * In all "unstacking" scenarios, the "used" object is "carried" as if
 * the player had just picked it up.  In particular, this means that if
 * the use of an item induces pack overflow, that item will be dropped.
 *
 * For simplicity, these routines induce a full "pack reorganization"
 * which not only combines similar items, but also reorganizes various
 * items to obey the current "sorting" method.  This may require about
 * 400 item comparisons, but only occasionally.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 *
 * Note that food/potions/scrolls no longer use bit-flags for effects,
 * but instead use the "sval" (which is also used to sort the objects).
 * </pre>
 */

#include "cmd-item/cmd-magiceat.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-usestaff.h"
#include "cmd-item/cmd-zaprod.h"
#include "cmd-item/cmd-zapwand.h"
#include "core/asking-player.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/object-kind-hook.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/magic-eater-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "spell/spell-info.h"
#include "system/baseitem-info.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include <algorithm>
#include <optional>

static std::optional<BaseitemKey> check_magic_eater_spell_repeat(magic_eater_data_type *magic_eater_data)
{
    COMMAND_CODE sn;
    if (!repeat_pull(&sn)) {
        return std::nullopt;
    }

    auto tval = ItemKindType::NONE;
    if (EATER_STAFF_BASE <= sn && sn < EATER_STAFF_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::STAFF;
    } else if (EATER_WAND_BASE <= sn && sn < EATER_WAND_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::WAND;
    } else if (EATER_ROD_BASE <= sn && sn < EATER_ROD_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::ROD;
    }

    const auto &item_group = magic_eater_data->get_item_group(tval);
    auto sval = sn % EATER_ITEM_GROUP_SIZE;
    if (sval >= static_cast<int>(item_group.size())) {
        return std::nullopt;
    }

    auto &item = item_group[sval];
    /* Verify the spell */
    switch (tval) {
    case ItemKindType::ROD:
        if (item.charge <= baseitems_info[lookup_baseitem_id({ ItemKindType::ROD, sval })].pval * (item.count - 1) * EATER_ROD_CHARGE) {
            return BaseitemKey(tval, sval);
        }
        break;
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
        if (item.charge >= EATER_CHARGE) {
            return BaseitemKey(tval, sval);
        }
        break;
    default:
        break;
    }

    return std::nullopt;
}

/*!
 * @brief 魔道具術師の取り込んだ魔力一覧から選択/閲覧する /
 * @param only_browse 閲覧するだけならばTRUE
 * @return 選択したアイテムのベースアイテムキー、キャンセルならばnullopt
 */
static std::optional<BaseitemKey> select_magic_eater(PlayerType *player_ptr, bool only_browse)
{
    bool flag, request_list;
    auto tval = ItemKindType::NONE;
    int menu_line = (use_menu ? 1 : 0);

    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();

    if (auto result = check_magic_eater_spell_repeat(magic_eater_data.get());
        result) {
        return result;
    }

    if (use_menu) {
        screen_save();

        while (tval == ItemKindType::NONE) {
#ifdef JP
            prt(format(" %s 杖", (menu_line == 1) ? "》" : "  "), 2, 14);
            prt(format(" %s 魔法棒", (menu_line == 2) ? "》" : "  "), 3, 14);
            prt(format(" %s ロッド", (menu_line == 3) ? "》" : "  "), 4, 14);
#else
            prt(format(" %s staff", (menu_line == 1) ? "> " : "  "), 2, 14);
            prt(format(" %s wand", (menu_line == 2) ? "> " : "  "), 3, 14);
            prt(format(" %s rod", (menu_line == 3) ? "> " : "  "), 4, 14);
#endif

            if (only_browse) {
                prt(_("どの種類の魔法を見ますか？", "Which type of magic do you browse?"), 0, 0);
            } else {
                prt(_("どの種類の魔法を使いますか？", "Which type of magic do you use?"), 0, 0);
            }

            const auto choice = inkey();
            switch (choice) {
            case ESCAPE:
            case 'z':
            case 'Z':
                screen_load();
                return std::nullopt;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case '8':
            case 'k':
            case 'K':
                menu_line += 2;
                break;
            case '\r':
            case 'x':
            case 'X':
                if (menu_line == 1) {
                    tval = ItemKindType::STAFF;
                } else if (menu_line == 2) {
                    tval = ItemKindType::WAND;
                } else {
                    tval = ItemKindType::ROD;
                }
                break;
            }
            if (menu_line > 3) {
                menu_line -= 3;
            }
        }
        screen_load();
    } else {
        while (true) {
            const auto choice = input_command(_("[A] 杖, [B] 魔法棒, [C] ロッド:", "[A] staff, [B] wand, [C] rod:"), true);
            if (!choice) {
                return std::nullopt;
            }

            if (choice == 'A' || choice == 'a') {
                tval = ItemKindType::STAFF;
                break;
            }

            if (choice == 'B' || choice == 'b') {
                tval = ItemKindType::WAND;
                break;
            }

            if (choice == 'C' || choice == 'c') {
                tval = ItemKindType::ROD;
                break;
            }
        }
    }

    const auto &item_group = magic_eater_data->get_item_group(tval);

    if (auto it = std::find_if(item_group.begin(), item_group.end(),
            [](const auto &item) { return item.count > 0; });
        it == item_group.end()) {
        msg_print(_("その種類の魔法は覚えていない！", "You don't have that type of magic!"));
        return std::nullopt;
    } else {
        if (use_menu) {
            menu_line = 1 + std::distance(std::begin(item_group), it);
        }
    }

    /* Nothing chosen yet */
    flag = false;

    std::string prompt;
    if (only_browse) {
        prompt = _("('*'で一覧, ESCで中断) どの魔力を見ますか？", "(*=List, ESC=exit) Browse which power? ");
    } else {
        prompt = _("('*'で一覧, ESCで中断) どの魔力を使いますか？", "(*=List, ESC=exit) Use which power? ");
    }

    screen_save();
    request_list = always_show_list;

    const int item_group_size = item_group.size();
    auto sval = 0;
    while (!flag) {
        /* Show the list */
        if (request_list || use_menu) {
            byte y, x = 0;
            PERCENTAGE chance;
            short bi_id;
            POSITION x1, y1;
            DEPTH level;
            byte col;

            for (y = 1; y < 20; y++) {
                prt("", y, x);
            }

            y = 1;

            /* Print header(s) */
#ifdef JP
            prt(format("                           %s 失率                           %s 失率", (tval == ItemKindType::ROD ? "  状態  " : "使用回数"),
                    (tval == ItemKindType::ROD ? "  状態  " : "使用回数")),
                y++, x);
#else
            prt(format("                           %s Fail                           %s Fail", (tval == ItemKindType::ROD ? "  Stat  " : " Charges"),
                    (tval == ItemKindType::ROD ? "  Stat  " : " Charges")),
                y++, x);
#endif

            /* Print list */
            for (auto sval_ctr = 0; sval_ctr < item_group_size; sval_ctr++) {
                auto &item = item_group[sval_ctr];
                if (item.count == 0) {
                    continue;
                }

                bi_id = lookup_baseitem_id({ tval, sval_ctr });

                std::string dummy;
                if (use_menu) {
                    if (sval_ctr == (menu_line - 1)) {
                        dummy = _("》", "> ");
                    } else {
                        dummy = "  ";
                    }
                }
                /* letter/number for power selection */
                else {
                    char letter;
                    if (sval_ctr < 26) {
                        letter = I2A(sval_ctr);
                    } else {
                        letter = '0' + sval_ctr - 26;
                    }
                    dummy = format("%c)", letter);
                }
                x1 = ((sval_ctr < item_group_size / 2) ? x : x + 40);
                y1 = ((sval_ctr < item_group_size / 2) ? y + sval_ctr : y + sval_ctr - item_group_size / 2);
                const auto &baseitem = baseitems_info[bi_id];
                level = (tval == ItemKindType::ROD ? baseitem.level * 5 / 6 - 5 : baseitem.level);
                chance = level * 4 / 5 + 20;
                chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
                level /= 2;
                if (player_ptr->lev > level) {
                    chance -= 3 * (player_ptr->lev - level);
                }
                chance = mod_spell_chance_1(player_ptr, chance);
                chance = std::max<int>(chance, adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]]);
                auto player_stun = player_ptr->effects()->stun();
                chance += player_stun->get_magic_chance_penalty();
                if (chance > 95) {
                    chance = 95;
                }

                chance = mod_spell_chance_2(player_ptr, chance);

                col = TERM_WHITE;

                if (bi_id) {
                    if (tval == ItemKindType::ROD) {
                        dummy.append(
                            format(_(" %-22.22s 充填:%2d/%2d%3d%%", " %-22.22s   (%2d/%2d) %3d%%"), baseitem.name.data(),
                                item.charge ? (item.charge - 1) / (EATER_ROD_CHARGE * baseitem.pval) + 1 : 0,
                                item.count, chance)
                                .data());
                        if (item.charge > baseitem.pval * (item.count - 1) * EATER_ROD_CHARGE) {
                            col = TERM_RED;
                        }
                    } else {
                        dummy.append(
                            format(" %-22.22s    %2d/%2d %3d%%", baseitem.name.data(), (int16_t)(item.charge / EATER_CHARGE),
                                item.count, chance)
                                .data());
                        if (item.charge < EATER_CHARGE) {
                            col = TERM_RED;
                        }
                    }
                } else {
                    dummy.clear();
                }
                c_prt(col, dummy, y1, x1);
            }
        }

        const auto choice = input_command(prompt);
        if (!choice) {
            break;
        }

        auto should_redraw_cursor = true;
        if (use_menu && choice != ' ') {
            switch (*choice) {
            case '0': {
                screen_load();
                return std::nullopt;
            }

            case '8':
            case 'k':
            case 'K': {
                do {
                    menu_line += item_group_size - 1;
                    if (menu_line > item_group_size) {
                        menu_line -= item_group_size;
                    }
                } while (item_group[menu_line - 1].count == 0);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                do {
                    menu_line++;
                    if (menu_line > item_group_size) {
                        menu_line -= item_group_size;
                    }
                } while (item_group[menu_line - 1].count == 0);
                break;
            }

            case '4':
            case 'h':
            case 'H':
            case '6':
            case 'l':
            case 'L': {
                bool reverse = false;
                if ((choice == '4') || (choice == 'h') || (choice == 'H')) {
                    reverse = true;
                }
                if (menu_line > item_group_size / 2) {
                    menu_line -= item_group_size / 2;
                    reverse = true;
                } else {
                    menu_line += item_group_size / 2;
                }
                while (item_group[menu_line - 1].count == 0) {
                    if (reverse) {
                        menu_line--;
                        if (menu_line < 2) {
                            reverse = false;
                        }
                    } else {
                        menu_line++;
                        if (menu_line > item_group_size - 1) {
                            reverse = true;
                        }
                    }
                }
                break;
            }

            case 'x':
            case 'X':
            case '\r': {
                sval = menu_line - 1;
                should_redraw_cursor = false;
                break;
            }
            }
        }

        /* Request redraw */
        if (use_menu && should_redraw_cursor) {
            continue;
        }

        /* Request redraw */
        if (!use_menu && ((choice == ' ') || (choice == '*') || (choice == '?'))) {
            /* Hide the list */
            if (request_list) {
                /* Hide list */
                request_list = false;
                screen_load();
                screen_save();
            } else {
                request_list = true;
            }

            /* Redo asking */
            continue;
        }

        if (!use_menu) {
            if (isalpha(*choice)) {
                sval = A2I(*choice);
            } else {
                sval = *choice - '0' + 26;
            }
        }

        /* Totally Illegal */
        if ((sval < 0) || (sval > item_group_size) || item_group[sval].count == 0) {
            bell();
            continue;
        }

        if (!only_browse) {
            auto &item = item_group[sval];
            if (tval == ItemKindType::ROD) {
                if (item.charge > baseitems_info[lookup_baseitem_id({ tval, sval })].pval * (item.count - 1) * EATER_ROD_CHARGE) {
                    msg_print(_("その魔法はまだ充填している最中だ。", "The magic is still charging."));
                    msg_print(nullptr);
                    continue;
                }
            } else {
                if (item.charge < EATER_CHARGE) {
                    msg_print(_("その魔法は使用回数が切れている。", "The magic has no charges left."));
                    msg_print(nullptr);
                    continue;
                }
            }
        }

        /* Browse */
        else {
            /* Clear lines, position cursor  (really should use strlen here) */
            term_erase(7, 23);
            term_erase(7, 22);
            term_erase(7, 21);
            term_erase(7, 20);

            display_wrap_around(baseitems_info[lookup_baseitem_id({ tval, sval })].text, 62, 21, 10);
            continue;
        }

        /* Stop the loop */
        flag = true;
    }
    screen_load();

    if (!flag) {
        return std::nullopt;
    }

    COMMAND_CODE base = 0;
    switch (tval) {
    case ItemKindType::STAFF:
        base = EATER_STAFF_BASE;
        break;
    case ItemKindType::WAND:
        base = EATER_WAND_BASE;
        break;
    case ItemKindType::ROD:
        base = EATER_ROD_BASE;
        break;
    default:
        break;
    }

    repeat_push(base + sval);

    return BaseitemKey(tval, sval);
}

/*!
 * @brief 取り込んだ魔力を利用するコマンドのメインルーチン /
 * Use eaten rod, wand or staff
 * @param only_browse 閲覧するだけならばTRUE
 * @param powerful 強力発動中の処理ならばTRUE
 * @return 実際にコマンドを実行したならばTRUEを返す。
 */
bool do_cmd_magic_eater(PlayerType *player_ptr, bool only_browse, bool powerful)
{
    bool use_charge = true;

    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    auto result = select_magic_eater(player_ptr, only_browse);
    PlayerEnergy energy(player_ptr);
    if (!result) {
        energy.reset_player_turn();
        return false;
    }
    auto &bi_key = *result;

    const auto bi_id = lookup_baseitem_id(bi_key);
    const auto &baseitem = baseitems_info[bi_id];
    auto level = (bi_key.tval() == ItemKindType::ROD ? baseitem.level * 5 / 6 - 5 : baseitem.level);
    auto chance = level * 4 / 5 + 20;
    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
    level /= 2;
    if (player_ptr->lev > level) {
        chance -= 3 * (player_ptr->lev - level);
    }
    chance = mod_spell_chance_1(player_ptr, chance);
    chance = std::max<int>(chance, adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]]);
    auto player_stun = player_ptr->effects()->stun();
    chance += player_stun->get_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    chance = mod_spell_chance_2(player_ptr, chance);

    if (randint0(100) < chance) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("呪文をうまく唱えられなかった！", "You failed to get the magic off!"));
        sound(SOUND_FAIL);
        if (randint1(100) >= chance) {
            chg_virtue(player_ptr, Virtue::CHANCE, -1);
        }
        energy.set_player_turn_energy(100);

        return true;
    } else {
        DIRECTION dir = 0;

        switch (bi_key.tval()) {
        case ItemKindType::ROD: {
            const auto sval = bi_key.sval();
            if (!sval) {
                return false;
            }

            if (bi_key.is_aiming_rod() && !get_aim_dir(player_ptr, &dir)) {
                return false;
            }

            (void)rod_effect(player_ptr, *sval, dir, &use_charge, powerful);
            if (!use_charge) {
                return false;
            }

            break;
        }
        case ItemKindType::WAND: {
            const auto sval = bi_key.sval();
            if (!sval) {
                return false;
            }

            if (!get_aim_dir(player_ptr, &dir)) {
                return false;
            }

            (void)wand_effect(player_ptr, *sval, dir, powerful, true);
            break;
        }
        default:
            const auto sval = bi_key.sval();
            if (!sval) {
                return false;
            }

            (void)staff_effect(player_ptr, *sval, &use_charge, powerful, true, true);
            if (!use_charge) {
                return false;
            }

            break;
        }

        if (randint1(100) < chance) {
            chg_virtue(player_ptr, Virtue::CHANCE, 1);
        }
    }

    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();
    const auto sval = bi_key.sval();
    if (!sval) {
        return false;
    }

    const auto tval = bi_key.tval();
    auto &item = magic_eater_data->get_item_group(tval)[*sval];

    energy.set_player_turn_energy(100);
    if (tval == ItemKindType::ROD) {
        item.charge += baseitem.pval * EATER_ROD_CHARGE;
    } else {
        item.charge -= EATER_CHARGE;
    }

    return true;
}
