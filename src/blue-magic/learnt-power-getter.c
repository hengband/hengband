#include "blue-magic/learnt-power-getter.h"
#include "blue-magic/learnt-info.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspells3.h" // todo set_rf_masks() が依存している、後で消す.
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
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
bool get_learned_power(player_type *caster_ptr, SPELL_IDX *sn)
{
    int blue_magic_num = 0;
    int count = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 18;
    PLAYER_LEVEL plev = caster_ptr->lev;
    PERCENTAGE chance = 0;
    int ask = TRUE;
    int mode = 0;
    int blue_magics[MAX_MONSPELLS];
    char choice;
    char out_val[160];
    char comment[80];
    BIT_FLAGS f4 = 0L;
    BIT_FLAGS f5 = 0L;
    BIT_FLAGS f6 = 0L;
    monster_power spell;
    int menu_line = use_menu ? 1 : 0;
    bool flag = FALSE;
    bool redraw = FALSE;

    *sn = -1;
    COMMAND_CODE code;
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        return TRUE;
    }

    if (use_menu) {
        screen_save();
        while (!mode) {
            prt(format(_(" %s ボルト", " %s bolt"), (menu_line == 1) ? _("》", "> ") : "  "), 2, 14);
            prt(format(_(" %s ボール", " %s ball"), (menu_line == 2) ? _("》", "> ") : "  "), 3, 14);
            prt(format(_(" %s ブレス", " %s breath"), (menu_line == 3) ? _("》", "> ") : "  "), 4, 14);
            prt(format(_(" %s 召喚", " %s sommoning"), (menu_line == 4) ? _("》", "> ") : "  "), 5, 14);
            prt(format(_(" %s その他", " %s others"), (menu_line == 5) ? _("》", "> ") : "  "), 6, 14);
            prt(_("どの種類の魔法を使いますか？", "use which type of magic? "), 0, 0);

            choice = inkey();
            switch (choice) {
            case ESCAPE:
            case 'z':
            case 'Z':
                screen_load();
                return FALSE;
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
            case 'x':
            case 'X':
                mode = menu_line;
                break;
            }
            if (menu_line > 5)
                menu_line -= 5;
        }

        screen_load();
    } else {
        sprintf(comment, _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:", "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:"));
        while (TRUE) {
            char ch;
            if (!get_com(comment, &ch, TRUE))
                return FALSE;

            if (ch == 'A' || ch == 'a') {
                mode = 1;
                break;
            }

            if (ch == 'B' || ch == 'b') {
                mode = 2;
                break;
            }

            if (ch == 'C' || ch == 'c') {
                mode = 3;
                break;
            }

            if (ch == 'D' || ch == 'd') {
                mode = 4;
                break;
            }

            if (ch == 'E' || ch == 'e') {
                mode = 5;
                break;
            }
        }
    }

    set_rf_masks(&f4, &f5, &f6, mode);

    for (blue_magic_num = 0, count = 0; blue_magic_num < 32; blue_magic_num++)
        if ((0x00000001 << blue_magic_num) & f4)
            blue_magics[count++] = blue_magic_num;

    for (; blue_magic_num < 64; blue_magic_num++)
        if ((0x00000001 << (blue_magic_num - 32)) & f5)
            blue_magics[count++] = blue_magic_num;

    for (; blue_magic_num < 96; blue_magic_num++)
        if ((0x00000001 << (blue_magic_num - 64)) & f6)
            blue_magics[count++] = blue_magic_num;

    for (blue_magic_num = 0; blue_magic_num < count; blue_magic_num++) {
        if (caster_ptr->magic_num2[blue_magics[blue_magic_num]] == 0)
            continue;

        if (use_menu)
            menu_line = blue_magic_num + 1;

        break;
    }

    if (blue_magic_num == count) {
        msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
        return FALSE;
    }

    (void)strnfmt(
        out_val, 78, _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "), I2A(0), I2A(count - 1), _("魔法", "magic"));

    if (use_menu)
        screen_save();

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
                do {
                    menu_line += (count - 1);
                    if (menu_line > count)
                        menu_line -= count;
                } while (!caster_ptr->magic_num2[blue_magics[menu_line - 1]]);
                break;
            }

            case '2':
            case 'j':
            case 'J': {
                do {
                    menu_line++;
                    if (menu_line > count)
                        menu_line -= count;
                } while (!caster_ptr->magic_num2[blue_magics[menu_line - 1]]);
                break;
            }

            case '6':
            case 'l':
            case 'L': {
                menu_line = count;
                while (!caster_ptr->magic_num2[blue_magics[menu_line - 1]])
                    menu_line--;
                break;
            }

            case '4':
            case 'h':
            case 'H': {
                menu_line = 1;
                while (!caster_ptr->magic_num2[blue_magics[menu_line - 1]])
                    menu_line++;
                break;
            }

            case 'x':
            case 'X':
            case '\r': {
                blue_magic_num = menu_line - 1;
                ask = FALSE;
                break;
            }
            }
        }

        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
            if (!redraw || use_menu) {
                char psi_desc[80];
                redraw = TRUE;
                if (!use_menu)
                    screen_save();

                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("MP 失率 効果", "SP Fail Info"), y, x + 33);

                for (blue_magic_num = 0; blue_magic_num < count; blue_magic_num++) {
                    int need_mana;
                    prt("", y + blue_magic_num + 1, x);
                    if (!caster_ptr->magic_num2[blue_magics[blue_magic_num]])
                        continue;

                    spell = monster_powers[blue_magics[blue_magic_num]];
                    chance = spell.fail;
                    if (plev > spell.level)
                        chance -= 3 * (plev - spell.level);
                    else
                        chance += (spell.level - plev);

                    chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[A_INT]] - 1);
                    chance = mod_spell_chance_1(caster_ptr, chance);
                    need_mana = mod_need_mana(caster_ptr, monster_powers[blue_magics[blue_magic_num]].smana, 0, REALM_NONE);
                    if (need_mana > caster_ptr->csp) {
                        chance += 5 * (need_mana - caster_ptr->csp);
                    }

                    PERCENTAGE minfail = adj_mag_fail[caster_ptr->stat_ind[A_INT]];
                    if (chance < minfail)
                        chance = minfail;

                    if (caster_ptr->stun > 50)
                        chance += 25;
                    else if (caster_ptr->stun)
                        chance += 15;

                    if (chance > 95)
                        chance = 95;

                    chance = mod_spell_chance_2(caster_ptr, chance);
                    learnt_info(caster_ptr, comment, blue_magics[blue_magic_num]);
                    if (use_menu) {
                        if (blue_magic_num == (menu_line - 1))
                            strcpy(psi_desc, _("  》", "  > "));
                        else
                            strcpy(psi_desc, "    ");
                    } else
                        sprintf(psi_desc, "  %c)", I2A(blue_magic_num));

                    strcat(psi_desc, format(" %-26s %3d %3d%%%s", spell.name, need_mana, chance, comment));
                    prt(psi_desc, y + blue_magic_num + 1, x);
                }

                if (y < 22)
                    prt("", y + blue_magic_num + 1, x);
            } else {
                redraw = FALSE;
                screen_load();
            }

            continue;
        }

        if (!use_menu) {
            ask = isupper(choice);
            if (ask)
                choice = (char)tolower(choice);

            blue_magic_num = (islower(choice) ? A2I(choice) : -1);
        }

        if ((blue_magic_num < 0) || (blue_magic_num >= count) || !caster_ptr->magic_num2[blue_magics[blue_magic_num]]) {
            bell();
            continue;
        }

        spell = monster_powers[blue_magics[blue_magic_num]];
        if (ask) {
            char tmp_val[160];
            (void)strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers[blue_magics[blue_magic_num]].name);
            if (!get_check(tmp_val))
                continue;
        }

        flag = TRUE;
    }

    if (redraw)
        screen_load();

    caster_ptr->window |= PW_SPELL;
    handle_stuff(caster_ptr);

    if (!flag)
        return FALSE;

    *sn = blue_magics[blue_magic_num];
    return TRUE;
}
