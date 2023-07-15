#include "spell/spell-info.h"
#include "io/input-key-requester.h"
#include "monster-race/monster-race.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"
#include "spell/spells-execution.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

// 5%
static const int extra_min_magic_fail_rate = 2;

/*!
 * @brief 呪文の消費MPを返す /
 * Modify mana consumption rate using spell exp and dec_mana
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param need_mana 基本消費MP
 * @param spell 呪文ID
 * @param realm 魔法領域
 * @return 消費MP
 */
MANA_POINT mod_need_mana(PlayerType *player_ptr, MANA_POINT need_mana, SPELL_IDX spell, int16_t realm)
{
#define MANA_CONST 2400
#define MANA_DIV 4
#define DEC_MANA_DIV 3
    if ((realm > REALM_NONE) && (realm <= MAX_REALM)) {
        need_mana = need_mana * (MANA_CONST + PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT) - PlayerSkill(player_ptr).exp_of_spell(realm, spell)) + (MANA_CONST - 1);
        need_mana *= player_ptr->dec_mana ? DEC_MANA_DIV : MANA_DIV;
        need_mana /= MANA_CONST * MANA_DIV;
        if (need_mana < 1) {
            need_mana = 1;
        }
    } else {
        if (player_ptr->dec_mana) {
            need_mana = (need_mana + 1) * DEC_MANA_DIV / MANA_DIV;
        }
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_1(PlayerType *player_ptr, PERCENTAGE chance)
{
    chance += player_ptr->to_m_chance;

    if (player_ptr->heavy_spell) {
        chance += 20;
    }

    if (player_ptr->dec_mana && player_ptr->easy_spell) {
        chance -= 4;
    } else if (player_ptr->easy_spell) {
        chance -= 3;
    } else if (player_ptr->dec_mana) {
        chance -= 2;
    }

    return chance;
}

/*!
 * @brief 呪文の失敗率修正処理2(消費魔力減少、呪い、負値修正) /
 * Modify spell fail rate
 * Using to_m_chance, dec_mana, easy_spell and heavy_spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * Modify spell fail rate (as "suffix" process)
 * Using dec_mana, easy_spell and heavy_spell
 * Note: variable "chance" cannot be negative.
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_2(PlayerType *player_ptr, PERCENTAGE chance)
{
    if (player_ptr->dec_mana) {
        chance--;
    }
    if (player_ptr->heavy_spell) {
        chance += 5;
    }
    return std::max(chance, 0);
}

/*!
 * @brief 呪文の失敗率計算メインルーチン /
 * Returns spell chance of failure for spell -RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 呪文ID
 * @param use_realm 魔法領域ID
 * @return 失敗率(%)
 */
PERCENTAGE spell_chance(PlayerType *player_ptr, SPELL_IDX spell, int16_t use_realm)
{
    if (mp_ptr->spell_book == ItemKindType::NONE) {
        return 100;
    }
    if (use_realm == REALM_HISSATSU) {
        return 0;
    }

    const magic_type *s_ptr;
    if (!is_magic(use_realm)) {
        s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
    } else {
        s_ptr = &mp_ptr->info[use_realm - 1][spell];
    }

    PERCENTAGE chance = s_ptr->sfail;
    chance -= 3 * (player_ptr->lev - s_ptr->slevel);
    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
    if (player_ptr->riding) {
        chance += (std::max(monraces_info[player_ptr->current_floor_ptr->m_list[player_ptr->riding].r_idx].level - player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 100 - 10, 0));
    }

    MANA_POINT need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, use_realm);
    if (need_mana > player_ptr->csp) {
        chance += 5 * (need_mana - player_ptr->csp);
    }

    PlayerClass pc(player_ptr);
    if ((use_realm != player_ptr->realm1) && (pc.equals(PlayerClassType::MAGE) || pc.equals(PlayerClassType::PRIEST))) {
        chance += 5;
    }

    PERCENTAGE minfail = adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]];
    if (any_bits(mp_ptr->spell_xtra, extra_min_magic_fail_rate)) {
        if (minfail < 5) {
            minfail = 5;
        }
    }

    if ((pc.equals(PlayerClassType::PRIEST) || pc.equals(PlayerClassType::SORCERER))) {
        if (player_ptr->is_icky_wield[0]) {
            chance += 25;
        }

        if (player_ptr->is_icky_wield[1]) {
            chance += 25;
        }
    }

    chance = mod_spell_chance_1(player_ptr, chance);
    PERCENTAGE penalty = (mp_ptr->spell_stat == A_WIS) ? 10 : 4;
    switch (use_realm) {
    case REALM_NATURE:
        if ((player_ptr->alignment > 50) || (player_ptr->alignment < -50)) {
            chance += penalty;
        }
        break;
    case REALM_LIFE:
    case REALM_CRUSADE:
        if (player_ptr->alignment < -20) {
            chance += penalty;
        }
        break;
    case REALM_DEATH:
    case REALM_DAEMON:
    case REALM_HEX:
        if (player_ptr->alignment > 20) {
            chance += penalty;
        }
        break;
    }

    if (chance < minfail) {
        chance = minfail;
    }

    auto player_stun = player_ptr->effects()->stun();
    chance += player_stun->get_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    if ((use_realm == player_ptr->realm1) || (use_realm == player_ptr->realm2) || pc.is_every_magic()) {
        auto exp = PlayerSkill(player_ptr).exp_of_spell(use_realm, spell);
        if (exp >= PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT)) {
            chance--;
        }
        if (exp >= PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER)) {
            chance--;
        }
    }

    return mod_spell_chance_2(player_ptr, chance);
}

/*!
 * @brief 呪文情報の表示処理 /
 * Print a list of spells (for browsing or casting or viewing)
 * @param player_ptr 術者の参照ポインタ
 * @param target_spell 呪文ID
 * @param spells 表示するスペルID配列の参照ポインタ
 * @param num 表示するスペルの数(spellsの要素数)
 * @param y 表示メッセージ左上Y座標
 * @param x 表示メッセージ左上X座標
 * @param use_realm 魔法領域ID
 */
void print_spells(PlayerType *player_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, int16_t use_realm)
{
    if (((use_realm <= REALM_NONE) || (use_realm > MAX_REALM)) && w_ptr->wizard) {
        msg_print(_("警告！ print_spell が領域なしに呼ばれた", "Warning! print_spells called with null realm"));
    }

    prt("", y, x);
    char buf[256];
    if (use_realm == REALM_HISSATSU) {
        strcpy(buf, _("  Lv   MP", "  Lv   SP"));
    } else {
        strcpy(buf, _("熟練度 Lv   MP 失率 効果", "Profic Lv   SP Fail Effect"));
    }

    put_str(_("名前", "Name"), y, x + 5);
    put_str(buf, y, x + 29);

    int increment = 64;
    PlayerClass pc(player_ptr);
    if (pc.is_every_magic()) {
        increment = 0;
    } else if (use_realm == player_ptr->realm1) {
        increment = 0;
    } else if (use_realm == player_ptr->realm2) {
        increment = 32;
    }

    int i;
    const magic_type *s_ptr;
    char ryakuji[5];
    bool max = false;
    for (i = 0; i < num; i++) {
        SPELL_IDX spell = spells[i];

        if (!is_magic(use_realm)) {
            s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
        } else {
            s_ptr = &mp_ptr->info[use_realm - 1][spell];
        }

        MANA_POINT need_mana;
        if (use_realm == REALM_HISSATSU) {
            need_mana = s_ptr->smana;
        } else {
            auto exp = PlayerSkill(player_ptr).exp_of_spell(use_realm, spell);
            need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, use_realm);
            PlayerSkillRank skill_rank;
            if ((increment == 64) || (s_ptr->slevel >= 99)) {
                skill_rank = PlayerSkillRank::UNSKILLED;
            } else {
                skill_rank = PlayerSkill::spell_skill_rank(exp);
            }

            max = false;
            if (!increment && (skill_rank == PlayerSkillRank::MASTER)) {
                max = true;
            } else if ((increment == 32) && (skill_rank >= PlayerSkillRank::EXPERT)) {
                max = true;
            } else if (s_ptr->slevel >= 99) {
                max = true;
            } else if (pc.equals(PlayerClassType::RED_MAGE) && (skill_rank >= PlayerSkillRank::SKILLED)) {
                max = true;
            }

            strncpy(ryakuji, PlayerSkill::skill_rank_str(skill_rank), 4);
            ryakuji[3] = ']';
            ryakuji[4] = '\0';
        }

        std::string out_val;
        if (use_menu && target_spell) {
            if (i == (target_spell - 1)) {
                out_val = _("  》 ", "  >  ");
            } else {
                out_val = "     ";
            }
        } else {
            out_val = format("  %c) ", I2A(i));
        }

        if (s_ptr->slevel >= 99) {
            out_val.append(format("%-30s", _("(判読不能)", "(illegible)")));
            c_prt(TERM_L_DARK, out_val, y + i + 1, x);
            continue;
        }

        const auto info = exe_spell(player_ptr, use_realm, spell, SpellProcessType::INFO);
        concptr comment = info->data();
        byte line_attr = TERM_WHITE;
        if (pc.is_every_magic()) {
            if (s_ptr->slevel > player_ptr->max_plv) {
                comment = _("未知", "unknown");
                line_attr = TERM_L_BLUE;
            } else if (s_ptr->slevel > player_ptr->lev) {
                comment = _("忘却", "forgotten");
                line_attr = TERM_YELLOW;
            }
        } else if ((use_realm != player_ptr->realm1) && (use_realm != player_ptr->realm2)) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if ((use_realm == player_ptr->realm1) ? ((player_ptr->spell_forgotten1 & (1UL << spell))) : ((player_ptr->spell_forgotten2 & (1UL << spell)))) {
            comment = _("忘却", "forgotten");
            line_attr = TERM_YELLOW;
        } else if (!((use_realm == player_ptr->realm1) ? (player_ptr->spell_learned1 & (1UL << spell)) : (player_ptr->spell_learned2 & (1UL << spell)))) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if (!((use_realm == player_ptr->realm1) ? (player_ptr->spell_worked1 & (1UL << spell)) : (player_ptr->spell_worked2 & (1UL << spell)))) {
            comment = _("未経験", "untried");
            line_attr = TERM_L_GREEN;
        }

        const auto spell_name = exe_spell(player_ptr, use_realm, spell, SpellProcessType::NAME);
        if (use_realm == REALM_HISSATSU) {
            out_val.append(format("%-25s %2d %4d", spell_name->data(), s_ptr->slevel, need_mana));
        } else {
            out_val.append(format("%-25s%c%-4s %2d %4d %3d%% %s", spell_name->data(), (max ? '!' : ' '), ryakuji, s_ptr->slevel,
                need_mana, spell_chance(player_ptr, spell, use_realm), comment));
        }

        c_prt(line_attr, out_val, y + i + 1, x);
    }

    prt("", y + i + 1, x);
}
