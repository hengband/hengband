#include "mind/mind-power-getter.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "core/stuff-handler.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-info.h"
#include "mind/mind-types.h"
#include "player/player-class.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "player/player-status-table.h"

/*!
 * @brief 使用可能な特殊技能を選択する /
 * Allow user to choose a mindcrafter power.
 * @param sn 選択した特殊技能ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @param only_browse 一覧を見るだけの場合TRUEを返す
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
bool get_mind_power(player_type *caster_ptr, SPELL_IDX *sn, bool only_browse)
{
    SPELL_IDX i;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 10;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = caster_ptr->lev;
    PERCENTAGE chance = 0;
    int ask = TRUE;
    char choice;
    char out_val[160];
    char comment[80];
    concptr p;
    COMMAND_CODE code;
    mind_type spell;
    const mind_power *mind_ptr;
    bool flag, redraw;
    int use_mind;
    int menu_line = (use_menu ? 1 : 0);

    switch (caster_ptr->pclass) {
    case CLASS_MINDCRAFTER: {
        use_mind = MIND_MINDCRAFTER;
        p = _("超能力", "mindcraft");
        break;
    }
    case CLASS_FORCETRAINER: {
        use_mind = MIND_KI;
        p = _("練気術", "Force");
        break;
    }
    case CLASS_BERSERKER: {
        use_mind = MIND_BERSERKER;
        p = _("技", "brutal power");
        break;
    }
    case CLASS_MIRROR_MASTER: {
        use_mind = MIND_MIRROR_MASTER;
        p = _("鏡魔法", "magic");
        break;
    }
    case CLASS_NINJA: {
        use_mind = MIND_NINJUTSU;
        p = _("忍術", "ninjutsu");
        break;
    }
    default: {
        use_mind = 0;
        p = _("超能力", "mindcraft");
        break;
    }
    }

    mind_ptr = &mind_powers[use_mind];
    *sn = -1;
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        if (*sn == INVEN_FORCE)
            repeat_pull(&code);

        *sn = (SPELL_IDX)code;
        if (mind_ptr->info[*sn].min_lev <= plev)
            return TRUE;
    }

    flag = FALSE;
    redraw = FALSE;

    for (i = 0; i < MAX_MIND_POWERS; i++)
        if (mind_ptr->info[i].min_lev <= plev)
            num++;

    if (only_browse)
        (void)strnfmt(out_val, 78, _("(%^s %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "), p, I2A(0),
            I2A(num - 1), p);
    else
        (void)strnfmt(
            out_val, 78, _("(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "), p, I2A(0), I2A(num - 1), p);

    if (use_menu && !only_browse)
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
                if (!only_browse)
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
        }

        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
            if (!redraw || use_menu) {
                char psi_desc[80];
                bool has_weapon[2];
                redraw = TRUE;
                if (!only_browse && !use_menu)
                    screen_save();

                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(format(_("Lv   %s   失率 効果", "Lv   %s   Fail Info"), ((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU)) ? "HP" : "MP"), y,
                    x + 35);
                has_weapon[0] = has_melee_weapon(caster_ptr, INVEN_RARM);
                has_weapon[1] = has_melee_weapon(caster_ptr, INVEN_LARM);
                for (i = 0; i < MAX_MIND_POWERS; i++) {
                    int mana_cost;
                    spell = mind_ptr->info[i];
                    if (spell.min_lev > plev)
                        break;

                    chance = spell.fail;
                    mana_cost = spell.mana_cost;
                    if (chance) {
                        chance -= 3 * (plev - spell.min_lev);
                        chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
                        if (use_mind == MIND_KI) {
                            if (heavy_armor(caster_ptr))
                                chance += 20;

                            if (caster_ptr->icky_wield[0])
                                chance += 20;
                            else if (has_weapon[0])

                                chance += 10;
                            if (caster_ptr->icky_wield[1])
                                chance += 20;
                            else if (has_weapon[1])
                                chance += 10;

                            if (i == 5) {
                                int j;
                                for (j = 0; j < get_current_ki(caster_ptr) / 50; j++)
                                    mana_cost += (j + 1) * 3 / 2;
                            }
                        }

                        if ((use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU) && (mana_cost > caster_ptr->csp))
                            chance += 5 * (mana_cost - caster_ptr->csp);

                        chance += caster_ptr->to_m_chance;
                        minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];
                        if (chance < minfail)
                            chance = minfail;

                        if (caster_ptr->stun > 50)
                            chance += 25;
                        else if (caster_ptr->stun)
                            chance += 15;

                        if (use_mind == MIND_KI) {
                            if (heavy_armor(caster_ptr))
                                chance += 5;
                            if (caster_ptr->icky_wield[0])
                                chance += 5;
                            if (caster_ptr->icky_wield[1])
                                chance += 5;
                        }

                        if (chance > 95)
                            chance = 95;
                    }

                    mindcraft_info(caster_ptr, comment, use_mind, i);
                    if (use_menu) {
                        if (i == (menu_line - 1))
                            strcpy(psi_desc, _("  》 ", "  >  "));
                        else
                            strcpy(psi_desc, "     ");
                    } else
                        sprintf(psi_desc, "  %c) ", I2A(i));

                    strcat(psi_desc,
                        format("%-30s%2d %4d%s %3d%%%s", spell.name, spell.min_lev, mana_cost,
                            (((use_mind == MIND_MINDCRAFTER) && (i == 13)) ? _("～", "~ ") : "  "), chance, comment));
                    prt(psi_desc, y + i + 1, x);
                }

                prt("", y + i + 1, x);
            } else if (!only_browse) {
                redraw = FALSE;
                screen_load();
            }

            continue;
        }

        if (!use_menu) {
            ask = isupper(choice);
            if (ask)
                choice = (char)tolower(choice);

            i = (islower(choice) ? A2I(choice) : -1);
        }

        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        spell = mind_ptr->info[i];
        if (ask) {
            char tmp_val[160];
            (void)strnfmt(tmp_val, 78, _("%sを使いますか？", "Use %s? "), spell.name);
            if (!get_check(tmp_val))
                continue;
        }

        flag = TRUE;
    }

    if (redraw && !only_browse)
        screen_load();

    caster_ptr->window |= PW_SPELL;
    handle_stuff(caster_ptr);
    if (!flag)
        return FALSE;

    *sn = i;
    repeat_push((COMMAND_CODE)i);
    return TRUE;
}
