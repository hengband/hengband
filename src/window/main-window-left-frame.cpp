﻿#include "window/main-window-left-frame.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "monster/monster-status.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-status-table.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "window/main-window-row-column.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの称号を表示する / Prints "title", including "wizard" or "winner" as needed.
 */
void print_title(PlayerType *player_ptr)
{
    GAME_TEXT str[14];
    concptr p = "";
    if (w_ptr->wizard) {
        p = _("[ウィザード]", "[=-WIZARD-=]");
    } else if (w_ptr->total_winner) {
        if (player_ptr->is_true_winner()) {
            p = _("*真・勝利者*", "*TRUEWINNER*");
        } else {
            p = _("***勝利者***", "***WINNER***");
        }
    } else {
        angband_strcpy(str, player_titles[enum2i(player_ptr->pclass)][(player_ptr->lev - 1) / 5].data(), sizeof(str));
        p = str;
    }

    print_field(p, ROW_TITLE, COL_TITLE);
}

/*!
 * @brief プレイヤーのレベルを表示する / Prints level
 */
void print_level(PlayerType *player_ptr)
{
    std::string tmp = format("%5d", player_ptr->lev);
    if (player_ptr->lev >= player_ptr->max_plv) {
        put_str(_("レベル ", "LEVEL "), ROW_LEVEL, 0);
        c_put_str(TERM_L_GREEN, tmp.data(), ROW_LEVEL, COL_LEVEL + 7);
    } else {
        put_str(_("xレベル", "Level "), ROW_LEVEL, 0);
        c_put_str(TERM_YELLOW, tmp.data(), ROW_LEVEL, COL_LEVEL + 7);
    }
}

/*!
 * @brief プレイヤーの経験値を表示する / Display the experience
 */
void print_exp(PlayerType *player_ptr)
{
    std::string out_val;

    PlayerRace pr(player_ptr);
    if ((!exp_need) || pr.equals(PlayerRaceType::ANDROID)) {
        out_val = format("%8ld", (long)player_ptr->exp);
    } else {
        if (player_ptr->lev >= PY_MAX_LEVEL) {
            out_val = "********";
        } else {
            out_val = format("%8ld", (long)(player_exp[player_ptr->lev - 1] * player_ptr->expfact / 100L) - player_ptr->exp);
        }
    }

    if (player_ptr->exp >= player_ptr->max_exp) {
        if (pr.equals(PlayerRaceType::ANDROID)) {
            put_str(_("強化 ", "Cst "), ROW_EXP, 0);
        } else {
            put_str(_("経験 ", "EXP "), ROW_EXP, 0);
        }
        c_put_str(TERM_L_GREEN, out_val.data(), ROW_EXP, COL_EXP + 4);
    } else {
        put_str(_("x経験", "Exp "), ROW_EXP, 0);
        c_put_str(TERM_YELLOW, out_val.data(), ROW_EXP, COL_EXP + 4);
    }
}

/*!
 * @brief プレイヤーのACを表示する / Prints current AC
 */
void print_ac(PlayerType *player_ptr)
{
    std::string tmp = format("%5d", player_ptr->dis_ac + player_ptr->dis_to_a);

#ifdef JP
    /* AC の表示方式を変更している */
    put_str(" ＡＣ(     )", ROW_AC, COL_AC);
    c_put_str(TERM_L_GREEN, tmp.data(), ROW_AC, COL_AC + 6);
#else
    put_str("Cur AC ", ROW_AC, COL_AC);
    c_put_str(TERM_L_GREEN, tmp.data(), ROW_AC, COL_AC + 7);
#endif
}

/*!
 * @brief プレイヤーのHPを表示する / Prints Cur/Max hit points
 */
void print_hp(PlayerType *player_ptr)
{
    put_str("HP", ROW_CURHP, COL_CURHP);
    TERM_COLOR color;
    if (player_ptr->chp >= player_ptr->mhp) {
        color = TERM_L_GREEN;
    } else if (player_ptr->chp > (player_ptr->mhp * hitpoint_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, format("%4ld", (long int)player_ptr->chp), ROW_CURHP, COL_CURHP + 3);
    put_str("/", ROW_CURHP, COL_CURHP + 7);
    color = TERM_L_GREEN;
    c_put_str(color, format("%4ld", (long int)player_ptr->mhp), ROW_CURHP, COL_CURHP + 8);
}

/*!
 * @brief プレイヤーのMPを表示する / Prints players max/cur spell points
 */
void print_sp(PlayerType *player_ptr)
{
    if ((mp_ptr->spell_book == ItemKindType::NONE) && mp_ptr->spell_first == SPELL_FIRST_NO_SPELL) {
        return;
    }

    put_str(_("MP", "SP"), ROW_CURSP, COL_CURSP);
    byte color;
    if (player_ptr->csp >= player_ptr->msp) {
        color = TERM_L_GREEN;
    } else if (player_ptr->csp > (player_ptr->msp * mana_warn) / 10) {
        color = TERM_YELLOW;
    } else {
        color = TERM_RED;
    }

    c_put_str(color, format("%4ld", (long int)player_ptr->csp), ROW_CURSP, COL_CURSP + 3);
    put_str("/", ROW_CURSP, COL_CURSP + 7);
    color = TERM_L_GREEN;
    c_put_str(color, format("%4ld", (long int)player_ptr->msp), ROW_CURSP, COL_CURSP + 8);
}

/*!
 * @brief プレイヤーの所持金を表示する / Prints current gold
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_gold(PlayerType *player_ptr)
{
    put_str(_("＄ ", "AU "), ROW_GOLD, COL_GOLD);
    c_put_str(TERM_L_GREEN, format("%9ld", (long)player_ptr->au), ROW_GOLD, COL_GOLD + 3);
}

/*!
 * @brief 現在のフロアの深さを表示する / Prints depth in stat area
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_depth(PlayerType *player_ptr)
{
    TERM_COLOR attr = TERM_WHITE;

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    TERM_LEN col_depth = wid + COL_DEPTH;
    TERM_LEN row_depth = hgt + ROW_DEPTH;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level) {
        c_prt(attr, format("%7s", _("地上", "Surf.")), row_depth, col_depth);
        return;
    }

    if (inside_quest(floor_ptr->quest_number) && !player_ptr->dungeon_idx) {
        c_prt(attr, format("%7s", _("地上", "Quest")), row_depth, col_depth);
        return;
    }

    std::string depths;
    if (depth_in_feet) {
        depths = format(_("%d ft", "%d ft"), (int)floor_ptr->dun_level * 50);
    } else {
        depths = format(_("%d 階", "Lev %d"), (int)floor_ptr->dun_level);
    }

    switch (player_ptr->feeling) {
    case 0:
        attr = TERM_SLATE;
        break; /* Unknown */
    case 1:
        attr = TERM_L_BLUE;
        break; /* Special */
    case 2:
        attr = TERM_VIOLET;
        break; /* Horrible visions */
    case 3:
        attr = TERM_RED;
        break; /* Very dangerous */
    case 4:
        attr = TERM_L_RED;
        break; /* Very bad feeling */
    case 5:
        attr = TERM_ORANGE;
        break; /* Bad feeling */
    case 6:
        attr = TERM_YELLOW;
        break; /* Nervous */
    case 7:
        attr = TERM_L_UMBER;
        break; /* Luck is turning */
    case 8:
        attr = TERM_L_WHITE;
        break; /* Don't like */
    case 9:
        attr = TERM_WHITE;
        break; /* Reasonably safe */
    case 10:
        attr = TERM_WHITE;
        break; /* Boring place */
    }

    c_prt(attr, format("%7s", depths.data()), row_depth, col_depth);
}

/*!
 * @brief プレイヤーのステータスを一括表示する（左側部分） / Display basic info (mostly left of map)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void print_frame_basic(PlayerType *player_ptr)
{
    if (player_ptr->mimic_form != MimicKindType::NONE) {
        print_field(mimic_info.at(player_ptr->mimic_form).title, ROW_RACE, COL_RACE);
    } else {
        char str[14];
        angband_strcpy(str, rp_ptr->title, sizeof(str));
        print_field(str, ROW_RACE, COL_RACE);
    }

    print_title(player_ptr);
    print_level(player_ptr);
    print_exp(player_ptr);
    for (int i = 0; i < A_MAX; i++) {
        print_stat(player_ptr, i);
    }

    print_ac(player_ptr);
    print_hp(player_ptr);
    print_sp(player_ptr);
    print_gold(player_ptr);
    print_depth(player_ptr);
    health_redraw(player_ptr, false);
    health_redraw(player_ptr, true);
}

/*!
 * @brief モンスターの体力ゲージを表示する
 * @param riding TRUEならば騎乗中のモンスターの体力、FALSEならターゲットモンスターの体力を表示する。表示位置は固定。
 * @details
 * <pre>
 * Redraw the "monster health bar"	-DRS-
 * Rather extensive modifications by	-BEN-
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.
 *
 * Display the monster health bar (affectionately known as the
 * "health-o-meter").  Clear health bar if nothing is being tracked.
 * Auto-track current target monster when bored.  Note that the
 * health-bar stops tracking any monster that "disappears".
 * </pre>
 */
void health_redraw(PlayerType *player_ptr, bool riding)
{
    int16_t health_who;
    int row, col;

    if (riding) {
        health_who = player_ptr->riding;
        row = ROW_RIDING_INFO;
        col = COL_RIDING_INFO;
    } else {
        health_who = player_ptr->health_who;
        row = ROW_INFO;
        col = COL_INFO;
    }

    MonsterEntity *m_ptr;
    m_ptr = &player_ptr->current_floor_ptr->m_list[health_who];

    if (w_ptr->wizard && player_ptr->phase_out) {
        row = ROW_INFO - 1;
        col = COL_INFO + 2;

        term_putstr(col - 2, row, 12, TERM_WHITE, "      /     ");
        term_putstr(col - 2, row + 1, 12, TERM_WHITE, "      /     ");
        term_putstr(col - 2, row + 2, 12, TERM_WHITE, "      /     ");
        term_putstr(col - 2, row + 3, 12, TERM_WHITE, "      /     ");

        if (MonsterRace(player_ptr->current_floor_ptr->m_list[1].r_idx).is_valid()) {
            term_putstr(col - 2, row, 2, monraces_info[player_ptr->current_floor_ptr->m_list[1].r_idx].x_attr,
                format("%c", monraces_info[player_ptr->current_floor_ptr->m_list[1].r_idx].x_char));
            term_putstr(col - 1, row, 5, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[1].hp));
            term_putstr(col + 5, row, 6, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[1].max_maxhp));
        }

        if (MonsterRace(player_ptr->current_floor_ptr->m_list[2].r_idx).is_valid()) {
            term_putstr(col - 2, row + 1, 2, monraces_info[player_ptr->current_floor_ptr->m_list[2].r_idx].x_attr,
                format("%c", monraces_info[player_ptr->current_floor_ptr->m_list[2].r_idx].x_char));
            term_putstr(col - 1, row + 1, 5, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[2].hp));
            term_putstr(col + 5, row + 1, 6, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[2].max_maxhp));
        }

        if (MonsterRace(player_ptr->current_floor_ptr->m_list[3].r_idx).is_valid()) {
            term_putstr(col - 2, row + 2, 2, monraces_info[player_ptr->current_floor_ptr->m_list[3].r_idx].x_attr,
                format("%c", monraces_info[player_ptr->current_floor_ptr->m_list[3].r_idx].x_char));
            term_putstr(col - 1, row + 2, 5, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[3].hp));
            term_putstr(col + 5, row + 2, 6, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[3].max_maxhp));
        }

        if (MonsterRace(player_ptr->current_floor_ptr->m_list[4].r_idx).is_valid()) {
            term_putstr(col - 2, row + 3, 2, monraces_info[player_ptr->current_floor_ptr->m_list[4].r_idx].x_attr,
                format("%c", monraces_info[player_ptr->current_floor_ptr->m_list[4].r_idx].x_char));
            term_putstr(col - 1, row + 3, 5, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[4].hp));
            term_putstr(col + 5, row + 3, 6, TERM_WHITE, format("%5d", player_ptr->current_floor_ptr->m_list[4].max_maxhp));
        }

        return;
    }

    if (!health_who) {
        term_erase(col, row, 12);
        return;
    }

    if (!m_ptr->ml) {
        term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    if (player_ptr->effects()->hallucination()->is_hallucinated()) {
        term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    if (m_ptr->hp < 0) {
        term_putstr(col, row, 12, TERM_WHITE, "[----------]");
        return;
    }

    int pct = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;
    int pct2 = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->max_maxhp : 0;
    int len = (pct2 < 10) ? 1 : (pct2 < 90) ? (pct2 / 10 + 1)
                                            : 10;
    TERM_COLOR attr = TERM_RED;
    if (m_ptr->is_invulnerable()) {
        attr = TERM_WHITE;
    } else if (m_ptr->is_asleep()) {
        attr = TERM_BLUE;
    } else if (m_ptr->is_fearful()) {
        attr = TERM_VIOLET;
    } else if (pct >= 100) {
        attr = TERM_L_GREEN;
    } else if (pct >= 60) {
        attr = TERM_YELLOW;
    } else if (pct >= 25) {
        attr = TERM_ORANGE;
    } else if (pct >= 10) {
        attr = TERM_L_RED;
    }

    term_putstr(col, row, 12, TERM_WHITE, "[----------]");
    term_putstr(col + 1, row, len, attr, "**********");
}
