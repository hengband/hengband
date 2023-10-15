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
#include "object-hook/hook-weapon.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
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
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
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
PERCENTAGE calc_acid_damage_rate(PlayerType *player_ptr)
{
    PERCENTAGE per = 100;

    if (has_immune_acid(player_ptr)) {
        return 0;
    }

    BIT_FLAGS flags = has_vuln_acid(player_ptr);

    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flags, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_acid(player_ptr)) {
        per = (per + 2) / 3;
    }
    if (is_oppose_acid(player_ptr)) {
        per = (per + 2) / 3;
    }

    return per;
}

/*!
 * @brief 電撃属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_elec_damage_rate(PlayerType *player_ptr)
{
    PERCENTAGE per = 100;

    if (has_immune_elec(player_ptr)) {
        return 0;
    }

    BIT_FLAGS flags = has_vuln_elec(player_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flags, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_elec(player_ptr)) {
        per = (per + 2) / 3;
    }
    if (is_oppose_elec(player_ptr)) {
        per = (per + 2) / 3;
    }

    return per;
}

/*!
 * @brief 火炎属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_fire_damage_rate(PlayerType *player_ptr)
{
    PERCENTAGE per = 100;
    BIT_FLAGS flags = has_vuln_fire(player_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flags, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    /* Resist the damage */
    if (has_resist_fire(player_ptr)) {
        per = (per + 2) / 3;
    }
    if (is_oppose_fire(player_ptr)) {
        per = (per + 2) / 3;
    }

    return per;
}

/*!
 * @brief 冷気属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_cold_damage_rate(PlayerType *player_ptr)
{
    PERCENTAGE per = 100;
    BIT_FLAGS flags = has_vuln_cold(player_ptr);
    for (BIT_FLAGS check_flag = 0x01U; check_flag < FLAG_CAUSE_MAX; check_flag <<= 1) {
        if (any_bits(flags, check_flag)) {
            if (check_flag == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (has_resist_cold(player_ptr)) {
        per = (per + 2) / 3;
    }
    if (is_oppose_cold(player_ptr)) {
        per = (per + 2) / 3;
    }

    return per;
}

/*!
 * @brief 毒属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_pois_damage_rate(PlayerType *player_ptr)
{
    PERCENTAGE per = 100;
    if (has_resist_pois(player_ptr)) {
        per = (per + 2) / 3;
    }
    if (is_oppose_pois(player_ptr)) {
        per = (per + 2) / 3;
    }

    return per;
}

/*!
 * @brief 放射性廃棄物攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nuke_damage_rate(PlayerType *player_ptr)
{

    PERCENTAGE per = 100;
    if (has_resist_pois(player_ptr)) {
        per = (2 * per + 2) / 5;
    }
    if (is_oppose_pois(player_ptr)) {
        per = (2 * per + 2) / 5;
    }

    return per;
}

/*!
 * @brief 死の光線に対するダメージ倍率計算
 */
PERCENTAGE calc_deathray_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    if (player_ptr->mimic_form != MimicKindType::NONE) {
        if (PlayerRace(player_ptr).is_mimic_nonliving()) {
            return 0;
        }
    }

    switch (player_ptr->prace) {
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::BALROG:
    case PlayerRaceType::SPECTRE:
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
PERCENTAGE calc_lite_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    PlayerRace race(player_ptr);

    if (race.tr_flags().has(TR_VUL_LITE)) {
        switch (race.life()) {
        case PlayerRaceLifeType::UNDEAD:
            per *= 2;
            break;
        default:
            per = per * 4 / 3;
            break;
        }
    }

    if (has_resist_lite(player_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    if (player_ptr->wraith_form) {
        per *= 2;
    }

    return per;
}

/*!
 * @brief 暗黒属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_dark_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_immune_dark(player_ptr)) {
        return 0;
    }

    if (has_resist_dark(player_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 破片属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_shards_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_shard(player_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 轟音属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_sound_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_sound(player_ptr)) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 混乱属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_conf_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_conf(player_ptr)) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 混沌属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_chaos_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_chaos(player_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 劣化属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_disenchant_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_disen(player_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 因果混乱属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nexus_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_disen(player_ptr)) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief ロケット属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_rocket_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_shard(player_ptr)) {
        per /= 2;
    }

    return per;
}

/*!
 * @brief 地獄属性攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_nether_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (has_resist_neth(player_ptr)) {
        if (!PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
            per *= 6;
        }
        per *= 100;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 時間逆転攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_time_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_time(player_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 水流攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_water_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_water(player_ptr)) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

/*!
 * @brief 聖なる火炎攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_holy_fire_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (player_ptr->alignment > 10) {
        per /= 2;
    } else if (player_ptr->alignment < -10) {
        per *= 2;
    }
    return per;
}

/*!
 * @brief 地獄の火炎攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_hell_fire_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (player_ptr->alignment > 10) {
        per *= 2;
    }
    return per;
}

/*!
 * @brief 重力攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_gravity_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (has_levitation(player_ptr)) {
        per = (per * 2) / 3;
    }
    return per;
}

/*!
 * @brief 虚無攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_void_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;
    if (has_pass_wall(player_ptr)) {
        per = per * 3 / 2;
    } else if (has_anti_tele(player_ptr) != 0) {
        per *= 400;
        per /= randrate(4, 7, mode);
    } else if (has_levitation(player_ptr) != 0) {
        per = (per * 2) / 3;
    }
    return per;
}

/*!
 * @brief 深淵攻撃に対するダメージ倍率計算
 */
PERCENTAGE calc_abyss_damage_rate(PlayerType *player_ptr, rate_calc_type_mode mode)
{
    (void)mode; // unused
    PERCENTAGE per = 100;

    if (has_resist_dark(player_ptr) != 0) {
        per *= 400;
        per /= randrate(4, 7, mode);
    } else if ((has_levitation(player_ptr) == 0) && (has_anti_tele(player_ptr) != 0)) {
        per = (per * 5) / 4;
    }
    return per;
}
