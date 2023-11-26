/*!
 * @brief 剣術の実装 / Blade arts
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "action/action-limited.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

#define TECHNIC_HISSATSU (REALM_HISSATSU - MIN_TECHNIC)

/*!
 * @brief 使用可能な剣術を選択する /
 * Allow user to choose a blade arts.
 * @param sn 選択した特殊技能ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
static int get_hissatsu_power(PlayerType *player_ptr, SPELL_IDX *sn)
{
    int j = 0;
    int num = 0;
    POSITION y = 1;
    POSITION x = 15;
    PLAYER_LEVEL plev = player_ptr->lev;
    char choice;
    concptr p = _("必殺剣", "special attack");
    COMMAND_CODE code;
    magic_type spell;
    int menu_line = (use_menu ? 1 : 0);

    /* Assume cancelled */
    *sn = (-1);

    /* Get the spell, if available */
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        /* Verify the spell */
        if (technic_info[TECHNIC_HISSATSU][*sn].slevel <= plev) {
            /* Success */
            return true;
        }
    }

    auto flag = false;
    auto redraw = false;

    int i;
    int selections[32]{};
    for (i = 0; i < 32; i++) {
        if (technic_info[TECHNIC_HISSATSU][i].slevel <= PY_MAX_LEVEL) {
            selections[num] = i;
            num++;
        }
    }

    constexpr auto fmt = _("(%s^ %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? ");
    const auto prompt = format(fmt, p, I2A(0), "abcdefghijklmnopqrstuvwxyz012345"[num - 1], p);

    if (use_menu) {
        screen_save();
    }
    choice = always_show_list ? ESCAPE : 1;

    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else {
            const auto new_choice = input_command(prompt);
            if (!new_choice) {
                break;
            }

            choice = *new_choice;
        }

        auto should_redraw_cursor = true;
        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0': {
                screen_load();
                return false;
            }

            case '8':
            case 'k':
            case 'K': {
                do {
                    menu_line += 31;
                    if (menu_line > 32) {
                        menu_line -= 32;
                    }
                } while (!(player_ptr->spell_learned1 & (1UL << (menu_line - 1))));
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                do {
                    menu_line++;
                    if (menu_line > 32) {
                        menu_line -= 32;
                    }
                } while (!(player_ptr->spell_learned1 & (1UL << (menu_line - 1))));
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
                if (menu_line > 16) {
                    menu_line -= 16;
                    reverse = true;
                } else {
                    menu_line += 16;
                }
                while (!(player_ptr->spell_learned1 & (1UL << (menu_line - 1)))) {
                    if (reverse) {
                        menu_line--;
                        if (menu_line < 2) {
                            reverse = false;
                        }
                    } else {
                        menu_line++;
                        if (menu_line > 31) {
                            reverse = true;
                        }
                    }
                }
                break;
            }

            case 'x':
            case 'X':
            case '\r':
            case '\n': {
                i = menu_line - 1;
                should_redraw_cursor = false;
                break;
            }
            }
        }
        /* Request redraw */
        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && should_redraw_cursor)) {
            /* Show the list */
            if (!redraw || use_menu) {
                int line;
                redraw = true;
                if (!use_menu) {
                    screen_save();
                }

                /* Display a list of spells */
                prt("", y, x);
                put_str(_("名前              Lv  MP      名前              Lv  MP ", "name              Lv  SP      name              Lv  SP "), y, x + 5);
                prt("", y + 1, x);
                /* Dump the spells */
                for (i = 0, line = 0; i < 32; i++) {
                    spell = technic_info[TECHNIC_HISSATSU][i];

                    if (spell.slevel > PY_MAX_LEVEL) {
                        continue;
                    }
                    line++;
                    if (!(player_ptr->spell_learned1 >> i)) {
                        break;
                    }

                    /* Access the spell */
                    if (spell.slevel > plev) {
                        continue;
                    }
                    if (!(player_ptr->spell_learned1 & (1UL << i))) {
                        continue;
                    }
                    std::string psi_desc;
                    if (use_menu) {
                        if (i == (menu_line - 1)) {
                            psi_desc = _("  》", "  > ");
                        } else {
                            psi_desc = "    ";
                        }

                    } else {
                        char letter;
                        if (line <= 26) {
                            letter = I2A(line - 1);
                        } else {
                            letter = '0' + line - 27;
                        }
                        psi_desc = format("  %c)", letter);
                    }

                    /* Dump the spell --(-- */
                    const auto spell_name = exe_spell(player_ptr, REALM_HISSATSU, i, SpellProcessType::NAME);
                    psi_desc.append(format(" %-18s%2d %3d", spell_name->data(), spell.slevel, spell.smana));
                    prt(psi_desc, y + (line % 17) + (line >= 17), x + (line / 17) * 30);
                    prt("", y + (line % 17) + (line >= 17) + 1, x + (line / 17) * 30);
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
            if (isalpha(choice)) {
                i = A2I(choice);
            } else {
                i = choice - '0' + 26;
            }
        }

        /* Totally Illegal */
        if ((i < 0) || (i >= 32) || !(player_ptr->spell_learned1 & (1U << selections[i]))) {
            bell();
            continue;
        }

        j = selections[i];

        /* Stop the loop */
        flag = true;
    }
    if (redraw) {
        screen_load();
    }

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);

    /* Abort if needed */
    if (!flag) {
        return false;
    }

    /* Save the choice */
    (*sn) = j;

    repeat_push((COMMAND_CODE)j);

    /* Success */
    return true;
}

/*!
 * @brief 剣術コマンドのメインルーチン
 */
void do_cmd_hissatsu(PlayerType *player_ptr)
{
    SPELL_IDX n = 0;
    magic_type spell;

    if (cmd_limit_confused(player_ptr)) {
        return;
    }
    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        if (flush_failure) {
            flush();
        }
        msg_print(_("武器を持たないと必殺技は使えない！", "You need to wield a weapon!"));
        return;
    }
    if (!player_ptr->spell_learned1) {
        msg_print(_("何も技を知らない。", "You don't know any special attacks."));
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::IAI, SamuraiStanceType::FUUJIN, SamuraiStanceType::KOUKIJIN });

    if (!get_hissatsu_power(player_ptr, &n)) {
        return;
    }

    spell = technic_info[TECHNIC_HISSATSU][n];

    /* Verify "dangerous" spells */
    if (spell.smana > player_ptr->csp) {
        if (flush_failure) {
            flush();
        }
        /* Warning */
        msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
        msg_print(nullptr);
        return;
    }

    sound(SOUND_ZAP);

    if (!exe_spell(player_ptr, REALM_HISSATSU, n, SpellProcessType::CAST)) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    player_ptr->csp -= spell.smana;
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
}

/*!
 * @brief 剣術コマンドの学習
 */
void do_cmd_gain_hissatsu(PlayerType *player_ptr)
{
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });
    if (cmd_limit_blind(player_ptr) || cmd_limit_confused(player_ptr)) {
        return;
    }

    if (!(player_ptr->new_spells)) {
        msg_print(_("新しい必殺技を覚えることはできない！", "You cannot learn any new special attacks!"));
        return;
    }

#ifdef JP
    msg_format("あと %d 種の必殺技を学べる。", player_ptr->new_spells);
#else
    msg_format("You can learn %d new special attack%s.", player_ptr->new_spells, (player_ptr->new_spells == 1 ? "" : "s"));
#endif

    constexpr auto q = _("どの書から学びますか? ", "Study which book? ");
    constexpr auto s = _("読める書がない。", "You have no books that you can read.");
    constexpr auto options = USE_INVEN | USE_FLOOR;
    short i_idx;
    const auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options, TvalItemTester(ItemKindType::HISSATSU_BOOK));
    if (o_ptr == nullptr) {
        return;
    }

    const auto sval = *o_ptr->bi_key.sval();
    auto gain = false;
    for (auto i = sval * 8; i < sval * 8 + 8; i++) {
        if (player_ptr->spell_learned1 & (1UL << i)) {
            continue;
        }

        if (technic_info[TECHNIC_HISSATSU][i].slevel > player_ptr->lev) {
            continue;
        }

        player_ptr->spell_learned1 |= (1UL << i);
        player_ptr->spell_worked1 |= (1UL << i);
        const auto spell_name = exe_spell(player_ptr, REALM_HISSATSU, i, SpellProcessType::NAME);
        msg_format(_("%sの技を覚えた。", "You have learned the special attack of %s."), spell_name->data());
        int j;
        for (j = 0; j < 64; j++) {
            /* Stop at the first empty space */
            if (player_ptr->spell_order[j] == 99) {
                break;
            }
        }

        player_ptr->spell_order[j] = i;
        gain = true;
    }

    if (!gain) {
        msg_print(_("何も覚えられなかった。", "You were not able to learn any special attacks."));
    } else {
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::SPELLS);
}
