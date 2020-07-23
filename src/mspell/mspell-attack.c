#include "mspell/mspell-attack.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/targeting.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/improper-mspell-remover.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-selector.h"
#include "mspell/mspell-util.h"
#include "mspell/mspells1.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

#define DO_SPELL_NONE 0
#define DO_SPELL_BR_LITE 1
#define DO_SPELL_BR_DISI 2
#define DO_SPELL_BA_LITE 3

/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 技能を使用するモンスター構造体の参照ポインタ
 * @param yp 最適な目標地点のY座標を返す参照ポインタ
 * @param xp 最適な目標地点のX座標を返す参照ポインタ
 * @param f_flag 射線に入れるのを避ける地形の所持フラグ
 * @param path_check 射線を判定するための関数ポインタ
 * @return 有効な座標があった場合TRUEを返す
 */
static bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, int f_flag,
    bool (*path_check)(player_type *, POSITION, POSITION, POSITION, POSITION))
{
    static int tonari_y[4][8] = { { -1, -1, -1, 0, 0, 1, 1, 1 }, { -1, -1, -1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 0, 0, -1, -1, -1 }, { 1, 1, 1, 0, 0, -1, -1, -1 } };
    static int tonari_x[4][8] = { { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 }, { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 } };

    int next;
    if (m_ptr->fy < target_ptr->y && m_ptr->fx < target_ptr->x)
        next = 0;
    else if (m_ptr->fy < target_ptr->y)
        next = 1;
    else if (m_ptr->fx < target_ptr->x)
        next = 2;
    else
        next = 3;

    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (int i = 0; i < 8; i++) {
        int next_x = *xp + tonari_x[next][i];
        int next_y = *yp + tonari_y[next][i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[next_y][next_x];
        if (!cave_have_flag_grid(g_ptr, f_flag))
            continue;

        if (path_check(target_ptr, m_ptr->fy, m_ptr->fx, next_y, next_x)) {
            *yp = next_y;
            *xp = next_x;
            return TRUE;
        }
    }

    return FALSE;
}

/*!
 * todo メインルーチンの割に長過ぎる。要分割
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 */
bool make_attack_spell(player_type *target_ptr, MONSTER_IDX m_idx)
{
#ifdef JP
#else
    char m_poss[80];
#endif
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (monster_confused_remaining(m_ptr)) {
        reset_target(m_ptr);
        return FALSE;
    }

    if (m_ptr->mflag & MFLAG_NICE)
        return FALSE;
    if (!is_hostile(m_ptr))
        return FALSE;

    bool no_inate = FALSE;
    if (randint0(100) >= (r_ptr->freq_spell * 2))
        no_inate = TRUE;

    BIT_FLAGS f4 = r_ptr->flags4;
    BIT_FLAGS f5 = r_ptr->a_ability_flags1;
    BIT_FLAGS f6 = r_ptr->a_ability_flags2;

    if ((m_ptr->cdis > get_max_range(target_ptr)) && !m_ptr->target_y)
        return FALSE;

    POSITION x = target_ptr->x;
    POSITION y = target_ptr->y;
    POSITION x_br_lite = 0;
    POSITION y_br_lite = 0;
    if (f4 & RF4_BR_LITE) {
        y_br_lite = y;
        x_br_lite = x;

        if (los(target_ptr, m_ptr->fy, m_ptr->fx, y_br_lite, x_br_lite)) {
            feature_type *f_ptr = &f_info[floor_ptr->grid_array[y_br_lite][x_br_lite].feat];

            if (!have_flag(f_ptr->flags, FF_LOS)) {
                if (have_flag(f_ptr->flags, FF_PROJECT) && one_in_(2))
                    f4 &= ~(RF4_BR_LITE);
            }
        } else if (!adjacent_grid_check(target_ptr, m_ptr, &y_br_lite, &x_br_lite, FF_LOS, los))
            f4 &= ~(RF4_BR_LITE);

        if (!(f4 & RF4_BR_LITE)) {
            y_br_lite = 0;
            x_br_lite = 0;
        }
    }

    bool do_spell = DO_SPELL_NONE;
    if (projectable(target_ptr, m_ptr->fy, m_ptr->fx, y, x)) {
        feature_type *f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];
        if (!have_flag(f_ptr->flags, FF_PROJECT)) {
            if ((f4 & RF4_BR_DISI) && have_flag(f_ptr->flags, FF_HURT_DISI) && one_in_(2))
                do_spell = DO_SPELL_BR_DISI;
            else if ((f4 & RF4_BR_LITE) && have_flag(f_ptr->flags, FF_LOS) && one_in_(2))
                do_spell = DO_SPELL_BR_LITE;
        }
    } else {
        bool success = FALSE;
        if ((f4 & RF4_BR_DISI) && (m_ptr->cdis < get_max_range(target_ptr) / 2) && in_disintegration_range(floor_ptr, m_ptr->fy, m_ptr->fx, y, x)
            && (one_in_(10) || (projectable(target_ptr, y, x, m_ptr->fy, m_ptr->fx) && one_in_(2)))) {
            do_spell = DO_SPELL_BR_DISI;
            success = TRUE;
        } else if ((f4 & RF4_BR_LITE) && (m_ptr->cdis < get_max_range(target_ptr) / 2) && los(target_ptr, m_ptr->fy, m_ptr->fx, y, x) && one_in_(5)) {
            do_spell = DO_SPELL_BR_LITE;
            success = TRUE;
        } else if ((f5 & RF5_BA_LITE) && (m_ptr->cdis <= get_max_range(target_ptr))) {
            POSITION by = y, bx = x;
            get_project_point(target_ptr, m_ptr->fy, m_ptr->fx, &by, &bx, 0L);
            if ((distance(by, bx, y, x) <= 3) && los(target_ptr, by, bx, y, x) && one_in_(5)) {
                do_spell = DO_SPELL_BA_LITE;
                success = TRUE;
            }
        }

        if (!success)
            success = adjacent_grid_check(target_ptr, m_ptr, &y, &x, FF_PROJECT, projectable);

        if (!success) {
            if (m_ptr->target_y && m_ptr->target_x) {
                y = m_ptr->target_y;
                x = m_ptr->target_x;
                f4 &= RF4_INDIRECT_MASK;
                f5 &= RF5_INDIRECT_MASK;
                f6 &= RF6_INDIRECT_MASK;
                success = TRUE;
            }

            if (y_br_lite && x_br_lite && (m_ptr->cdis < get_max_range(target_ptr) / 2) && one_in_(5)) {
                if (!success) {
                    y = y_br_lite;
                    x = x_br_lite;
                    do_spell = DO_SPELL_BR_LITE;
                    success = TRUE;
                } else
                    f4 |= (RF4_BR_LITE);
            }
        }

        if (!success)
            return FALSE;
    }

    reset_target(m_ptr);
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    if (no_inate) {
        f4 &= ~(RF4_NOMAGIC_MASK);
        f5 &= ~(RF5_NOMAGIC_MASK);
        f6 &= ~(RF6_NOMAGIC_MASK);
    }

    bool can_use_lite_area = FALSE;
    if (f6 & RF6_DARKNESS) {
        if ((target_ptr->pclass == CLASS_NINJA) && !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) && !(r_ptr->flags7 & RF7_DARK_MASK))
            can_use_lite_area = TRUE;

        if (!(r_ptr->flags2 & RF2_STUPID)) {
            if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
                f6 &= ~(RF6_DARKNESS);
            else if ((target_ptr->pclass == CLASS_NINJA) && !can_use_lite_area)
                f6 &= ~(RF6_DARKNESS);
        }
    }

    bool in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID)) {
        f4 &= (RF4_NOMAGIC_MASK);
        f5 &= (RF5_NOMAGIC_MASK);
        f6 &= (RF6_NOMAGIC_MASK);
    }

    if (r_ptr->flags2 & RF2_SMART) {
        if ((m_ptr->hp < m_ptr->maxhp / 10) && (randint0(100) < 50)) {
            f4 &= (RF4_INT_MASK);
            f5 &= (RF5_INT_MASK);
            f6 &= (RF6_INT_MASK);
        }

        if ((f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, 0)) {
            f6 &= ~(RF6_TELE_LEVEL);
        }
    }

    if (!f4 && !f5 && !f6)
        return FALSE;

    remove_bad_spells(m_idx, target_ptr, &f4, &f5, &f6);
    if (floor_ptr->inside_arena || target_ptr->phase_out) {
        f4 &= ~(RF4_SUMMON_MASK);
        f5 &= ~(RF5_SUMMON_MASK);
        f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

        if (m_ptr->r_idx == MON_ROLENTO)
            f6 &= ~(RF6_SPECIAL);
    }

    if (!f4 && !f5 && !f6)
        return FALSE;

    if (!(r_ptr->flags2 & RF2_STUPID)) {
        if (!target_ptr->csp)
            f5 &= ~(RF5_DRAIN_MANA);

        if (((f4 & RF4_BOLT_MASK) || (f5 & RF5_BOLT_MASK) || (f6 & RF6_BOLT_MASK))
            && !clean_shot(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x, FALSE)) {
            f4 &= ~(RF4_BOLT_MASK);
            f5 &= ~(RF5_BOLT_MASK);
            f6 &= ~(RF6_BOLT_MASK);
        }

        if (((f4 & RF4_SUMMON_MASK) || (f5 & RF5_SUMMON_MASK) || (f6 & RF6_SUMMON_MASK)) && !(summon_possible(target_ptr, y, x))) {
            f4 &= ~(RF4_SUMMON_MASK);
            f5 &= ~(RF5_SUMMON_MASK);
            f6 &= ~(RF6_SUMMON_MASK);
        }

        if ((f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, m_ptr)) {
            f6 &= ~(RF6_RAISE_DEAD);
        }

        if (f6 & RF6_SPECIAL) {
            if ((m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, y, x)) {
                f6 &= ~(RF6_SPECIAL);
            }
        }

        if (!f4 && !f5 && !f6)
            return FALSE;
    }

    byte spell[96], num = 0;
    for (int k = 0; k < 32; k++) {
        if (f4 & (1L << k))
            spell[num++] = k + RF4_SPELL_START;
    }

    for (int k = 0; k < 32; k++) {
        if (f5 & (1L << k))
            spell[num++] = k + RF5_SPELL_START;
    }

    for (int k = 0; k < 32; k++) {
        if (f6 & (1L << k))
            spell[num++] = k + RF6_SPELL_START;
    }

    if (!num)
        return FALSE;

    if (!target_ptr->playing || target_ptr->is_dead)
        return FALSE;

    if (target_ptr->leaving)
        return FALSE;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(target_ptr, m_name, m_ptr, 0x00);

#ifdef JP
#else
    /* Get the monster possessive ("his"/"her"/"its") */
    monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

    SPELL_IDX thrown_spell = 0;
    switch (do_spell) {
    case DO_SPELL_NONE: {
        int attempt = 10;
        while (attempt--) {
            thrown_spell = choose_attack_spell(target_ptr, m_idx, spell, num);
            if (thrown_spell)
                break;
        }
        break;
    }
    case DO_SPELL_BR_LITE:
        thrown_spell = 96 + 14; /* RF4_BR_LITE */
        break;
    case DO_SPELL_BR_DISI:
        thrown_spell = 96 + 31; /* RF4_BR_DISI */
        break;
    case DO_SPELL_BA_LITE:
        thrown_spell = 128 + 20; /* RF5_BA_LITE */
        break;
    default:
        return FALSE;
    }

    if (!thrown_spell)
        return FALSE;

    PERCENTAGE failrate = 25 - (rlev + 3) / 4;

    if (r_ptr->flags2 & RF2_STUPID)
        failrate = 0;

    if (!spell_is_inate(thrown_spell) && (in_no_magic_dungeon || (monster_stunned_remaining(m_ptr) && one_in_(2)) || (randint0(100) < failrate))) {
        disturb(target_ptr, TRUE, TRUE);
        msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), m_name);

        return TRUE;
    }

    if (!spell_is_inate(thrown_spell) && magic_barrier(target_ptr, m_idx)) {
        msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), m_name);
        return TRUE;
    }

    bool direct = player_bold(target_ptr, y, x);
    bool can_remember = is_original_ap_and_seen(target_ptr, m_ptr);
    if (!direct) {
        switch (thrown_spell) {
        case 96 + 2: /* RF4_DISPEL */
        case 96 + 4: /* RF4_SHOOT */
        case 128 + 9: /* RF5_DRAIN_MANA */
        case 128 + 10: /* RF5_MIND_BLAST */
        case 128 + 11: /* RF5_BRAIN_SMASH */
        case 128 + 12: /* RF5_CAUSE_1 */
        case 128 + 13: /* RF5_CAUSE_2 */
        case 128 + 14: /* RF5_CAUSE_3 */
        case 128 + 15: /* RF5_CAUSE_4 */
        case 128 + 16: /* RF5_BO_ACID */
        case 128 + 17: /* RF5_BO_ELEC */
        case 128 + 18: /* RF5_BO_FIRE */
        case 128 + 19: /* RF5_BO_COLD */
        case 128 + 21: /* RF5_BO_NETH */
        case 128 + 22: /* RF5_BO_WATE */
        case 128 + 23: /* RF5_BO_MANA */
        case 128 + 24: /* RF5_BO_PLAS */
        case 128 + 25: /* RF5_BO_ICEE */
        case 128 + 26: /* RF5_MISSILE */
        case 128 + 27: /* RF5_SCARE */
        case 128 + 28: /* RF5_BLIND */
        case 128 + 29: /* RF5_CONF */
        case 128 + 30: /* RF5_SLOW */
        case 128 + 31: /* RF5_HOLD */
        case 160 + 1: /* RF6_HAND_DOOM */
        case 160 + 8: /* RF6_TELE_TO */
        case 160 + 9: /* RF6_TELE_AWAY */
        case 160 + 10: /* RF6_TELE_LEVEL */
        case 160 + 11: /* RF6_PSY_SPEAR */
        case 160 + 12: /* RF6_DARKNESS */
        case 160 + 14: /* RF6_FORGET */
            return FALSE;
        }
    }

    int dam = monspell_to_player(target_ptr, thrown_spell, y, x, m_idx);
    if (dam < 0)
        return FALSE;

    if ((target_ptr->action == ACTION_LEARN) && thrown_spell > 175) {
        learn_spell(target_ptr, thrown_spell - 96);
    }

    bool seen = (!target_ptr->blind && m_ptr->ml);
    bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
    if (seen && maneable && !current_world_ptr->timewalk_m_idx && (target_ptr->pclass == CLASS_IMITATOR)) {
        if (thrown_spell != 167) /* Not RF6_SPECIAL */
        {
            if (target_ptr->mane_num == MAX_MANE) {
                int i;
                target_ptr->mane_num--;
                for (i = 0; i < target_ptr->mane_num; i++) {
                    target_ptr->mane_spell[i] = target_ptr->mane_spell[i + 1];
                    target_ptr->mane_dam[i] = target_ptr->mane_dam[i + 1];
                }
            }
            target_ptr->mane_spell[target_ptr->mane_num] = thrown_spell - 96;
            target_ptr->mane_dam[target_ptr->mane_num] = dam;
            target_ptr->mane_num++;
            target_ptr->new_mane = TRUE;

            target_ptr->redraw |= PR_IMITATION;
        }
    }

    if (can_remember) {
        if (thrown_spell < 32 * 4) {
            r_ptr->r_flags4 |= (1L << (thrown_spell - 32 * 3));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < 32 * 5) {
            r_ptr->r_flags5 |= (1L << (thrown_spell - 32 * 4));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < 32 * 6) {
            r_ptr->r_flags6 |= (1L << (thrown_spell - 32 * 5));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        }
    }

    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena) {
        r_ptr->r_deaths++;
    }

    return TRUE;
}
