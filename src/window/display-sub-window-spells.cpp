﻿#include "window/display-sub-window-spells.h"
#include "core/window-redrawer.h"
#include "game-option/option-flags.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-info.h"
#include "mind/mind-sniper.h"
#include "mind/mind-types.h"
#include "player/player-class.h"
#include "player/player-status-table.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief クリーチャー全既知呪文を表示する /
 * Hack -- Display all known spells in a window
 * @param caster_ptr 術者の参照ポインタ
 * return なし
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static void display_spell_list(player_type *caster_ptr)
{
    TERM_LEN y, x;
    int m[9];
    const magic_type *s_ptr;
    GAME_TEXT name[MAX_NLEN];
    char out_val[160];

    clear_from(0);

    if (caster_ptr->pclass == CLASS_SORCERER)
        return;
    if (caster_ptr->pclass == CLASS_RED_MAGE)
        return;
    if (caster_ptr->pclass == CLASS_SNIPER) {
        display_snipe_list(caster_ptr);
        return;
    }

    if ((caster_ptr->pclass == CLASS_MINDCRAFTER) || (caster_ptr->pclass == CLASS_BERSERKER) || (caster_ptr->pclass == CLASS_NINJA)
        || (caster_ptr->pclass == CLASS_MIRROR_MASTER) || (caster_ptr->pclass == CLASS_FORCETRAINER)) {
        PERCENTAGE minfail = 0;
        PLAYER_LEVEL plev = caster_ptr->lev;
        PERCENTAGE chance = 0;
        mind_type spell;
        char comment[80];
        char psi_desc[160];
        int use_mind;
        bool use_hp = FALSE;

        y = 1;
        x = 1;

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

        switch (caster_ptr->pclass) {
        case CLASS_MINDCRAFTER:
            use_mind = MIND_MINDCRAFTER;
            break;
        case CLASS_FORCETRAINER:
            use_mind = MIND_KI;
            break;
        case CLASS_BERSERKER:
            use_mind = MIND_BERSERKER;
            use_hp = TRUE;
            break;
        case CLASS_MIRROR_MASTER:
            use_mind = MIND_MIRROR_MASTER;
            break;
        case CLASS_NINJA:
            use_mind = MIND_NINJUTSU;
            use_hp = TRUE;
            break;
        default:
            use_mind = 0;
            break;
        }

        for (int i = 0; i < MAX_MIND_POWERS; i++) {
            byte a = TERM_WHITE;
            spell = mind_powers[use_mind].info[i];
            if (spell.min_lev > plev)
                break;

            chance = spell.fail;
            chance -= 3 * (caster_ptr->lev - spell.min_lev);
            chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
            if (!use_hp) {
                if (spell.mana_cost > caster_ptr->csp) {
                    chance += 5 * (spell.mana_cost - caster_ptr->csp);
                    a = TERM_ORANGE;
                }
            } else {
                if (spell.mana_cost > caster_ptr->chp) {
                    chance += 100;
                    a = TERM_RED;
                }
            }

            minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];
            if (chance < minfail)
                chance = minfail;

            if (caster_ptr->stun > 50)
                chance += 25;
            else if (caster_ptr->stun)
                chance += 15;

            if (chance > 95)
                chance = 95;

            mindcraft_info(caster_ptr, comment, use_mind, i);
            sprintf(psi_desc, "  %c) %-30s%2d %4d %3d%%%s", I2A(i), spell.name, spell.min_lev, spell.mana_cost, chance, comment);

            term_putstr(x, y + i + 1, -1, a, psi_desc);
        }

        return;
    }

    if (REALM_NONE == caster_ptr->realm1)
        return;

    for (int j = 0; j < ((caster_ptr->realm2 > REALM_NONE) ? 2 : 1); j++) {
        m[j] = 0;
        y = (j < 3) ? 0 : (m[j - 3] + 2);
        x = 27 * (j % 3);
        int n = 0;
        for (int i = 0; i < 32; i++) {
            byte a = TERM_WHITE;

            if (!is_magic((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2)) {
                s_ptr = &technic_info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - MIN_TECHNIC][i % 32];
            } else {
                s_ptr = &mp_ptr->info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - 1][i % 32];
            }

            strcpy(name, exe_spell(caster_ptr, (j < 1) ? caster_ptr->realm1 : caster_ptr->realm2, i % 32, SPELL_NAME));

            if (s_ptr->slevel >= 99) {
                strcpy(name, _("(判読不能)", "(illegible)"));
                a = TERM_L_DARK;
            } else if ((j < 1) ? ((caster_ptr->spell_forgotten1 & (1UL << i))) : ((caster_ptr->spell_forgotten2 & (1UL << (i % 32))))) {
                a = TERM_ORANGE;
            } else if (!((j < 1) ? (caster_ptr->spell_learned1 & (1UL << i)) : (caster_ptr->spell_learned2 & (1UL << (i % 32))))) {
                a = TERM_RED;
            } else if (!((j < 1) ? (caster_ptr->spell_worked1 & (1UL << i)) : (caster_ptr->spell_worked2 & (1UL << (i % 32))))) {
                a = TERM_YELLOW;
            }

            sprintf(out_val, "%c/%c) %-20.20s", I2A(n / 8), I2A(n % 8), name);

            m[j] = y + n;
            term_putstr(x, m[j], -1, a, out_val);
            n++;
        }
    }
}

/*!
 * @brief 現在の習得済魔法をサブウィンドウに表示する /
 * @param player_ptr プレーヤーへの参照ポインタ
 * Hack -- display spells in sub-windows
 * @return なし
 */
void fix_spell(player_type *player_ptr)
{
    for (int j = 0; j < 8; j++) {
        term_type *old = Term;
        if (!angband_term[j])
            continue;

        if (!(window_flag[j] & (PW_SPELL)))
            continue;

        term_activate(angband_term[j]);
        display_spell_list(player_ptr);
        term_fresh();
        player_ptr->window_flags &= ~(PW_SPELL);
        term_activate(old);
    }
}
