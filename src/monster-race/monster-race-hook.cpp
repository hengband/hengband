#include "monster-race/monster-race-hook.h"
#include "dungeon/quest.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player/player-status.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <set>

/*! 通常pit生成時のモンスターの構成条件ID / Race index for "monster pit (clone)" */
MonsterRaceId vault_aux_race;

/*! 単一シンボルpit生成時の指定シンボル / Race index for "monster pit (symbol clone)" */
char vault_aux_char;

/*! ブレス属性に基づくドラゴンpit生成時条件マスク / Breath mask for "monster pit (dragon)" */
EnumClassFlagGroup<MonsterAbilityType> vault_aux_dragon_mask4;

/*!
 * @brief pit/nestの基準となる単種モンスターを決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_clone(PlayerType *player_ptr)
{
    get_mon_num_prep(player_ptr, vault_aux_simple, nullptr);
    vault_aux_race = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
    get_mon_num_prep(player_ptr, nullptr, nullptr);
}

/*!
 * @brief pit/nestの基準となるモンスターシンボルを決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_symbol(PlayerType *player_ptr)
{
    get_mon_num_prep(player_ptr, vault_aux_simple, nullptr);
    MonsterRaceId r_idx = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
    get_mon_num_prep(player_ptr, nullptr, nullptr);
    vault_aux_char = monraces_info[r_idx].d_char;
}

/*!
 * @brief pit/nestの基準となるドラゴンの種類を決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_dragon(PlayerType *player_ptr)
{
    /* Unused */
    (void)player_ptr;

    vault_aux_dragon_mask4.clear();

    constexpr static auto breath_list = {
        MonsterAbilityType::BR_ACID, /* Black */
        MonsterAbilityType::BR_ELEC, /* Blue */
        MonsterAbilityType::BR_FIRE, /* Red */
        MonsterAbilityType::BR_COLD, /* White */
        MonsterAbilityType::BR_POIS, /* Green */
    };

    if (one_in_(6)) {
        /* Multi-hued */
        vault_aux_dragon_mask4.set(breath_list);
        return;
    }

    vault_aux_dragon_mask4.set(rand_choice(breath_list));
}

/*!
 * @brief モンスターがクエストの討伐対象に成り得るかを返す / Hook function for quest monsters
 * @param r_idx モンスターＩＤ
 * @return 討伐対象にできるならTRUEを返す。
 */
bool mon_hook_quest(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_ONLY)) {
        return false;
    }

    if (r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC)) {
        return false;
    }

    if (any_bits(r_ptr->flags2, RF2_MULTIPLY)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがダンジョンに出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return ダンジョンに出現するならばTRUEを返す
 * @details
 * <pre>
 * 地上は常にTRUE(荒野の出現は別hookで絞るため)。
 * 荒野限定(WILD_ONLY)の場合、荒野の山に出るモンスターにのみダンジョンの山に出現を許可する。
 * その他の場合、山及び火山以外のダンジョンでは全てのモンスターに出現を許可する。
 * ダンジョンが山の場合は、荒野の山(WILD_MOUNTAIN)に出ない水棲動物(AQUATIC)は許可しない。
 * ダンジョンが火山の場合は、荒野の火山(WILD_VOLCANO)に出ない水棲動物(AQUATIC)は許可しない。
 * </pre>
 */
bool mon_hook_dungeon(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.is_in_dungeon() && !floor.is_in_quest()) {
        return true;
    }

    auto *r_ptr = &monraces_info[r_idx];
    dungeon_type *d_ptr = &floor.get_dungeon_definition();
    if (r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_ONLY)) {
        return d_ptr->mon_wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN) && r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
    }

    auto land = r_ptr->feature_flags.has_not(MonsterFeatureType::AQUATIC);
    auto is_mountain_monster = d_ptr->mon_wilderness_flags.has_none_of({ MonsterWildernessType::WILD_MOUNTAIN, MonsterWildernessType::WILD_VOLCANO });
    is_mountain_monster |= d_ptr->mon_wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN) && (land || r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN));
    is_mountain_monster |= d_ptr->mon_wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO) && (land || r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO));
    return is_mountain_monster;
}

/*!
 * @brief モンスターが海洋に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 海洋に出現するならばTRUEを返す
 */
bool mon_hook_ocean(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_OCEAN);
}

/*!
 * @brief モンスターが海岸に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 海岸に出現するならばTRUEを返す
 */
bool mon_hook_shore(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_SHORE);
}

/*!
 * @brief モンスターが荒地に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 荒地に出現するならばTRUEを返す
 */
bool mon_hook_waste(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_WASTE, MonsterWildernessType::WILD_ALL });
}

/*!
 * @brief モンスターが町に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 荒地に出現するならばTRUEを返す
 */
bool mon_hook_town(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
}

/*!
 * @brief モンスターが森林に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 森林に出現するならばTRUEを返す
 */
bool mon_hook_wood(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_WOOD, MonsterWildernessType::WILD_ALL });
}

/*!
 * @brief モンスターが火山に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 火山に出現するならばTRUEを返す
 */
bool mon_hook_volcano(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO);
}

/*!
 * @brief モンスターが山地に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 山地に出現するならばTRUEを返す
 */
bool mon_hook_mountain(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
}

/*!
 * @brief モンスターが草原に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 森林に出現するならばTRUEを返す
 */
bool mon_hook_grass(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    return r_ptr->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_GRASS, MonsterWildernessType::WILD_ALL });
}

/*!
 * @brief モンスターが深い水地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 深い水地形に出現するならばTRUEを返す
 */
bool mon_hook_deep_water(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx)) {
        return false;
    }

    return r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC);
}

/*!
 * @brief モンスターが浅い水地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 浅い水地形に出現するならばTRUEを返す
 */
bool mon_hook_shallow_water(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx)) {
        return false;
    }

    return r_ptr->aura_flags.has_not(MonsterAuraType::FIRE);
}

/*!
 * @brief モンスターが溶岩地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 溶岩地形に出現するならばTRUEを返す
 */
bool mon_hook_lava(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx)) {
        return false;
    }

    return (r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK) || r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY)) && r_ptr->aura_flags.has_not(MonsterAuraType::COLD);
}

/*!
 * @brief モンスターが通常の床地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 通常の床地形に出現するならばTRUEを返す
 */
bool mon_hook_floor(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->feature_flags.has_not(MonsterFeatureType::AQUATIC) || r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        return true;
    } else {
        return false;
    }
}

/*
 * Helper function for "glass room"
 */
bool vault_aux_lite(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->ability_flags.has_none_of({ MonsterAbilityType::BR_LITE, MonsterAbilityType::BA_LITE })) {
        return false;
    }

    if (r_ptr->feature_flags.has_any_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL })) {
        return false;
    }

    if (r_ptr->ability_flags.has(MonsterAbilityType::BR_DISI)) {
        return false;
    }

    return true;
}

/*
 * Helper function for "glass room"
 */
bool vault_aux_shards(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->ability_flags.has_not(MonsterAbilityType::BR_SHAR)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがVault生成の最低必要条件を満たしているかを返す /
 * Helper monster selection function
 * @param r_idx 確認したいモンスター種族ID
 * @return Vault生成の最低必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_simple(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    return vault_monster_okay(player_ptr, r_idx);
}

/*!
 * @brief モンスターがゼリーnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (jelly)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_jelly(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY) && r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if (!angband_strchr("ijm,", r_ptr->d_char)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが動物nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (animal)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_animal(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::ANIMAL)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがアンデッドnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (undead)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_undead(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが聖堂nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (chapel)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_chapel_g(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    static const std::set<MonsterRaceId> chapel_list = {
        MonsterRaceId::NOV_PRIEST,
        MonsterRaceId::NOV_PALADIN,
        MonsterRaceId::NOV_PRIEST_G,
        MonsterRaceId::NOV_PALADIN_G,
        MonsterRaceId::PRIEST,
        MonsterRaceId::JADE_MONK,
        MonsterRaceId::IVORY_MONK,
        MonsterRaceId::ULTRA_PALADIN,
        MonsterRaceId::EBONY_MONK,
        MonsterRaceId::W_KNIGHT,
        MonsterRaceId::KNI_TEMPLAR,
        MonsterRaceId::PALADIN,
        MonsterRaceId::TOPAZ_MONK,
    };

    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if ((r_idx == MonsterRaceId::A_GOLD) || (r_idx == MonsterRaceId::A_SILVER)) {
        return false;
    }

    if (r_ptr->d_char == 'A') {
        return true;
    }

    return chapel_list.find(r_idx) != chapel_list.end();
}

/*!
 * @brief モンスターが犬小屋nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (kennel)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_kennel(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (!angband_strchr("CZ", r_ptr->d_char)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがミミックnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (mimic)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_mimic(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (!angband_strchr("!$&(/=?[\\|][`~>+", r_ptr->d_char)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが単一クローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_clone(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    return r_idx == vault_aux_race;
}

/*!
 * @brief モンスターが邪悪属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_e(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY) && r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        return false;
    }

    if (r_ptr->d_char != vault_aux_char) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが善良属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_g(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY) && r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if (r_ptr->d_char != vault_aux_char) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがオークpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (orc)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_orc(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::ORC)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがトロルpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (troll)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_troll(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::TROLL)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが巨人pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (giant)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_giant(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::GIANT)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがドラゴンpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (dragon)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_dragon(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::DRAGON)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        return false;
    }

    auto flags = RF_ABILITY_BREATH_MASK;
    flags.reset(vault_aux_dragon_mask4);

    if (r_ptr->ability_flags.has_any_of(flags) || !r_ptr->ability_flags.has_all_of(vault_aux_dragon_mask4)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが悪魔pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (demon)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_demon(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY) && r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::DEMON)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが狂気pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (lovecraftian)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_cthulhu(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY) && r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (!(r_ptr->flags2 & (RF2_ELDRITCH_HORROR))) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがダークエルフpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (dark elf)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_dark_elf(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    static const std::set<MonsterRaceId> dark_elf_list = {
        MonsterRaceId::D_ELF,
        MonsterRaceId::D_ELF_MAGE,
        MonsterRaceId::D_ELF_WARRIOR,
        MonsterRaceId::D_ELF_PRIEST,
        MonsterRaceId::D_ELF_LORD,
        MonsterRaceId::D_ELF_WARLOCK,
        MonsterRaceId::D_ELF_DRUID,
        MonsterRaceId::NIGHTBLADE,
        MonsterRaceId::D_ELF_SORC,
        MonsterRaceId::D_ELF_SHADE,
    };

    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    return dark_elf_list.find(r_idx) != dark_elf_list.end();
}

/*!
 * @brief バルログが死体を食べられるモンスターかの判定 / Hook function for human corpses
 * @param r_idx モンスターＩＤ
 * @return 死体を食べられるならTRUEを返す。
 */
bool monster_hook_human(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (angband_strchr("pht", r_ptr->d_char)) {
        return true;
    }

    return false;
}

/*!
 * @brief 悪夢の元凶となるモンスターかどうかを返す。
 * @param r_idx 判定対象となるモンスターのＩＤ
 * @return 悪夢の元凶となり得るか否か。
 */
bool get_nightmare(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (none_bits(r_ptr->flags2, RF2_ELDRITCH_HORROR)) {
        return false;
    }

    if (r_ptr->level <= player_ptr->lev) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスター種族が釣れる種族かどうかを判定する。
 * @param r_idx 判定したいモンスター種族のID
 * @return 釣れる対象ならばTRUEを返す
 */
bool monster_is_fishing_target(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC) && r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE) && angband_strchr("Jjlw", r_ptr->d_char)) {
        return true;
    } else {
        return false;
    }
}

/*!
 * @brief モンスター闘技場に参加できるモンスターの判定
 * @param r_idx モンスターＩＤ
 * @details 基準はNEVER_MOVE MULTIPLY QUANTUM AQUATIC RF7_CHAMELEONのいずれも持たず、
 * 自爆以外のなんらかのHP攻撃手段を持っていること。
 * @return 参加できるか否か
 */
bool monster_can_entry_arena(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    int dam = 0;
    const auto &monrace = monraces_info[r_idx];
    bool unselectable = monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE);
    unselectable |= any_bits(monrace.flags2, RF2_MULTIPLY);
    unselectable |= monrace.kind_flags.has(MonsterKindType::QUANTUM) && monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
    unselectable |= monrace.feature_flags.has(MonsterFeatureType::AQUATIC);
    unselectable |= any_bits(monrace.flags7, RF7_CHAMELEON);
    unselectable |= monrace.is_explodable();
    if (unselectable) {
        return false;
    }

    for (const auto &blow : monrace.blows) {
        if (blow.effect != RaceBlowEffectType::DR_MANA) {
            dam += blow.d_dice;
        }
    }

    if (!dam && monrace.ability_flags.has_none_of(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK | RF_ABILITY_BREATH_MASK)) {
        return false;
    }

    return true;
}

/*!
 * モンスターが人形のベースにできるかを返す
 * @param r_idx チェックしたいモンスター種族のID
 * @return 人形にできるならTRUEを返す
 */
bool item_monster_okay(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (any_bits(r_ptr->flags7, RF7_KAGE)) {
        return false;
    }

    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        return false;
    }

    if (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return false;
    }

    if (any_bits(r_ptr->flags1, RF1_FORCE_DEPTH)) {
        return false;
    }

    if (any_bits(r_ptr->flags7, RF7_UNIQUE2)) {
        return false;
    }

    return true;
}

/*!
 * vaultに配置可能なモンスターの条件を指定する / Monster validation
 * @param r_idx モンスター種別ID
 * @param Vaultに配置可能であればTRUE
 * @details
 * Line 1 -- forbid town monsters
 * Line 2 -- forbid uniques
 * Line 3 -- forbid aquatic monsters
 */
bool vault_monster_okay(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    const auto &monrace = monraces_info[r_idx];
    auto is_valid = mon_hook_dungeon(player_ptr, r_idx);
    is_valid &= monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
    is_valid &= none_bits(monrace.flags7, RF7_UNIQUE2);
    is_valid &= monrace.resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
    is_valid &= monrace.feature_flags.has_not(MonsterFeatureType::AQUATIC);
    return is_valid;
}
