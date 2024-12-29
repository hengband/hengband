#include "monster-race/monster-race-hook.h"
#include "dungeon/quest.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player/player-status.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/services/dungeon-monrace-service.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <set>

/*! 通常pit生成時のモンスターの構成条件ID / Race index for "monster pit (clone)" */
MonraceId vault_aux_race;

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
    get_mon_num_prep_enum(player_ptr, MonraceHook::VAULT);
    vault_aux_race = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
    get_mon_num_prep_enum(player_ptr);
}

/*!
 * @brief pit/nestの基準となるモンスターシンボルを決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_symbol(PlayerType *player_ptr)
{
    get_mon_num_prep_enum(player_ptr, MonraceHook::VAULT);
    MonraceId r_idx = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
    get_mon_num_prep_enum(player_ptr);
    vault_aux_char = monraces_info[r_idx].symbol_definition.character;
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

/*
 * Helper function for "glass room"
 */
bool vault_aux_shards(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.ability_flags.has_not(MonsterAbilityType::BR_SHAR)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがゼリーnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (jelly)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_jelly(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    return monrace.symbol_char_is_any_of("ijm,");
}

/*!
 * @brief モンスターが動物nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (animal)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_animal(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::ANIMAL)) {
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
bool vault_aux_undead(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNDEAD)) {
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
bool vault_aux_chapel_g(PlayerType *player_ptr, MonraceId r_idx)
{
    static const std::set<MonraceId> chapel_list = {
        MonraceId::NOV_PRIEST,
        MonraceId::NOV_PALADIN,
        MonraceId::NOV_PRIEST_G,
        MonraceId::NOV_PALADIN_G,
        MonraceId::PRIEST,
        MonraceId::JADE_MONK,
        MonraceId::IVORY_MONK,
        MonraceId::ULTRA_PALADIN,
        MonraceId::EBONY_MONK,
        MonraceId::W_KNIGHT,
        MonraceId::KNI_TEMPLAR,
        MonraceId::PALADIN,
        MonraceId::TOPAZ_MONK,
    };

    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if ((r_idx == MonraceId::A_GOLD) || (r_idx == MonraceId::A_SILVER)) {
        return false;
    }

    if (monrace.symbol_char_is_any_of("A")) {
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
bool vault_aux_kennel(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    return monrace.symbol_char_is_any_of("CZ");
}

/*!
 * @brief モンスターがミミックnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (mimic)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_mimic(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    return monrace.symbol_char_is_any_of("!$&(/=?[\\|][`~>+");
}

/*!
 * @brief モンスターが単一クローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_clone(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
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
bool vault_aux_symbol_e(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        return false;
    }

    if (monrace.symbol_definition.character != vault_aux_char) {
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
bool vault_aux_symbol_g(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if (monrace.symbol_definition.character != vault_aux_char) {
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
bool vault_aux_orc(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::ORC)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
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
bool vault_aux_troll(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::TROLL)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
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
bool vault_aux_giant(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::GIANT)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
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
bool vault_aux_dragon(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::DRAGON)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
        return false;
    }

    auto flags = RF_ABILITY_BREATH_MASK;
    flags.reset(vault_aux_dragon_mask4);

    if (monrace.ability_flags.has_any_of(flags) || !monrace.ability_flags.has_all_of(vault_aux_dragon_mask4)) {
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
bool vault_aux_demon(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::DEMON)) {
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
bool vault_aux_cthulhu(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.misc_flags.has_not(MonsterMiscType::ELDRITCH_HORROR)) {
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
bool vault_aux_dark_elf(PlayerType *player_ptr, MonraceId r_idx)
{
    static const std::set<MonraceId> dark_elf_list = {
        MonraceId::D_ELF,
        MonraceId::D_ELF_MAGE,
        MonraceId::D_ELF_WARRIOR,
        MonraceId::D_ELF_PRIEST,
        MonraceId::D_ELF_LORD,
        MonraceId::D_ELF_WARLOCK,
        MonraceId::D_ELF_DRUID,
        MonraceId::NIGHTBLADE,
        MonraceId::D_ELF_SORC,
        MonraceId::D_ELF_SHADE,
    };

    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    return dark_elf_list.find(r_idx) != dark_elf_list.end();
}

/*!
 * @brief モンスター種族が釣れる種族かどうかを判定する。
 * @param r_idx 判定したいモンスター種族のID
 * @return 釣れる対象ならばTRUEを返す
 */
bool monster_is_fishing_target(PlayerType *player_ptr, MonraceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    const auto &monrace = monraces_info[r_idx];
    auto can_fish = monrace.feature_flags.has(MonsterFeatureType::AQUATIC);
    can_fish &= monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
    can_fish &= angband_strchr("Jjlw", monrace.symbol_definition.character) != nullptr;
    return can_fish;
}
