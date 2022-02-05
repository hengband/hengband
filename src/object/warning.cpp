#include "object/warning.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "inventory/inventory-slot-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-damage-calculator.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player-base/player-race.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/special-defense-types.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 警告を放つアイテムを選択する /
 * Choose one of items that have warning flag
 * Calculate spell damages
 * @return 警告を行う
 */
object_type *choose_warning_item(PlayerType *player_ptr)
{
    int choices[INVEN_TOTAL - INVEN_MAIN_HAND];

    /* Paranoia -- Player has no warning ability */
    if (!player_ptr->warning)
        return nullptr;

    /* Search Inventory */
    int number = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];

        auto flgs = object_flags(o_ptr);
        if (flgs.has(TR_WARNING)) {
            choices[number] = i;
            number++;
        }
    }

    /* Choice one of them */
    return number ? &player_ptr->inventory_list[choices[randint0(number)]] : nullptr;
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する /
 * Calculate spell damages
 * @param m_ptr 魔法を行使するモンスターの構造体参照ポインタ
 * @param typ 効果属性のID
 * @param dam 基本ダメージ
 * @param max 算出した最大ダメージを返すポインタ
 */
static void spell_damcalc(PlayerType *player_ptr, monster_type *m_ptr, AttributeType typ, HIT_POINT dam, int *max)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = r_ptr->level;
    bool ignore_wraith_form = false;

    /* Vulnerability, resistance and immunity */
    switch (typ) {
    case AttributeType::ELEC:
        if (has_immune_elec(player_ptr)) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_elec_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::POIS:
        dam = dam * calc_pois_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::ACID:
        if (has_immune_acid(player_ptr)) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_acid_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::COLD:
    case AttributeType::ICE:
        if (has_immune_cold(player_ptr)) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_cold_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::FIRE:
        if (has_immune_fire(player_ptr)) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_fire_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::PSY_SPEAR:
        ignore_wraith_form = true;
        break;

    case AttributeType::MONSTER_SHOOT:
        if (!player_ptr->blind && (has_invuln_arrow(player_ptr))) {
            dam = 0;
            ignore_wraith_form = true;
        }
        break;

    case AttributeType::LITE:
        dam = dam * calc_lite_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::DARK:
        dam = dam * calc_dark_damage_rate(player_ptr, CALC_MAX) / 100;
        if (has_immune_dark(player_ptr) || player_ptr->wraith_form)
            ignore_wraith_form = true;
        break;

    case AttributeType::SHARDS:
        dam = dam * calc_shards_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::SOUND:
        dam = dam * calc_sound_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::CONFUSION:
        dam = dam * calc_conf_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::CHAOS:
        dam = dam * calc_chaos_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::NETHER:
        dam = dam * calc_nether_damage_rate(player_ptr, CALC_MAX) / 100;
        if (PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
            ignore_wraith_form = true;
            dam = 0;
        }
        break;

    case AttributeType::DISENCHANT:
        dam = dam * calc_disenchant_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::NEXUS:
        dam = dam * calc_nexus_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::TIME:
        dam = dam * calc_time_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::GRAVITY:
        dam = dam * calc_gravity_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::ROCKET:
        dam = dam * calc_rocket_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::NUKE:
        dam = dam * calc_nuke_damage_rate(player_ptr) / 100;
        break;

    case AttributeType::DEATH_RAY:
        dam = dam * calc_deathray_damage_rate(player_ptr, CALC_MAX) / 100;
        if (dam == 0)
            ignore_wraith_form = true;
        break;

    case AttributeType::HOLY_FIRE:
        dam = dam * calc_holy_fire_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::HELL_FIRE:
        dam = dam * calc_hell_fire_damage_rate(player_ptr, CALC_MAX) / 100;
        break;

    case AttributeType::MIND_BLAST:
    case AttributeType::BRAIN_SMASH:
        if (100 + rlev / 2 <= std::max<short>(5, player_ptr->skill_sav)) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;

    case AttributeType::CAUSE_1:
    case AttributeType::CAUSE_2:
    case AttributeType::CAUSE_3:
    case AttributeType::HAND_DOOM:
        if (100 + rlev / 2 <= player_ptr->skill_sav) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;

    case AttributeType::CAUSE_4:
        if ((100 + rlev / 2 <= player_ptr->skill_sav) && (m_ptr->r_idx != MON_KENSHIROU)) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;
    default:
        break;
    }

    if (player_ptr->wraith_form && !ignore_wraith_form) {
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
 * @param spell_num RF4ならRF_ABILITY::SPELL_STARTのように32区切りのベースとなる数値
 * @param typ 効果属性のID
 * @param m_idx 魔法を行使するモンスターのID
 * @param max 算出した最大ダメージを返すポインタ
 */
static void spell_damcalc_by_spellnum(PlayerType *player_ptr, MonsterAbilityType ms_type, AttributeType typ, MONSTER_IDX m_idx, int *max)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    HIT_POINT dam = monspell_damage(player_ptr, ms_type, m_idx, DAM_MAX);
    spell_damcalc(player_ptr, m_ptr, typ, dam, max);
}

/*!
 * @brief 警告基準を定めるためにモンスターの打撃最大ダメージを算出する /
 * Calculate blow damages
 * @param m_ptr 打撃を行使するモンスターの構造体参照ポインタ
 * @param blow_ptr モンスターの打撃能力の構造体参照ポインタ
 * @return 算出された最大ダメージを返す。
 */
static int blow_damcalc(monster_type *m_ptr, PlayerType *player_ptr, monster_blow *blow_ptr)
{
    int dam = blow_ptr->d_dice * blow_ptr->d_side;
    int dummy_max = 0;

    if (blow_ptr->method == RaceBlowMethodType::EXPLODE) {
        dam = (dam + 1) / 2;
        spell_damcalc(player_ptr, m_ptr, mbe_info[enum2i(blow_ptr->effect)].explode_type, dam, &dummy_max);
        dam = dummy_max;
        return dam;
    }

    ARMOUR_CLASS ac = player_ptr->ac + player_ptr->to_a;
    bool check_wraith_form = true;
    switch (blow_ptr->effect) {
    case RaceBlowEffectType::SUPERHURT: {
        int tmp_dam = dam - (dam * ((ac < 150) ? ac : 150) / 250);
        dam = std::max(dam, tmp_dam * 2);
        break;
    }

    case RaceBlowEffectType::HURT:
    case RaceBlowEffectType::SHATTER:
        dam -= (dam * ((ac < 150) ? ac : 150) / 250);
        break;

    case RaceBlowEffectType::ACID:
        spell_damcalc(player_ptr, m_ptr, AttributeType::ACID, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::ELEC:
        spell_damcalc(player_ptr, m_ptr, AttributeType::ELEC, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::FIRE:
        spell_damcalc(player_ptr, m_ptr, AttributeType::FIRE, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::COLD:
        spell_damcalc(player_ptr, m_ptr, AttributeType::COLD, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::DR_MANA:
        dam = 0;
        check_wraith_form = false;
        break;

    default:
        break;
    }

    if (check_wraith_form && player_ptr->wraith_form) {
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
bool process_warning(PlayerType *player_ptr, POSITION xx, POSITION yy)
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

            if (!in_bounds(player_ptr->current_floor_ptr, my, mx) || (distance(my, mx, yy, xx) > WARNING_AWARE_RANGE))
                continue;

            g_ptr = &player_ptr->current_floor_ptr->grid_array[my][mx];

            if (!g_ptr->m_idx)
                continue;

            m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

            if (monster_csleep_remaining(m_ptr))
                continue;
            if (!is_hostile(m_ptr))
                continue;

            r_ptr = &r_info[m_ptr->r_idx];

            /* Monster spells (only powerful ones)*/
            if (projectable(player_ptr, my, mx, yy, xx)) {
                const auto flags = r_ptr->ability_flags;

                if (d_info[player_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::NO_MAGIC)) {
                    if (flags.has(MonsterAbilityType::BA_CHAO))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BA_CHAO, AttributeType::CHAOS, g_ptr->m_idx, &dam_max0);
                    if (flags.has(MonsterAbilityType::BA_MANA))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BA_MANA, AttributeType::MANA, g_ptr->m_idx, &dam_max0);
                    if (flags.has(MonsterAbilityType::BA_DARK))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BA_DARK, AttributeType::DARK, g_ptr->m_idx, &dam_max0);
                    if (flags.has(MonsterAbilityType::BA_LITE))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BA_LITE, AttributeType::LITE, g_ptr->m_idx, &dam_max0);
                    if (flags.has(MonsterAbilityType::HAND_DOOM))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::HAND_DOOM, AttributeType::HAND_DOOM, g_ptr->m_idx, &dam_max0);
                    if (flags.has(MonsterAbilityType::PSY_SPEAR))
                        spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::PSY_SPEAR, AttributeType::PSY_SPEAR, g_ptr->m_idx, &dam_max0);
                }

                if (flags.has(MonsterAbilityType::ROCKET))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::ROCKET, AttributeType::ROCKET, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_ACID))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_ACID, AttributeType::ACID, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_ELEC))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_ELEC, AttributeType::ELEC, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_FIRE))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_FIRE, AttributeType::FIRE, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_COLD))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_COLD, AttributeType::COLD, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_POIS))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_POIS, AttributeType::POIS, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_NETH))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_NETH, AttributeType::NETHER, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_LITE))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_LITE, AttributeType::LITE, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_DARK))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_DARK, AttributeType::DARK, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_CONF))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_CONF, AttributeType::CONFUSION, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_SOUN))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_SOUN, AttributeType::SOUND, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_CHAO))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_CHAO, AttributeType::CHAOS, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_DISE))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_DISE, AttributeType::DISENCHANT, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_NEXU))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_NEXU, AttributeType::NEXUS, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_TIME))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_TIME, AttributeType::TIME, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_INER))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_INER, AttributeType::INERTIAL, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_GRAV))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_GRAV, AttributeType::GRAVITY, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_SHAR))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_SHAR, AttributeType::SHARDS, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_PLAS))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_PLAS, AttributeType::PLASMA, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_FORC))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_FORC, AttributeType::FORCE, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_MANA))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_MANA, AttributeType::MANA, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_NUKE))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_NUKE, AttributeType::NUKE, g_ptr->m_idx, &dam_max0);
                if (flags.has(MonsterAbilityType::BR_DISI))
                    spell_damcalc_by_spellnum(player_ptr, MonsterAbilityType::BR_DISI, AttributeType::DISINTEGRATE, g_ptr->m_idx, &dam_max0);
            }

            /* Monster melee attacks */
            if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW) || d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
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
                if (r_ptr->blow[m].method == RaceBlowMethodType::NONE || (r_ptr->blow[m].method == RaceBlowMethodType::SHOOT))
                    continue;

                /* Extract the attack info */
                dam_melee += blow_damcalc(m_ptr, player_ptr, &r_ptr->blow[m]);
                if (r_ptr->blow[m].method == RaceBlowMethodType::EXPLODE)
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

        if (dam_max > player_ptr->chp / 2) {
            object_type *o_ptr = choose_warning_item(player_ptr);

            if (o_ptr)
                describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            else
                strcpy(o_name, _("体", "body")); /* Warning ability without item */
            msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);

            disturb(player_ptr, false, true);
            return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
        }
    } else
        old_damage = old_damage / 2;

    g_ptr = &player_ptr->current_floor_ptr->grid_array[yy][xx];
    bool is_warning = (!easy_disarm && is_trap(player_ptr, g_ptr->feat)) || (g_ptr->mimic && is_trap(player_ptr, g_ptr->feat));
    is_warning &= !one_in_(13);
    if (!is_warning)
        return true;

    object_type *o_ptr = choose_warning_item(player_ptr);
    if (o_ptr != nullptr)
        describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    else
        strcpy(o_name, _("体", "body")); /* Warning ability without item */
    msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);
    disturb(player_ptr, false, true);
    return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
}
