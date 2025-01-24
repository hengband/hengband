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

PitNestFilter PitNestFilter::instance{};

PitNestFilter &PitNestFilter::get_instance()
{
    return instance;
}

/*!
 * @brief pit/nestの基準となる単種モンスターを決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_clone(PlayerType *player_ptr)
{
    get_mon_num_prep_enum(player_ptr, MonraceHook::VAULT);
    PitNestFilter::get_instance().vault_aux_race = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
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
    PitNestFilter::get_instance().vault_aux_char = monraces_info[r_idx].symbol_definition.character;
}

/*!
 * @brief pit/nestの基準となるドラゴンの種類を決める /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void vault_prep_dragon(PlayerType *player_ptr)
{
    /* Unused */
    (void)player_ptr;

    auto &pit_filer = PitNestFilter::get_instance();
    pit_filer.vault_aux_dragon_mask4.clear();

    constexpr static auto breath_list = {
        MonsterAbilityType::BR_ACID, /* Black */
        MonsterAbilityType::BR_ELEC, /* Blue */
        MonsterAbilityType::BR_FIRE, /* Red */
        MonsterAbilityType::BR_COLD, /* White */
        MonsterAbilityType::BR_POIS, /* Green */
    };

    if (one_in_(6)) {
        /* Multi-hued */
        pit_filer.vault_aux_dragon_mask4.set(breath_list);
        return;
    }

    pit_filer.vault_aux_dragon_mask4.set(rand_choice(breath_list));
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

    return r_idx == PitNestFilter::get_instance().vault_aux_race;
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

    if (monrace.symbol_definition.character != PitNestFilter::get_instance().vault_aux_char) {
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

    if (monrace.symbol_definition.character != PitNestFilter::get_instance().vault_aux_char) {
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
    const auto &dragon_mask = PitNestFilter::get_instance().vault_aux_dragon_mask4;
    flags.reset(dragon_mask);
    if (monrace.ability_flags.has_any_of(flags) || !monrace.ability_flags.has_all_of(dragon_mask)) {
        return false;
    }

    return true;
}
