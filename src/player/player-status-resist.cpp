#include "player/player-status-resist.h"
#include "artifact/fixed-art-types.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"

/*!
 * @brief 耐性倍率計算の用途に応じた分岐処理
 */
PERCENTAGE randrate(int dice, int fix, rate_calc_type_mode mode)
{
    switch (mode) {
    case CALC_RAND:
        return randint1(dice) * 100 + fix * 100;
        break;
    case CALC_AVERAGE:
        return (dice + 1) * 50 + fix * 100;
        break;
    case CALC_MIN:
        return (fix + 1) * 100;
        break;
    case CALC_MAX:
        return (dice + fix) * 100;
        break;
    default:
        return (fix + 1) * 100;
        break;
    }
}

/*!
 * @brief 酸属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_acid_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;

    if (has_immune_acid(creature_ptr)) {
        return 0;
    }

    BIT_FLAGS flgs = has_vuln_acid(creature_ptr);
    
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flgs, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_acid(creature_ptr))
        per = (per + 2) / 3;
    if (is_oppose_acid(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

/*!
 * @brief 電撃属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_elec_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;

    if (has_immune_elec(creature_ptr)) {
        return 0;
    }

    BIT_FLAGS flgs = has_vuln_elec(creature_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flgs, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_elec(creature_ptr))
        per = (per + 2) / 3;
    if (is_oppose_elec(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

/*!
 * @brief 火炎属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_fire_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    BIT_FLAGS flgs = has_vuln_fire(creature_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flgs, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    /* Resist the damage */
    if (has_resist_fire(creature_ptr))
        per = (per + 2) / 3;
    if (is_oppose_fire(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

/*!
 * @brief 冷気属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_cold_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    BIT_FLAGS flgs = has_vuln_cold(creature_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flgs, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_cold(creature_ptr))
        per = (per + 2) / 3;
    if (is_oppose_cold(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

/*!
 * @brief 毒属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_pois_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    if (has_resist_pois(creature_ptr))
        per = (per + 2) / 3;
    if (is_oppose_pois(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

/*!
 * @brief 放射性廃棄物攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nuke_damage_rate(player_type *creature_ptr)
{

    PERCENTAGE per = 100;
    if (has_resist_pois(creature_ptr))
        per = (2 * per + 2) / 5;
    if (is_oppose_pois(creature_ptr))
        per = (2 * per + 2) / 5;

    return per;
}

/*!
 * @brief 死の光線に対するダメージ倍率計算
 */
PERCENTAGE calc_deathray_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    if (creature_ptr->mimic_form) {
        if (mimic_info[creature_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING) {
            return 0;
        }
    }

    switch (creature_ptr->prace) {
    case player_race_type::GOLEM:
    case player_race_type::SKELETON:
    case player_race_type::ZOMBIE:
    case player_race_type::VAMPIRE:
    case player_race_type::BALROG:
    case player_race_type::SPECTRE:
        return 0;
        break;

    default:
        break;
    }

    return 100;
}

/*!
 * @brief 閃光属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_lite_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (player_race_has_flag(creature_ptr, TR_VUL_LITE)) {
        switch (player_race_life(creature_ptr)) {
        case PlayerRaceLife::UNDEAD:
            per *= 2;
            break;
        default:
            per = per * 4 / 3;
            break;
        }
    }

    if (has_resist_lite(creature_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    if (creature_ptr->wraith_form)
        per *= 2;

    return per;
}

/*!
 * @brief 暗黒属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_dark_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_immune_dark(creature_ptr))
        return 0;

    if (has_resist_dark(creature_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 破片属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_shards_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_shard(creature_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 轟音属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_sound_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_sound(creature_ptr)) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 混乱属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_conf_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_conf(creature_ptr)) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 混沌属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_chaos_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_chaos(creature_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 劣化属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_disenchant_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_disen(creature_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 因果混乱属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nexus_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_disen(creature_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief ロケット属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_rocket_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_shard(creature_ptr)) {
        per /= 2;
    }

    return per;
}

/*!
 * @brief 地獄属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nether_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_neth(creature_ptr)) {
        if (!is_specific_player_race(creature_ptr, player_race_type::SPECTRE))
            per *= 6;
        per *= 100;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 時間逆転攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_time_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_time(creature_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 水流攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_water_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_water(creature_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 聖なる火炎攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_holy_fire_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (creature_ptr->alignment > 10)
        per /= 2;
    else if (creature_ptr->alignment < -10)
        per *= 2;
    return per;
}

/*!
 * @brief 地獄の火炎攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_hell_fire_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (creature_ptr->alignment > 10)
        per *= 2;
    return per;
}

/*!
 * @brief 重力攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_gravity_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (creature_ptr->levitation) {
        per = (per * 2) / 3;
    }
    return per;
}

/*!
 * @brief 虚無攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_void_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (creature_ptr->tim_pass_wall) {
        per = per * 3 / 2;
    } else if (creature_ptr->anti_tele) {
        per *= 400;
        per /= randrate(4, 7, mode);
    } else if (creature_ptr->levitation) {
        per = (per * 2) / 3;
    }
    return per;
}

/*!
 * @brief 深淵攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_abyss_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_dark(creature_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    } else if (!creature_ptr->levitation && creature_ptr->anti_tele) {
        per = (per * 5) / 4;
    }
    return per;
}
