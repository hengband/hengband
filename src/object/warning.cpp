﻿#include "object/warning.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/special-defense-types.h"
#include "spell/spell-types.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 警告を放つアイテムを選択する /
 * Choose one of items that have warning flag
 * Calculate spell damages
 * @return 警告を行う
 */
object_type *choose_warning_item(player_type *creature_ptr)
{
    int choices[INVEN_TOTAL - INVEN_MAIN_HAND];

    /* Paranoia -- Player has no warning ability */
    if (!creature_ptr->warning)
        return NULL;

    /* Search Inventory */
    int number = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_type *o_ptr = &creature_ptr->inventory_list[i];

        object_flags(creature_ptr, o_ptr, flgs);
        if (has_flag(flgs, TR_WARNING)) {
            choices[number] = i;
            number++;
        }
    }

    /* Choice one of them */
    return number ? &creature_ptr->inventory_list[choices[randint0(number)]] : NULL;
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する /
 * Calculate spell damages
 * @param m_ptr 魔法を行使するモンスターの構造体参照ポインタ
 * @param typ 効果属性のID
 * @param dam 基本ダメージ
 * @param max 算出した最大ダメージを返すポインタ
 * @return なし
 */
static void spell_damcalc(player_type *target_ptr, monster_type *m_ptr, EFFECT_ID typ, HIT_POINT dam, int *max)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = r_ptr->level;
    bool ignore_wraith_form = FALSE;

    /* Vulnerability, resistance and immunity */
    switch (typ) {
    case GF_ELEC:
        if (has_immune_elec(target_ptr)) {
            ignore_wraith_form = TRUE;
        }
        dam = dam * calc_elec_damage_rate(target_ptr) / 100;
        break;

    case GF_POIS:
        dam = dam * calc_pois_damage_rate(target_ptr) / 100;
        break;

    case GF_ACID:
        if (has_immune_acid(target_ptr)) {
            ignore_wraith_form = TRUE;
        }
        dam = dam * calc_acid_damage_rate(target_ptr) / 100;
        break;

    case GF_COLD:
    case GF_ICE:
        if (has_immune_cold(target_ptr)) {
            ignore_wraith_form = TRUE;
        }
        dam = dam * calc_cold_damage_rate(target_ptr) / 100;
        break;

    case GF_FIRE:
        if (has_immune_fire(target_ptr)) {
            ignore_wraith_form = TRUE;
        }
        dam = dam * calc_fire_damage_rate(target_ptr) / 100;
        break;

    case GF_PSY_SPEAR:
        ignore_wraith_form = TRUE;
        break;

    case GF_ARROW:
        if (!target_ptr->blind && (has_invuln_arrow(target_ptr))) {
            dam = 0;
            ignore_wraith_form = TRUE;
        }
        break;

    case GF_LITE:
        dam = dam * calc_lite_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_DARK:
        dam = dam * calc_dark_damage_rate(target_ptr, CALC_MAX) / 100;
        if (is_specific_player_race(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE) || target_ptr->wraith_form)
            ignore_wraith_form = TRUE;
        break;

    case GF_SHARDS:
        dam = dam * calc_shards_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_SOUND:
        dam = dam * calc_sound_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_CONFUSION:
        dam = dam * calc_conf_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_CHAOS:
        dam = dam * calc_chaos_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_NETHER:
        dam = dam * calc_nether_damage_rate(target_ptr, CALC_MAX) / 100;
        if (is_specific_player_race(target_ptr, RACE_SPECTRE)) {
            ignore_wraith_form = TRUE;
            dam = 0;
        }
        break;

    case GF_DISENCHANT:
        dam = dam * calc_disenchant_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_NEXUS:
        dam = dam * calc_nexus_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_TIME:
        dam = dam * calc_time_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_GRAVITY:
        dam = dam * calc_gravity_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_ROCKET:
        dam = dam * calc_rocket_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_NUKE:
        dam = dam * calc_nuke_damage_rate(target_ptr) / 100;
        break;

    case GF_DEATH_RAY:
        dam = dam * calc_deathray_damage_rate(target_ptr, CALC_MAX) / 100;
        if (dam == 0)
            ignore_wraith_form = TRUE;
        break;

    case GF_HOLY_FIRE:
        dam = dam * calc_holy_fire_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_HELL_FIRE:
        dam = dam * calc_hell_fire_damage_rate(target_ptr, CALC_MAX) / 100;
        break;

    case GF_MIND_BLAST:
    case GF_BRAIN_SMASH:
        if (100 + rlev / 2 <= MAX(5, target_ptr->skill_sav)) {
            dam = 0;
            ignore_wraith_form = TRUE;
        }

        break;

    case GF_CAUSE_1:
    case GF_CAUSE_2:
    case GF_CAUSE_3:
    case GF_HAND_DOOM:
        if (100 + rlev / 2 <= target_ptr->skill_sav) {
            dam = 0;
            ignore_wraith_form = TRUE;
        }

        break;

    case GF_CAUSE_4:
        if ((100 + rlev / 2 <= target_ptr->skill_sav) && (m_ptr->r_idx != MON_KENSHIROU)) {
            dam = 0;
            ignore_wraith_form = TRUE;
        }

        break;
    }

    if (target_ptr->wraith_form && !ignore_wraith_form) {
        dam /= 2;
        if (!dam)
            dam = 1;
    }

    if (dam > *max)
        *max = dam;
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する。 /
 * Calculate spell damages
 * @param spell_num RF4ならRF4_SPELL_STARTのように32区切りのベースとなる数値
 * @param typ 効果属性のID
 * @param m_idx 魔法を行使するモンスターのID
 * @param max 算出した最大ダメージを返すポインタ
 * @return なし
 */
void spell_damcalc_by_spellnum(player_type *creature_ptr, monster_spell_type ms_type, EFFECT_ID typ, MONSTER_IDX m_idx, int *max)
{
    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
    HIT_POINT dam = monspell_damage(creature_ptr, ms_type, m_idx, DAM_MAX);
    spell_damcalc(creature_ptr, m_ptr, typ, dam, max);
}

/*!
 * @brief 警告基準を定めるためにモンスターの打撃最大ダメージを算出する /
 * Calculate blow damages
 * @param m_ptr 打撃を行使するモンスターの構造体参照ポインタ
 * @param blow_ptr モンスターの打撃能力の構造体参照ポインタ
 * @return 算出された最大ダメージを返す。
 */
static int blow_damcalc(monster_type *m_ptr, player_type *target_ptr, monster_blow *blow_ptr)
{
    int dam = blow_ptr->d_dice * blow_ptr->d_side;
    int dummy_max = 0;

    if (blow_ptr->method == RBM_EXPLODE) {
        dam = (dam + 1) / 2;
        spell_damcalc(target_ptr, m_ptr, mbe_info[blow_ptr->effect].explode_type, dam, &dummy_max);
        dam = dummy_max;
        return dam;
    }

    ARMOUR_CLASS ac = target_ptr->ac + target_ptr->to_a;
    bool check_wraith_form = TRUE;
    switch (blow_ptr->effect) {
    case RBE_SUPERHURT: {
        int tmp_dam = dam - (dam * ((ac < 150) ? ac : 150) / 250);
        dam = MAX(dam, tmp_dam * 2);
        break;
    }

    case RBE_HURT:
    case RBE_SHATTER:
        dam -= (dam * ((ac < 150) ? ac : 150) / 250);
        break;

    case RBE_ACID:
        spell_damcalc(target_ptr, m_ptr, GF_ACID, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = FALSE;
        break;

    case RBE_ELEC:
        spell_damcalc(target_ptr, m_ptr, GF_ELEC, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = FALSE;
        break;

    case RBE_FIRE:
        spell_damcalc(target_ptr, m_ptr, GF_FIRE, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = FALSE;
        break;

    case RBE_COLD:
        spell_damcalc(target_ptr, m_ptr, GF_COLD, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = FALSE;
        break;

    case RBE_DR_MANA:
        dam = 0;
        check_wraith_form = FALSE;
        break;
    }

    if (check_wraith_form && target_ptr->wraith_form) {
        dam /= 2;
        if (!dam)
            dam = 1;
    }

    return dam;
}

/*!
 * @brief プレイヤーが特定地点へ移動した場合に警告を発する処理 /
 * Examine the grid (xx,yy) and warn the player if there are any danger
 * @param xx 危険性を調査するマスのX座標
 * @param yy 危険性を調査するマスのY座標
 * @return 警告を無視して進むことを選択するかか問題が無ければTRUE、警告に従ったならFALSEを返す。
 */
bool process_warning(player_type *creature_ptr, POSITION xx, POSITION yy)
{
    POSITION mx, my;
    grid_type *g_ptr;
    GAME_TEXT o_name[MAX_NLEN];

#define WARNING_AWARE_RANGE 12
    int dam_max = 0;
    static int old_damage = 0;

    for (mx = xx - WARNING_AWARE_RANGE; mx < xx + WARNING_AWARE_RANGE + 1; mx++) {
        for (my = yy - WARNING_AWARE_RANGE; my < yy + WARNING_AWARE_RANGE + 1; my++) {
            int dam_max0 = 0;
            monster_type *m_ptr;
            monster_race *r_ptr;

            if (!in_bounds(creature_ptr->current_floor_ptr, my, mx) || (distance(my, mx, yy, xx) > WARNING_AWARE_RANGE))
                continue;

            g_ptr = &creature_ptr->current_floor_ptr->grid_array[my][mx];

            if (!g_ptr->m_idx)
                continue;

            m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

            if (monster_csleep_remaining(m_ptr))
                continue;
            if (!is_hostile(m_ptr))
                continue;

            r_ptr = &r_info[m_ptr->r_idx];

            /* Monster spells (only powerful ones)*/
            if (projectable(creature_ptr, my, mx, yy, xx)) {
                BIT_FLAGS f4 = r_ptr->flags4;
                BIT_FLAGS f5 = r_ptr->a_ability_flags1;
                BIT_FLAGS f6 = r_ptr->a_ability_flags2;

                if (!(d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC)) {
                    if (f4 & RF4_BA_CHAO)
                        spell_damcalc_by_spellnum(creature_ptr, MS_BALL_CHAOS, GF_CHAOS, g_ptr->m_idx, &dam_max0);
                    if (f5 & RF5_BA_MANA)
                        spell_damcalc_by_spellnum(creature_ptr, MS_BALL_MANA, GF_MANA, g_ptr->m_idx, &dam_max0);
                    if (f5 & RF5_BA_DARK)
                        spell_damcalc_by_spellnum(creature_ptr, MS_BALL_DARK, GF_DARK, g_ptr->m_idx, &dam_max0);
                    if (f5 & RF5_BA_LITE)
                        spell_damcalc_by_spellnum(creature_ptr, MS_STARBURST, GF_LITE, g_ptr->m_idx, &dam_max0);
                    if (f6 & RF6_HAND_DOOM)
                        spell_damcalc_by_spellnum(creature_ptr, MS_HAND_DOOM, GF_HAND_DOOM, g_ptr->m_idx, &dam_max0);
                    if (f6 & RF6_PSY_SPEAR)
                        spell_damcalc_by_spellnum(creature_ptr, MS_PSY_SPEAR, GF_PSY_SPEAR, g_ptr->m_idx, &dam_max0);
                }

                if (f4 & RF4_ROCKET)
                    spell_damcalc_by_spellnum(creature_ptr, MS_ROCKET, GF_ROCKET, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_ACID)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_ACID, GF_ACID, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_ELEC)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_ELEC, GF_ELEC, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_FIRE)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_FIRE, GF_FIRE, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_COLD)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_COLD, GF_COLD, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_POIS)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_POIS, GF_POIS, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_NETH)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_NETHER, GF_NETHER, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_LITE)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_LITE, GF_LITE, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_DARK)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_DARK, GF_DARK, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_CONF)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_CONF, GF_CONFUSION, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_SOUN)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_SOUND, GF_SOUND, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_CHAO)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_CHAOS, GF_CHAOS, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_DISE)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_DISEN, GF_DISENCHANT, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_NEXU)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_NEXUS, GF_NEXUS, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_TIME)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_TIME, GF_TIME, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_INER)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_INERTIA, GF_INERTIAL, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_GRAV)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_GRAVITY, GF_GRAVITY, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_SHAR)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_SHARDS, GF_SHARDS, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_PLAS)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_PLASMA, GF_PLASMA, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_WALL)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_FORCE, GF_FORCE, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_MANA)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_MANA, GF_MANA, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_NUKE)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_NUKE, GF_NUKE, g_ptr->m_idx, &dam_max0);
                if (f4 & RF4_BR_DISI)
                    spell_damcalc_by_spellnum(creature_ptr, MS_BR_DISI, GF_DISINTEGRATE, g_ptr->m_idx, &dam_max0);
            }

            /* Monster melee attacks */
            if ((r_ptr->flags1 & RF1_NEVER_BLOW) || (d_info[creature_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)) {
                dam_max += dam_max0;
                continue;
            }

            if (!(mx <= xx + 1 && mx >= xx - 1 && my <= yy + 1 && my >= yy - 1)) {
                dam_max += dam_max0;
                continue;
            }

            int dam_melee = 0;
            for (int m = 0; m < 4; m++) {
                /* Skip non-attacks */
                if (!r_ptr->blow[m].method || (r_ptr->blow[m].method == RBM_SHOOT))
                    continue;

                /* Extract the attack info */
                dam_melee += blow_damcalc(m_ptr, creature_ptr, &r_ptr->blow[m]);
                if (r_ptr->blow[m].method == RBM_EXPLODE)
                    break;
            }

            if (dam_melee > dam_max0)
                dam_max0 = dam_melee;
            dam_max += dam_max0;
        }
    }

    /* Prevent excessive warning */
    if (dam_max > old_damage) {
        old_damage = dam_max * 3 / 2;

        if (dam_max > creature_ptr->chp / 2) {
            object_type *o_ptr = choose_warning_item(creature_ptr);

            if (o_ptr)
                describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            else
                strcpy(o_name, _("体", "body")); /* Warning ability without item */
            msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);

            disturb(creature_ptr, FALSE, TRUE);
            return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
        }
    } else
        old_damage = old_damage / 2;

    g_ptr = &creature_ptr->current_floor_ptr->grid_array[yy][xx];
    bool is_warning = (!easy_disarm && is_trap(creature_ptr, g_ptr->feat)) || (g_ptr->mimic && is_trap(creature_ptr, g_ptr->feat));
    is_warning &= !one_in_(13);
    if (!is_warning)
        return TRUE;

    object_type *o_ptr = choose_warning_item(creature_ptr);
    if (o_ptr != NULL)
        describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    else
        strcpy(o_name, _("体", "body")); /* Warning ability without item */
    msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);
    disturb(creature_ptr, FALSE, TRUE);
    return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
}
