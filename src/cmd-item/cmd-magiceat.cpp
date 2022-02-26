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
#include "object/object-kind.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/magic-eater-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "spell/spell-info.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-rod-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#include <algorithm>
#include <optional>
#include <tuple>

static std::optional<std::tuple<ItemKindType, OBJECT_SUBTYPE_VALUE>> check_magic_eater_spell_repeat(magic_eater_data_type *magic_eater_data)
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
        if (item.charge <= k_info[lookup_kind(ItemKindType::ROD, sval)].pval * (item.count - 1) * EATER_ROD_CHARGE) {
            return std::make_tuple(tval, sval);
        }
        break;
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
        if (item.charge >= EATER_CHARGE) {
            return std::make_tuple(tval, sval);
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
 * @return 選択した魔力のID、キャンセルならば-1を返す
 */
static std::optional<std::tuple<ItemKindType, OBJECT_SUBTYPE_VALUE>> select_magic_eater(PlayerType *player_ptr, bool only_browse)
{
    char choice;
    bool flag, request_list;
    auto tval = ItemKindType::NONE;
    int ask = true;
    OBJECT_SUBTYPE_VALUE i = 0;
    char out_val[160];

    int menu_line = (use_menu ? 1 : 0);

    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();

    if (auto result = check_magic_eater_spell_repeat(magic_eater_data.get());
        result.has_value()) {
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

            choice = inkey();
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
            if (!get_com(_("[A] 杖, [B] 魔法棒, [C] ロッド:", "[A] staff, [B] wand, [C] rod:"), &choice, true)) {
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

    if (only_browse) {
        strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの魔力を見ますか？", "(*=List, ESC=exit) Browse which power? "));
    } else {
        strnfmt(out_val, 78, _("('*'で一覧, ESCで中断) どの魔力を使いますか？", "(*=List, ESC=exit) Use which power? "));
    }
    screen_save();

    request_list = always_show_list;

    const int ITEM_GROUP_SIZE = item_group.size();
    while (!flag) {
        /* Show the list */
        if (request_list || use_menu) {
            byte y, x = 0;
            OBJECT_SUBTYPE_VALUE ctr;
            PERCENTAGE chance;
            KIND_OBJECT_IDX k_idx;
            char dummy[80];
            POSITION x1, y1;
            DEPTH level;
            byte col;

            strcpy(dummy, "");

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
            for (ctr = 0; ctr < ITEM_GROUP_SIZE; ctr++) {
                auto &item = item_group[ctr];
                if (item.count == 0) {
                    continue;
                }

                k_idx = lookup_kind(tval, ctr);

                if (use_menu) {
                    if (ctr == (menu_line - 1)) {
                        strcpy(dummy, _("》", "> "));
                    } else {
                        strcpy(dummy, "  ");
                    }
                }
                /* letter/number for power selection */
                else {
                    char letter;
                    if (ctr < 26) {
                        letter = I2A(ctr);
                    } else {
                        letter = '0' + ctr - 26;
                    }
                    sprintf(dummy, "%c)", letter);
                }
                x1 = ((ctr < ITEM_GROUP_SIZE / 2) ? x : x + 40);
                y1 = ((ctr < ITEM_GROUP_SIZE / 2) ? y + ctr : y + ctr - ITEM_GROUP_SIZE / 2);
                level = (tval == ItemKindType::ROD ? k_info[k_idx].level * 5 / 6 - 5 : k_info[k_idx].level);
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

                if (k_idx) {
                    if (tval == ItemKindType::ROD) {
                        strcat(dummy,
                            format(_(" %-22.22s 充填:%2d/%2d%3d%%", " %-22.22s   (%2d/%2d) %3d%%"), k_info[k_idx].name.c_str(),
                                item.charge ? (item.charge - 1) / (EATER_ROD_CHARGE * k_info[k_idx].pval) + 1 : 0,
                                item.count, chance));
                        if (item.charge > k_info[k_idx].pval * (item.count - 1) * EATER_ROD_CHARGE) {
                            col = TERM_RED;
                        }
                    } else {
                        strcat(dummy,
                            format(" %-22.22s    %2d/%2d %3d%%", k_info[k_idx].name.c_str(), (int16_t)(item.charge / EATER_CHARGE),
                                item.count, chance));
                        if (item.charge < EATER_CHARGE) {
                            col = TERM_RED;
                        }
                    }
                } else {
                    strcpy(dummy, "");
                }
                c_prt(col, dummy, y1, x1);
            }
        }

        if (!get_com(out_val, &choice, false)) {
            break;
        }

        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0': {
                screen_load();
                return std::nullopt;
            }

            case '8':
            case 'k':
            case 'K': {
                do {
                    menu_line += ITEM_GROUP_SIZE - 1;
                    if (menu_line > ITEM_GROUP_SIZE) {
                        menu_line -= ITEM_GROUP_SIZE;
                    }
                } while (item_group[menu_line - 1].count == 0);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                do {
                    menu_line++;
                    if (menu_line > ITEM_GROUP_SIZE) {
                        menu_line -= ITEM_GROUP_SIZE;
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
                if (menu_line > ITEM_GROUP_SIZE / 2) {
                    menu_line -= ITEM_GROUP_SIZE / 2;
                    reverse = true;
                } else {
                    menu_line += ITEM_GROUP_SIZE / 2;
                }
                while (item_group[menu_line - 1].count == 0) {
                    if (reverse) {
                        menu_line--;
                        if (menu_line < 2) {
                            reverse = false;
                        }
                    } else {
                        menu_line++;
                        if (menu_line > ITEM_GROUP_SIZE - 1) {
                            reverse = true;
                        }
                    }
                }
                break;
            }

            case 'x':
            case 'X':
            case '\r': {
                i = menu_line - 1;
                ask = false;
                break;
            }
            }
        }

        /* Request redraw */
        if (use_menu && ask) {
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
            if (isalpha(choice)) {
                /* Note verify */
                ask = (isupper(choice));

                /* Lowercase */
                if (ask) {
                    choice = (char)tolower(choice);
                }

                /* Extract request */
                i = (islower(choice) ? A2I(choice) : -1);
            } else {
                ask = false; /* Can't uppercase digits */

                i = choice - '0' + 26;
            }
        }

        /* Totally Illegal */
        if ((i < 0) || (i > ITEM_GROUP_SIZE) || item_group[i].count == 0) {
            bell();
            continue;
        }

        if (!only_browse) {
            /* Verify it */
            if (ask) {
                char tmp_val[160];

                /* Prompt */
                (void)strnfmt(tmp_val, 78, _("%sを使いますか？ ", "Use %s? "), k_info[lookup_kind(tval, i)].name.c_str());

                /* Belay that order */
                if (!get_check(tmp_val)) {
                    continue;
                }
            }
            auto &item = item_group[i];
            if (tval == ItemKindType::ROD) {
                if (item.charge > k_info[lookup_kind(tval, i)].pval * (item.count - 1) * EATER_ROD_CHARGE) {
                    msg_print(_("その魔法はまだ充填している最中だ。", "The magic is still charging."));
                    msg_print(nullptr);
                    if (use_menu) {
                        ask = true;
                    }
                    continue;
                }
            } else {
                if (item.charge < EATER_CHARGE) {
                    msg_print(_("その魔法は使用回数が切れている。", "The magic has no charges left."));
                    msg_print(nullptr);
                    if (use_menu) {
                        ask = true;
                    }
                    continue;
                }
            }
        }

        /* Browse */
        else {
            int line, j;
            char temp[70 * 20];

            /* Clear lines, position cursor  (really should use strlen here) */
            term_erase(7, 23, 255);
            term_erase(7, 22, 255);
            term_erase(7, 21, 255);
            term_erase(7, 20, 255);

            shape_buffer(k_info[lookup_kind(tval, i)].text.c_str(), 62, temp, sizeof(temp));
            for (j = 0, line = 21; temp[j]; j += 1 + strlen(&temp[j])) {
                prt(&temp[j], line, 10);
                line++;
            }

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
    default:
        break;
    }

    repeat_push(base + i);

    return std::make_tuple(tval, i);
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
    if (!result.has_value()) {
        energy.reset_player_turn();
        return false;
    }
    auto [tval, sval] = result.value();

    auto k_idx = lookup_kind(tval, sval);
    auto level = (tval == ItemKindType::ROD ? k_info[k_idx].level * 5 / 6 - 5 : k_info[k_idx].level);
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
            chg_virtue(player_ptr, V_CHANCE, -1);
        }
        energy.set_player_turn_energy(100);

        return true;
    } else {
        DIRECTION dir = 0;

        if (tval == ItemKindType::ROD) {
            if ((sval >= SV_ROD_MIN_DIRECTION) && (sval != SV_ROD_HAVOC) && (sval != SV_ROD_AGGRAVATE) && (sval != SV_ROD_PESTICIDE)) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return false;
                }
            }
            (void)rod_effect(player_ptr, sval, dir, &use_charge, powerful);
            if (!use_charge) {
                return false;
            }
        } else if (tval == ItemKindType::WAND) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return false;
            }
            wand_effect(player_ptr, sval, dir, powerful, true);
        } else {
            staff_effect(player_ptr, sval, &use_charge, powerful, true, true);
            if (!use_charge) {
                return false;
            }
        }
        if (randint1(100) < chance) {
            chg_virtue(player_ptr, V_CHANCE, 1);
        }
    }

    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();
    auto &item = magic_eater_data->get_item_group(tval)[sval];

    energy.set_player_turn_energy(100);
    if (tval == ItemKindType::ROD) {
        item.charge += k_info[k_idx].pval * EATER_ROD_CHARGE;
    } else {
        item.charge -= EATER_CHARGE;
    }

    return true;
}
