#include "window/display-sub-window-spells.h"
#include "core/window-redrawer.h"
#include "game-option/option-flags.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-info.h"
#include "mind/mind-sniper.h"
#include "mind/mind-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-status-table.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの全既知呪文を表示する / Display all known spells in a window
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static void display_spell_list(PlayerType *player_ptr)
{
    TERM_LEN y, x;
    int m[9];
    const magic_type *s_ptr;
    GAME_TEXT name[MAX_NLEN];
    char out_val[160];

    clear_from(0);

    PlayerClass pc(player_ptr);
    if (pc.is_every_magic()) {
        return;
    }

    if (pc.equals(PlayerClassType::SNIPER)) {
        display_snipe_list(player_ptr);
        return;
    }

    if (pc.has_listed_magics()) {
        PERCENTAGE minfail = 0;
        PLAYER_LEVEL plev = player_ptr->lev;
        PERCENTAGE chance = 0;
        mind_type spell;
        char comment[80];
        char psi_desc[160];
        MindKindType use_mind;
        bool use_hp = false;

        y = 1;
        x = 1;

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

        switch (player_ptr->pclass) {
        case PlayerClassType::MINDCRAFTER:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        case PlayerClassType::FORCETRAINER:
            use_mind = MindKindType::KI;
            break;
        case PlayerClassType::BERSERKER:
            use_mind = MindKindType::BERSERKER;
            use_hp = true;
            break;
        case PlayerClassType::MIRROR_MASTER:
            use_mind = MindKindType::MIRROR_MASTER;
            break;
        case PlayerClassType::NINJA:
            use_mind = MindKindType::NINJUTSU;
            use_hp = true;
            break;
        case PlayerClassType::ELEMENTALIST:
            use_mind = MindKindType::ELEMENTAL;
            break;
        default:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        }

        for (int i = 0; i < MAX_MIND_POWERS; i++) {
            byte a = TERM_WHITE;
            spell = mind_powers[static_cast<int>(use_mind)].info[i];
            if (spell.min_lev > plev) {
                break;
            }

            chance = spell.fail;
            chance -= 3 * (player_ptr->lev - spell.min_lev);
            chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
            if (!use_hp) {
                if (spell.mana_cost > player_ptr->csp) {
                    chance += 5 * (spell.mana_cost - player_ptr->csp);
                    a = TERM_ORANGE;
                }
            } else {
                if (spell.mana_cost > player_ptr->chp) {
                    chance += 100;
                    a = TERM_RED;
                }
            }

            minfail = adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]];
            if (chance < minfail) {
                chance = minfail;
            }

            auto player_stun = player_ptr->effects()->stun();
            chance += player_stun->get_magic_chance_penalty();
            if (chance > 95) {
                chance = 95;
            }

            mindcraft_info(player_ptr, comment, use_mind, i);
            sprintf(psi_desc, "  %c) %-30s%2d %4d %3d%%%s", I2A(i), spell.name, spell.min_lev, spell.mana_cost, chance, comment);

            term_putstr(x, y + i + 1, -1, a, psi_desc);
        }

        return;
    }

    if (REALM_NONE == player_ptr->realm1) {
        return;
    }

    for (int j = 0; j < ((player_ptr->realm2 > REALM_NONE) ? 2 : 1); j++) {
        m[j] = 0;
        y = (j < 3) ? 0 : (m[j - 3] + 2);
        x = 27 * (j % 3);
        int n = 0;
        for (int i = 0; i < 32; i++) {
            byte a = TERM_WHITE;

            if (!is_magic((j < 1) ? player_ptr->realm1 : player_ptr->realm2)) {
                s_ptr = &technic_info[((j < 1) ? player_ptr->realm1 : player_ptr->realm2) - MIN_TECHNIC][i % 32];
            } else {
                s_ptr = &mp_ptr->info[((j < 1) ? player_ptr->realm1 : player_ptr->realm2) - 1][i % 32];
            }

            const auto spell_name = exe_spell(player_ptr, (j < 1) ? player_ptr->realm1 : player_ptr->realm2, i % 32, SpellProcessType::NAME);
            strcpy(name, spell_name->data());

            if (s_ptr->slevel >= 99) {
                strcpy(name, _("(判読不能)", "(illegible)"));
                a = TERM_L_DARK;
            } else if ((j < 1) ? ((player_ptr->spell_forgotten1 & (1UL << i))) : ((player_ptr->spell_forgotten2 & (1UL << (i % 32))))) {
                a = TERM_ORANGE;
            } else if (!((j < 1) ? (player_ptr->spell_learned1 & (1UL << i)) : (player_ptr->spell_learned2 & (1UL << (i % 32))))) {
                a = TERM_RED;
            } else if (!((j < 1) ? (player_ptr->spell_worked1 & (1UL << i)) : (player_ptr->spell_worked2 & (1UL << (i % 32))))) {
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display spells in sub-windows
 */
void fix_spell(PlayerType *player_ptr)
{
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        term_type *old = game_term;
        if (!angband_terms[i]) {
            continue;
        }

        if (!(window_flag[i] & (PW_SPELL))) {
            continue;
        }

        term_activate(angband_terms[i]);
        display_spell_list(player_ptr);
        term_fresh();
        player_ptr->window_flags &= ~(PW_SPELL);
        term_activate(old);
    }
}
