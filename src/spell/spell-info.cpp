﻿#include "spell/spell-info.h"
#include "io/input-key-requester.h"
#include "monster-race/monster-race.h"
#include "player/player-class.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"
#include "spell/spells-execution.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 呪文の経験値を返す /
 * Returns experience of a spell
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 呪文ID
 * @param use_realm 魔法領域
 * @return 経験値
 */
EXP experience_of_spell(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX use_realm)
{
    if (caster_ptr->pclass == CLASS_SORCERER)
        return SPELL_EXP_MASTER;
    else if (caster_ptr->pclass == CLASS_RED_MAGE)
        return SPELL_EXP_SKILLED;
    else if (use_realm == caster_ptr->realm1)
        return caster_ptr->spell_exp[spell];
    else if (use_realm == caster_ptr->realm2)
        return caster_ptr->spell_exp[spell + 32];
    else
        return 0;
}

/*!
 * @brief 呪文の消費MPを返す /
 * Modify mana consumption rate using spell exp and dec_mana
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param need_mana 基本消費MP
 * @param spell 呪文ID
 * @param realm 魔法領域
 * @return 消費MP
 */
MANA_POINT mod_need_mana(player_type *caster_ptr, MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm)
{
#define MANA_CONST 2400
#define MANA_DIV 4
#define DEC_MANA_DIV 3
    if ((realm > REALM_NONE) && (realm <= MAX_REALM)) {
        need_mana = need_mana * (MANA_CONST + SPELL_EXP_EXPERT - experience_of_spell(caster_ptr, spell, realm)) + (MANA_CONST - 1);
        need_mana *= caster_ptr->dec_mana ? DEC_MANA_DIV : MANA_DIV;
        need_mana /= MANA_CONST * MANA_DIV;
        if (need_mana < 1)
            need_mana = 1;
    } else {
        if (caster_ptr->dec_mana)
            need_mana = (need_mana + 1) * DEC_MANA_DIV / MANA_DIV;
    }

#undef DEC_MANA_DIV
#undef MANA_DIV
#undef MANA_CONST

    return need_mana;
}

/*!
 * @brief 呪文の失敗率修正処理1(呪い、消費魔力減少、呪文簡易化) /
 * Modify spell fail rate
 * Using to_m_chance, dec_mana, easy_spell and heavy_spell
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_1(player_type *caster_ptr, PERCENTAGE chance)
{
    chance += caster_ptr->to_m_chance;

    if (caster_ptr->heavy_spell)
        chance += 20;

    if (caster_ptr->dec_mana && caster_ptr->easy_spell)
        chance -= 4;
    else if (caster_ptr->easy_spell)
        chance -= 3;
    else if (caster_ptr->dec_mana)
        chance -= 2;

    return chance;
}

/*!
 * @brief 呪文の失敗率修正処理2(消費魔力減少、呪い、負値修正) /
 * Modify spell fail rate
 * Using to_m_chance, dec_mana, easy_spell and heavy_spell
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * Modify spell fail rate (as "suffix" process)
 * Using dec_mana, easy_spell and heavy_spell
 * Note: variable "chance" cannot be negative.
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_2(player_type *caster_ptr, PERCENTAGE chance)
{
    if (caster_ptr->dec_mana)
        chance--;
    if (caster_ptr->heavy_spell)
        chance += 5;
    return MAX(chance, 0);
}

/*!
 * @brief 呪文の失敗率計算メインルーチン /
 * Returns spell chance of failure for spell -RAK-
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 呪文ID
 * @param use_realm 魔法領域ID
 * @return 失敗率(%)
 */
PERCENTAGE spell_chance(player_type *caster_ptr, SPELL_IDX spell, REALM_IDX use_realm)
{
    if (!mp_ptr->spell_book)
        return 100;
    if (use_realm == REALM_HISSATSU)
        return 0;

    const magic_type *s_ptr;
    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[use_realm - 1][spell];
    }

    PERCENTAGE chance = s_ptr->sfail;
    chance -= 3 * (caster_ptr->lev - s_ptr->slevel);
    chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
    if (caster_ptr->riding)
        chance += (MAX(r_info[caster_ptr->current_floor_ptr->m_list[caster_ptr->riding].r_idx].level - caster_ptr->skill_exp[GINOU_RIDING] / 100 - 10, 0));

    MANA_POINT need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, use_realm);
    if (need_mana > caster_ptr->csp) {
        chance += 5 * (need_mana - caster_ptr->csp);
    }

    if ((use_realm != caster_ptr->realm1) && ((caster_ptr->pclass == CLASS_MAGE) || (caster_ptr->pclass == CLASS_PRIEST)))
        chance += 5;

    PERCENTAGE minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];
    if (mp_ptr->spell_xtra & MAGIC_FAIL_5PERCENT) {
        if (minfail < 5)
            minfail = 5;
    }

    if (((caster_ptr->pclass == CLASS_PRIEST) || (caster_ptr->pclass == CLASS_SORCERER)) && caster_ptr->icky_wield[0])
        chance += 25;
    if (((caster_ptr->pclass == CLASS_PRIEST) || (caster_ptr->pclass == CLASS_SORCERER)) && caster_ptr->icky_wield[1])
        chance += 25;

    chance = mod_spell_chance_1(caster_ptr, chance);
    PERCENTAGE penalty = (mp_ptr->spell_stat == A_WIS) ? 10 : 4;
    switch (use_realm) {
    case REALM_NATURE:
        if ((caster_ptr->align > 50) || (caster_ptr->align < -50))
            chance += penalty;
        break;
    case REALM_LIFE:
    case REALM_CRUSADE:
        if (caster_ptr->align < -20)
            chance += penalty;
        break;
    case REALM_DEATH:
    case REALM_DAEMON:
    case REALM_HEX:
        if (caster_ptr->align > 20)
            chance += penalty;
        break;
    }

    if (chance < minfail)
        chance = minfail;

    if (caster_ptr->stun > 50)
        chance += 25;
    else if (caster_ptr->stun)
        chance += 15;

    if (chance > 95)
        chance = 95;

    if ((use_realm == caster_ptr->realm1) || (use_realm == caster_ptr->realm2) || (caster_ptr->pclass == CLASS_SORCERER)
        || (caster_ptr->pclass == CLASS_RED_MAGE)) {
        EXP exp = experience_of_spell(caster_ptr, spell, use_realm);
        if (exp >= SPELL_EXP_EXPERT)
            chance--;
        if (exp >= SPELL_EXP_MASTER)
            chance--;
    }

    return mod_spell_chance_2(caster_ptr, chance);
}

/*!
 * @brief 呪文情報の表示処理 /
 * Print a list of spells (for browsing or casting or viewing)
 * @param caster_ptr 術者の参照ポインタ
 * @param target_spell 呪文ID
 * @param spells 表示するスペルID配列の参照ポインタ
 * @param num 表示するスペルの数(spellsの要素数)
 * @param y 表示メッセージ左上Y座標
 * @param x 表示メッセージ左上X座標
 * @param use_realm 魔法領域ID
 * @return なし
 */
void print_spells(player_type *caster_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX use_realm)
{
    if (((use_realm <= REALM_NONE) || (use_realm > MAX_REALM)) && current_world_ptr->wizard)
        msg_print(_("警告！ print_spell が領域なしに呼ばれた", "Warning! print_spells called with null realm"));

    prt("", y, x);
    char buf[256];
    if (use_realm == REALM_HISSATSU)
        strcpy(buf, _("  Lv   MP", "  Lv   SP"));
    else
        strcpy(buf, _("熟練度 Lv   MP 失率 効果", "Profic Lv   SP Fail Effect"));

    put_str(_("名前", "Name"), y, x + 5);
    put_str(buf, y, x + 29);

    int increment = 64;
    if ((caster_ptr->pclass == CLASS_SORCERER) || (caster_ptr->pclass == CLASS_RED_MAGE))
        increment = 0;
    else if (use_realm == caster_ptr->realm1)
        increment = 0;
    else if (use_realm == caster_ptr->realm2)
        increment = 32;

    int i;
    int exp_level;
    const magic_type *s_ptr;
    char info[80];
    char out_val[160];
    char ryakuji[5];
    bool max = FALSE;
    for (i = 0; i < num; i++) {
        SPELL_IDX spell = spells[i];

        if (!is_magic(use_realm)) {
            s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
        } else {
            s_ptr = &mp_ptr->info[use_realm - 1][spell];
        }

        MANA_POINT need_mana;
        if (use_realm == REALM_HISSATSU)
            need_mana = s_ptr->smana;
        else {
            EXP exp = experience_of_spell(caster_ptr, spell, use_realm);
            need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, use_realm);
            if ((increment == 64) || (s_ptr->slevel >= 99))
                exp_level = EXP_LEVEL_UNSKILLED;
            else
                exp_level = spell_exp_level(exp);

            max = FALSE;
            if (!increment && (exp_level == EXP_LEVEL_MASTER))
                max = TRUE;
            else if ((increment == 32) && (exp_level >= EXP_LEVEL_EXPERT))
                max = TRUE;
            else if (s_ptr->slevel >= 99)
                max = TRUE;
            else if ((caster_ptr->pclass == CLASS_RED_MAGE) && (exp_level >= EXP_LEVEL_SKILLED))
                max = TRUE;

            strncpy(ryakuji, exp_level_str[exp_level], 4);
            ryakuji[3] = ']';
            ryakuji[4] = '\0';
        }

        if (use_menu && target_spell) {
            if (i == (target_spell - 1))
                strcpy(out_val, _("  》 ", "  >  "));
            else
                strcpy(out_val, "     ");
        } else
            sprintf(out_val, "  %c) ", I2A(i));

        if (s_ptr->slevel >= 99) {
            strcat(out_val, format("%-30s", _("(判読不能)", "(illegible)")));
            c_prt(TERM_L_DARK, out_val, y + i + 1, x);
            continue;
        }

        strcpy(info, exe_spell(caster_ptr, use_realm, spell, SPELL_INFO));
        concptr comment = info;
        byte line_attr = TERM_WHITE;
        if ((caster_ptr->pclass == CLASS_SORCERER) || (caster_ptr->pclass == CLASS_RED_MAGE)) {
            if (s_ptr->slevel > caster_ptr->max_plv) {
                comment = _("未知", "unknown");
                line_attr = TERM_L_BLUE;
            } else if (s_ptr->slevel > caster_ptr->lev) {
                comment = _("忘却", "forgotten");
                line_attr = TERM_YELLOW;
            }
        } else if ((use_realm != caster_ptr->realm1) && (use_realm != caster_ptr->realm2)) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if ((use_realm == caster_ptr->realm1) ? ((caster_ptr->spell_forgotten1 & (1UL << spell))) : ((caster_ptr->spell_forgotten2 & (1UL << spell)))) {
            comment = _("忘却", "forgotten");
            line_attr = TERM_YELLOW;
        } else if (!((use_realm == caster_ptr->realm1) ? (caster_ptr->spell_learned1 & (1UL << spell)) : (caster_ptr->spell_learned2 & (1UL << spell)))) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if (!((use_realm == caster_ptr->realm1) ? (caster_ptr->spell_worked1 & (1UL << spell)) : (caster_ptr->spell_worked2 & (1UL << spell)))) {
            comment = _("未経験", "untried");
            line_attr = TERM_L_GREEN;
        }

        if (use_realm == REALM_HISSATSU) {
            strcat(out_val, format("%-25s %2d %4d", exe_spell(caster_ptr, use_realm, spell, SPELL_NAME), s_ptr->slevel, need_mana));
        } else {
            strcat(out_val,
                format("%-25s%c%-4s %2d %4d %3d%% %s", exe_spell(caster_ptr, use_realm, spell, SPELL_NAME), (max ? '!' : ' '), ryakuji, s_ptr->slevel,
                    need_mana, spell_chance(caster_ptr, spell, use_realm), comment));
        }

        c_prt(line_attr, out_val, y + i + 1, x);
    }

    prt("", y + i + 1, x);
}
