#include "monster/monster-util.h"
#include "dungeon/quest.h"
#include "game-option/cheat-options.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-ability-mask.h"
#include "monster/monster-info.h"
#include "mspell/summon-checker.h"
#include "room/pit-nest-util.h"
#include "spell/summon-types.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-allocation.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/services/dungeon-monrace-service.h"
#include "system/terrain/terrain-definition.h"
#include "view/display-messages.h"
#include "wizard/monrace-filter-debug-info.h"
#include <algorithm>
#include <iterator>

/**
 * @brief モンスターがダンジョンに出現できる条件を満たしているかのフラグ判定関数(AND)
 *
 * @param r_flags モンスター側のフラグ
 * @param d_flags ダンジョン側の判定フラグ
 * @return 出現可能かどうか
 */
template <class T>
static bool is_possible_monster_and(const EnumClassFlagGroup<T> &r_flags, const EnumClassFlagGroup<T> &d_flags)
{
    return r_flags.has_all_of(d_flags);
}

/**
 * @brief モンスターがダンジョンに出現できる条件を満たしているかのフラグ判定関数(OR)
 *
 * @param r_flags モンスター側のフラグ
 * @param d_flags ダンジョン側の判定フラグ
 * @return 出現可能かどうか
 */
template <class T>
static bool is_possible_monster_or(const EnumClassFlagGroup<T> &r_flags, const EnumClassFlagGroup<T> &d_flags)
{
    return r_flags.has_any_of(d_flags);
}

/*!
 * @brief 指定されたモンスター種族がダンジョンの制限にかかるかどうかをチェックする
 * @param dungeon ダンジョンへの参照
 * @param floor_level 生成階層
 * @param monrace_id チェックするモンスター種族ID
 * @param summon_specific_type summon_specific() によるものの場合、召喚種別を指定する
 * @param is_chameleon_polymorph カメレオンの変身の場合、true
 * @return 召喚条件が一致するならtrue / Return TRUE is the monster is OK and FALSE otherwise
 */
static bool restrict_monster_to_dungeon(const DungeonDefinition &dungeon, int floor_level, MonraceId monrace_id, bool has_summon_specific_type = false, bool is_chameleon_polymorph = false)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (dungeon.flags.has(DungeonFeatureType::CHAMELEON)) {
        if (is_chameleon_polymorph) {
            return true;
        }
    }

    if (dungeon.flags.has(DungeonFeatureType::NO_MAGIC)) {
        if (monrace_id != MonraceId::CHAMELEON && monrace.freq_spell && monrace.ability_flags.has_none_of(RF_ABILITY_NOMAGIC_MASK)) {
            return false;
        }
    }

    if (dungeon.flags.has(DungeonFeatureType::NO_MELEE)) {
        if (monrace_id == MonraceId::CHAMELEON) {
            return true;
        }

        static const EnumClassFlagGroup<MonsterAbilityType> rf_ability_masks(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK);
        static const EnumClassFlagGroup<MonsterAbilityType> abilities = {
            MonsterAbilityType::CAUSE_1,
            MonsterAbilityType::CAUSE_2,
            MonsterAbilityType::CAUSE_3,
            MonsterAbilityType::CAUSE_4,
            MonsterAbilityType::MIND_BLAST,
            MonsterAbilityType::BRAIN_SMASH,
        };
        if (monrace.ability_flags.has_none_of(rf_ability_masks) && monrace.ability_flags.has_none_of(abilities)) {
            return false;
        }
    }

    if (dungeon.flags.has(DungeonFeatureType::BEGINNER)) {
        if (monrace.level > floor_level) {
            return false;
        }
    }

    if (dungeon.special_div >= 64) {
        return true;
    }

    if (has_summon_specific_type && dungeon.flags.has_not(DungeonFeatureType::CHAMELEON)) {
        return true;
    }

    switch (dungeon.mode) {
    case DungeonMode::AND:
    case DungeonMode::NAND: {
        std::vector<bool> is_possible = {
            is_possible_monster_and(monrace.ability_flags, dungeon.mon_ability_flags),
            is_possible_monster_and(monrace.behavior_flags, dungeon.mon_behavior_flags),
            is_possible_monster_and(monrace.resistance_flags, dungeon.mon_resistance_flags),
            is_possible_monster_and(monrace.drop_flags, dungeon.mon_drop_flags),
            is_possible_monster_and(monrace.kind_flags, dungeon.mon_kind_flags),
            is_possible_monster_and(monrace.wilderness_flags, dungeon.mon_wilderness_flags),
            is_possible_monster_and(monrace.feature_flags, dungeon.mon_feature_flags),
            is_possible_monster_and(monrace.population_flags, dungeon.mon_population_flags),
            is_possible_monster_and(monrace.speak_flags, dungeon.mon_speak_flags),
            is_possible_monster_and(monrace.brightness_flags, dungeon.mon_brightness_flags),
            is_possible_monster_and(monrace.misc_flags, dungeon.mon_misc_flags),
        };

        auto result = std::all_of(is_possible.begin(), is_possible.end(), [](const auto &v) { return v; });
        result &= std::all_of(dungeon.r_chars.begin(), dungeon.r_chars.end(), [&monrace](const auto &v) { return v == monrace.symbol_definition.character; });
        return dungeon.mode == DungeonMode::AND ? result : !result;
    }
    case DungeonMode::OR:
    case DungeonMode::NOR: {
        std::vector<bool> is_possible = {
            is_possible_monster_or(monrace.ability_flags, dungeon.mon_ability_flags),
            is_possible_monster_or(monrace.behavior_flags, dungeon.mon_behavior_flags),
            is_possible_monster_or(monrace.resistance_flags, dungeon.mon_resistance_flags),
            is_possible_monster_or(monrace.drop_flags, dungeon.mon_drop_flags),
            is_possible_monster_or(monrace.kind_flags, dungeon.mon_kind_flags),
            is_possible_monster_or(monrace.wilderness_flags, dungeon.mon_wilderness_flags),
            is_possible_monster_or(monrace.feature_flags, dungeon.mon_feature_flags),
            is_possible_monster_or(monrace.population_flags, dungeon.mon_population_flags),
            is_possible_monster_or(monrace.speak_flags, dungeon.mon_speak_flags),
            is_possible_monster_or(monrace.brightness_flags, dungeon.mon_brightness_flags),
            is_possible_monster_or(monrace.misc_flags, dungeon.mon_misc_flags),
        };

        auto result = std::any_of(is_possible.begin(), is_possible.end(), [](const auto &v) { return v; });
        result |= std::any_of(dungeon.r_chars.begin(), dungeon.r_chars.end(), [&monrace](const auto &v) { return v == monrace.symbol_definition.character; });
        return dungeon.mode == DungeonMode::OR ? result : !result;
    }
    }

    return true;
}

static bool do_hook(PlayerType *player_ptr, MonraceHook hook, MonraceId monrace_id)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(monrace_id);
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto is_suitable_for_dungeon = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, monrace_id);
    switch (hook) {
    case MonraceHook::NONE:
    case MonraceHook::DUNGEON:
        return is_suitable_for_dungeon;
    case MonraceHook::TOWN:
        return monrace.is_suitable_for_town();
    case MonraceHook::OCEAN:
        return monrace.is_suitable_for_ocean();
    case MonraceHook::SHORE:
        return monrace.is_suitable_for_shore();
    case MonraceHook::WASTE:
        return monrace.is_suitable_for_waste();
    case MonraceHook::GRASS:
        return monrace.is_suitable_for_grass();
    case MonraceHook::WOOD:
        return monrace.is_suitable_for_wood();
    case MonraceHook::VOLCANO:
        return monrace.is_suitable_for_volcano();
    case MonraceHook::MOUNTAIN:
        return monrace.is_suitable_for_mountain();
    case MonraceHook::FIGURINE:
        return monrace.is_suitable_for_figurine();
    case MonraceHook::ARENA:
        return monrace.can_entry_arena();
    case MonraceHook::NIGHTMARE:
        return monrace.is_suitable_for_nightmare(player_ptr->lev);
    case MonraceHook::HUMAN:
        return monrace.is_eatable_human();
    case MonraceHook::GLASS:
        return is_suitable_for_dungeon && monrace.is_suitable_for_glass_through();
    case MonraceHook::SHARDS:
        return is_suitable_for_dungeon && monrace.is_suitable_for_glass_breaking();
    case MonraceHook::TANUKI: {
        if (!monrace.is_suitable_for_tanuki()) {
            return false;
        }

        const auto hook_tanuki = floor.get_monrace_hook();
        return do_hook(player_ptr, hook_tanuki, monrace_id);
    }
    case MonraceHook::FISHING:
        return monrace.is_catchable_for_fishing();
    case MonraceHook::QUEST:
        return monrace.is_suitable_for_random_quest();
    case MonraceHook::VAULT:
        return is_suitable_for_dungeon && monrace.is_suitable_for_special_room();
    case MonraceHook::CLONE:
        return is_suitable_for_dungeon && monrace.is_suitable_for_special_room() && (monrace_id == PitNestFilter::get_instance().get_monrace_id());
    case MonraceHook::JELLY:
        return is_suitable_for_dungeon && monrace.is_suitable_for_jelly_nest();
    case MonraceHook::GOOD:
        return is_suitable_for_dungeon && monrace.is_suitable_for_good_nest(PitNestFilter::get_instance().get_monrace_symbol());
    case MonraceHook::EVIL:
        return is_suitable_for_dungeon && monrace.is_suitable_for_evil_nest(PitNestFilter::get_instance().get_monrace_symbol());
    case MonraceHook::MIMIC:
        return is_suitable_for_dungeon && monrace.is_suitable_for_mimic_nest();
    case MonraceHook::HORROR:
        return is_suitable_for_dungeon && monrace.is_suitable_for_horror_pit();
    case MonraceHook::KENNEL:
        return is_suitable_for_dungeon && monrace.is_suitable_for_dog_nest();
    case MonraceHook::ANIMAL:
        return is_suitable_for_dungeon && monrace.is_suitable_for_animal_nest();
    case MonraceHook::CHAPEL:
        return is_suitable_for_dungeon && (monraces.is_angel(monrace_id) || MonraceList::is_chapel(monrace_id)) && monrace.is_suitable_for_chapel_nest();
    case MonraceHook::UNDEAD:
        return is_suitable_for_dungeon && monrace.is_suitable_for_undead_nest();
    case MonraceHook::ORC:
        return is_suitable_for_dungeon && monrace.is_suitable_for_orc_pit();
    case MonraceHook::TROLL:
        return is_suitable_for_dungeon && monrace.is_suitable_for_troll_pit();
    case MonraceHook::GIANT:
        return is_suitable_for_dungeon && monrace.is_suitable_for_giant_pit();
    case MonraceHook::DRAGON:
        return is_suitable_for_dungeon && monrace.is_suitable_for_dragon_nest(PitNestFilter::get_instance().get_dragon_breaths());
    case MonraceHook::DEMON:
        return is_suitable_for_dungeon && monrace.is_suitable_for_demon_pit();
    case MonraceHook::DARK_ELF:
        return is_suitable_for_dungeon && MonraceList::is_dark_elf(monrace_id) && monrace.is_suitable_for_special_room();
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid monrace hook type is specified! %d", enum2i(hook)));
    }
}

/*!
 * @brief モンスター生成テーブルの重み修正
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param hook1 生成制約1
 * @param hook2 生成制約2
 * @param summon_specific_type summon_specific によるものの場合、召喚種別を指定する
 * @return 常に 0
 *
 * get_mon_num() を呼ぶ前に get_mon_num_prep() 系関数のいずれかを呼ぶこと。
 */
void get_mon_num_prep_enum(PlayerType *player_ptr, MonraceHook hook1, MonraceHookTerrain hook2)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!do_hook(player_ptr, hook1, monrace_id)) {
            continue;
        }

        if (!floor.filter_monrace_terrain(monrace_id, hook2)) {
            continue;
        }

        if (!system.is_phase_out()) {
            if (!entry.is_permitted(dungeon_level)) {
                continue;
            }

            if (floor.is_in_quest() && !entry.is_defeatable(dungeon_level)) {
                continue;
            }
        }

        entry.prob2 = entry.prob1;
        const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
        const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
        if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id)) {
            entry.update_prob2(dungeon.special_div);
        }

        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief モンスター種族が護衛となれるかどうかをチェックする
 * @param monrace_id チェックするモンスターの種族ID
 * @param escorted_monrace_id 護衛されるモンスターの種族ID
 * @param escorted_m_idx 護衛されるモンスターのフロア内インデックス
 * @return 護衛にできるならばtrue
 */
static bool place_monster_can_escort(PlayerType *player_ptr, MonraceId monrace_id, MonraceId escorted_monrace_id, short escorted_m_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_id = floor.dungeon_id;
    const auto &escorted_monster = floor.m_list[escorted_m_idx];
    const auto &monraces = MonraceList::get_instance();
    const auto &escorted_monrace = monraces.get_monrace(escorted_monrace_id);
    const auto &monrace = monraces.get_monrace(monrace_id);
    const auto is_suitable_escorted_monrace = DungeonMonraceService::is_suitable_for_dungeon(dungeon_id, escorted_monrace_id);
    const auto is_suitable_escorting_monrace = DungeonMonraceService::is_suitable_for_dungeon(dungeon_id, monrace_id);
    if (floor.is_underground() && (is_suitable_escorted_monrace != is_suitable_escorting_monrace)) {
        return false;
    }

    if (monrace.symbol_definition.character != escorted_monrace.symbol_definition.character) {
        return false;
    }

    if (monrace.level > escorted_monrace.level) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (escorted_monrace_id == monrace_id) {
        return false;
    }

    if (monster_has_hostile_to_other_monster(escorted_monster, monrace)) {
        return false;
    }

    if (escorted_monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY)) {
        if (monster_has_hostile_to_player(player_ptr, 1, -1, monrace)) {
            return false;
        }
    }

    if (escorted_monrace.misc_flags.has(MonsterMiscType::CHAMELEON) && monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスター生成テーブルの重み修正
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param escorted_monrace_id 護衛されるモンスターの種族ID
 * @param m_idx 護衛されるモンスターのフロア内インデックス
 * @param hook 生成の地形条件
 */
void get_mon_num_prep_escort(PlayerType *player_ptr, MonraceId escorted_monrace_id, short m_idx, MonraceHookTerrain hook)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!place_monster_can_escort(player_ptr, monrace_id, escorted_monrace_id, m_idx)) {
            continue;
        }

        if (!floor.filter_monrace_terrain(monrace_id, hook)) {
            continue;
        }

        if (!system.is_phase_out()) {
            if (!entry.is_permitted(dungeon_level)) {
                continue;
            }

            if (floor.is_in_quest() && !entry.is_defeatable(dungeon_level)) {
                continue;
            }
        }

        entry.prob2 = entry.prob1;
        const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
        const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
        if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id)) {
            entry.update_prob2(dungeon.special_div);
        }

        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief モンスターが召喚の基本条件に合っているかをチェックする / Hack -- help decide if a monster race is "okay" to summon
 * @param r_idx チェックするモンスター種族ID
 * @param type 召喚種別
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 召喚対象にできるならばTRUE
 */
static bool summon_specific_okay(PlayerType *player_ptr, MonraceId monrace_id, const SummonCondition &condition)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_underground() && !DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, monrace_id)) {
        return false;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (condition.summoner_m_idx) {
        const auto &monster = floor.m_list[*condition.summoner_m_idx];
        if (monster_has_hostile_to_other_monster(monster, monrace)) {
            return false;
        }
    } else if (any_bits(condition.mode, PM_FORCE_PET)) {
        if (monster_has_hostile_to_player(player_ptr, 10, -10, monrace) && !one_in_(std::abs(player_ptr->alignment) / 2 + 1)) {
            return false;
        }
    }

    if (none_bits(condition.mode, PM_ALLOW_UNIQUE) && (monrace.kind_flags.has(MonsterKindType::UNIQUE) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL)))) {
        return false;
    }

    if (condition.type == SUMMON_NONE) {
        return true;
    }

    const auto is_like_unique = monrace.kind_flags.has(MonsterKindType::UNIQUE) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL));
    if (any_bits(condition.mode, PM_FORCE_PET) && is_like_unique && monster_has_hostile_to_player(player_ptr, 10, -10, monrace)) {
        return false;
    }

    if (monrace.misc_flags.has(MonsterMiscType::CHAMELEON) && floor.get_dungeon_definition().flags.has(DungeonFeatureType::CHAMELEON)) {
        return true;
    }

    if (!condition.summoner_m_idx) {
        return check_summon_specific(player_ptr, MonraceId::PLAYER, monrace_id, condition.type);
    }

    const auto &monster = floor.m_list[*condition.summoner_m_idx];
    return check_summon_specific(player_ptr, monster.r_idx, monrace_id, condition.type);
}

/*!
 * @brief モンスター生成テーブルの重み修正 (召喚用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param SummonCondition 生成制約
 */
void get_mon_num_prep_summon(PlayerType *player_ptr, const SummonCondition &condition)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!summon_specific_okay(player_ptr, monrace_id, condition)) {
            continue;
        }

        if (!floor.filter_monrace_terrain(monrace_id, condition.hook)) {
            continue;
        }

        if (!system.is_phase_out() && (condition.type != SUMMON_GUARDIANS)) {
            if (!entry.is_permitted(dungeon_level)) {
                continue;
            }

            if (floor.is_in_quest() && !entry.is_defeatable(dungeon_level)) {
                continue;
            }
        }

        entry.prob2 = entry.prob1;
        const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
        const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
        if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id, true)) {
            entry.update_prob2(dungeon.special_div);
        }

        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ct カメレオンの変身情報
 * @param monrace_id 変身後のモンスター種族ID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(PlayerType *player_ptr, const ChameleonTransformation &ct, MonraceId monrace_id)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(monrace_id);
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY) || monrace.misc_flags.has(MonsterMiscType::CHAMELEON)) {
        return false;
    }

    if (std::abs(monrace.level - monraces.get_monrace(MonraceId::CHAMELEON_K).level) > 5) {
        return false;
    }

    if (monrace.is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, ct.terrain_id, monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[ct.m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        return !monster_has_hostile_to_other_monster(monster, monrace);
    }

    return !ct.summoner_m_idx || !monster_has_hostile_to_other_monster(floor.m_list[*ct.summoner_m_idx], monrace);
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ct カメレオンの変身情報
 * @param monrace_id 変身後のモンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(PlayerType *player_ptr, const ChameleonTransformation &ct, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY) || (monrace.misc_flags.has(MonsterMiscType::CHAMELEON))) {
        return false;
    }

    if (monrace.is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, ct.terrain_id, monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[ct.m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        if (old_monrace.kind_flags.has(MonsterKindType::GOOD) && monrace.kind_flags.has_not(MonsterKindType::GOOD)) {
            return false;
        }

        if (old_monrace.kind_flags.has(MonsterKindType::EVIL) && monrace.kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }

        if (old_monrace.kind_flags.has_none_of(alignment_mask) && monrace.kind_flags.has_any_of(alignment_mask)) {
            return false;
        }
    } else if (ct.summoner_m_idx && monster_has_hostile_to_other_monster(floor.m_list[*ct.summoner_m_idx], monrace)) {
        return false;
    }

    const auto hook = floor.get_monrace_hook();
    return do_hook(player_ptr, hook, monrace_id);
}

/*!
 * @brief モンスター生成テーブルの重み修正(カメレオン変身専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ct カメレオンの変身情報
 * @details get_mon_num() を呼ぶ前に get_mon_num_prep 系関数のいずれかを呼ぶこと。
 */
void get_mon_num_prep_chameleon(PlayerType *player_ptr, const ChameleonTransformation &ct)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    const auto hook_func = ct.is_unique ? monster_hook_chameleon_lord : monster_hook_chameleon;
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!hook_func(player_ptr, ct, monrace_id)) {
            continue;
        }

        entry.prob2 = entry.prob1;
        const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
        const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
        if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id, false, true)) {
            entry.update_prob2(dungeon.special_div);
        }

        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief モンスター生成テーブルの重み修正(賞金首選定用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details get_mon_num() を呼ぶ前に get_mon_num_prep 系関数のいずれかを呼ぶこと。
 */
void get_mon_num_prep_bounty(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!system.is_phase_out()) {
            if (!entry.is_permitted(dungeon_level)) {
                continue;
            }

            if (floor.is_in_quest() && !entry.is_defeatable(dungeon_level)) {
                continue;
            }
        }

        entry.prob2 = entry.prob1;
        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief モンスターにPRESENT_AT_TURN_STARTを設定する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void mark_monsters_present(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;

    for (MONSTER_IDX m_idx = floor.m_max - 1; m_idx >= 1; m_idx--) {
        auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid()) {
            continue;
        }
        monster.mflag.set(MonsterTemporaryFlagType::PRESENT_AT_TURN_START);
    }
}

bool is_player(MONSTER_IDX m_idx)
{
    return m_idx == 0;
}

bool is_monster(MONSTER_IDX m_idx)
{
    return m_idx > 0;
}
