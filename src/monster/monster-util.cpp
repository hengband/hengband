#include "monster/monster-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/wild.h"
#include "game-option/cheat-options.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-info.h"
#include "spell/summon-types.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-allocation.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "util/bit-flags-calculator.h"
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
static bool restrict_monster_to_dungeon(const DungeonDefinition &dungeon, int floor_level, MonraceId monrace_id, bool has_summon_specific_type, bool is_chameleon_polymorph)
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
        result &= std::all_of(dungeon.r_chars.begin(), dungeon.r_chars.end(), [monrace](const auto &v) { return v == monrace.symbol_definition.character; });
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
        result |= std::any_of(dungeon.r_chars.begin(), dungeon.r_chars.end(), [monrace](const auto &v) { return v == monrace.symbol_definition.character; });
        return dungeon.mode == DungeonMode::OR ? result : !result;
    }
    }

    return true;
}

/*!
 * @brief プレイヤーの現在の広域マップ座標から得た地勢を元にモンスターの生成条件関数を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_underground()) {
        return mon_hook_dungeon;
    }

    switch (wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].terrain) {
    case TERRAIN_TOWN:
        return mon_hook_town;
    case TERRAIN_DEEP_WATER:
        return mon_hook_ocean;
    case TERRAIN_SHALLOW_WATER:
    case TERRAIN_SWAMP:
        return mon_hook_shore;
    case TERRAIN_DIRT:
    case TERRAIN_DESERT:
        return mon_hook_waste;
    case TERRAIN_GRASS:
        return mon_hook_grass;
    case TERRAIN_TREES:
        return mon_hook_wood;
    case TERRAIN_SHALLOW_LAVA:
    case TERRAIN_DEEP_LAVA:
        return mon_hook_volcano;
    case TERRAIN_MOUNTAIN:
        return mon_hook_mountain;
    default:
        return mon_hook_dungeon;
    }
}

/*!
 * @brief 指定された広域マップ座標の地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
MonraceHookTerrain get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const Pos2D pos(y, x);
    const auto &terrain = player_ptr->current_floor_ptr->get_grid(pos).get_terrain();
    if (terrain.flags.has(TerrainCharacteristics::WATER)) {
        return terrain.flags.has(TerrainCharacteristics::DEEP) ? MonraceHookTerrain::DEEP_WATER : MonraceHookTerrain::SHALLOW_WATER;
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        return MonraceHookTerrain::LAVA;
    }

    return MonraceHookTerrain::FLOOR;
}

/*!
 * @brief 開門トラップに配置するモンスターの条件フィルタ
 * @details 穴を掘るモンスター、壁を抜けるモンスターは却下
 */
static bool vault_aux_trapped_pit(PlayerType *player_ptr, MonraceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    if (r_ptr->feature_flags.has_any_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL })) {
        return false;
    }

    return true;
}

static bool filter_monrace_hook2(PlayerType *player_ptr, MonraceId monrace_id, MonraceHookTerrain hook)
{
    switch (hook) {
    case MonraceHookTerrain::NONE:
        return true;
    case MonraceHookTerrain::FLOOR:
        return mon_hook_floor(player_ptr, monrace_id);
    case MonraceHookTerrain::SHALLOW_WATER:
        return mon_hook_shallow_water(player_ptr, monrace_id);
    case MonraceHookTerrain::DEEP_WATER:
        return mon_hook_deep_water(player_ptr, monrace_id);
    case MonraceHookTerrain::TRAPPED_PIT:
        return vault_aux_trapped_pit(player_ptr, monrace_id);
    case MonraceHookTerrain::LAVA:
        return mon_hook_lava(player_ptr, monrace_id);
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid monrace hook type is specified! %d", enum2i(hook)));
    }
}

/*!
 * @brief モンスター生成テーブルの重みを指定条件に従って変更する。
 * @param player_ptr
 * @param hook1 生成制約関数1 (nullptr の場合、制約なし)
 * @param hook2 生成制約関数2 (nullptr の場合、制約なし)
 * @param restrict_to_dungeon 現在プレイヤーのいるダンジョンの制約を適用するか
 * @param summon_specific_type summon_specific によるものの場合、召喚種別を指定する
 * @param is_chameleon_polymorph カメレオンの変身の場合、true
 *
 * モンスター生成テーブル alloc_race_table の各要素の基本重み prob1 を指定条件
 * に従って変更し、結果を prob2 に書き込む。
 */
static void do_get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type &hook1, MonraceHookTerrain hook2, bool restrict_to_dungeon, std::optional<summon_type> summon_specific_type, bool is_chameleon_polymorph)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;

    // モンスター生成テーブルの各要素について重みを修正する。
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;

        // 生成を禁止する要素は重み 0 とする。
        entry.prob2 = 0;

        // 基本重みが 0 以下なら生成禁止。
        // テーブル内の無効エントリもこれに該当する(alloc_race_table は生成時にゼロクリアされるため)。
        if (entry.prob1 <= 0) {
            continue;
        }

        // 生成制約関数が偽を返したら生成禁止。
        if (hook1 && !hook1(player_ptr, monrace_id)) {
            continue;
        }

        if (!filter_monrace_hook2(player_ptr, monrace_id, hook2)) {
            continue;
        }

        // 原則生成禁止するものたち(フェイズアウト状態 / カメレオンの変身先 / ダンジョンの主召喚 は例外)。
        if (!system.is_phase_out() && !is_chameleon_polymorph && summon_specific_type != SUMMON_GUARDIANS) {
            if (!entry.is_permitted(dungeon_level)) {
                continue;
            }

            // 殲滅系クエストの詰み防止 (クエスト内では特殊なフラグ持ちの生成を禁止する)
            if (floor.is_in_quest() && !entry.is_defeatable(dungeon_level)) {
                continue;
            }
        }

        // 生成を許可するものは基本重みをそのまま引き継ぐ。
        entry.prob2 = entry.prob1;

        // 引数で指定されていればさらにダンジョンによる制約を試みる。
        if (restrict_to_dungeon) {
            // ダンジョンによる制約を適用する条件:
            //
            //   * フェイズアウト状態でない
            //   * 1階かそれより深いところにいる
            //   * ランダムクエスト中でない
            const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
            const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
            if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id, summon_specific_type.has_value(), is_chameleon_polymorph)) {
                // ダンジョンによる制約に掛かった場合、重みを special_div/64 倍する。
                // 丸めは確率的に行う。
                const int numer = entry.prob2 * dungeon.special_div;
                const int q = numer / 64;
                const int r = numer % 64;
                entry.prob2 = static_cast<short>(randint0(64) < r ? q + 1 : q);
            }
        }

        mfdi.update(entry.prob2, entry.level);
    }

    if (cheat_hear) {
        msg_print(mfdi.to_string());
    }
}

/*!
 * @brief モンスター生成テーブルの重み修正
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param hook1 生成制約関数1 (nullptr の場合、制約なし)
 * @param hook2 生成制約関数2 (nullptr の場合、制約なし)
 * @param summon_specific_type summon_specific によるものの場合、召喚種別を指定する
 * @return 常に 0
 *
 * get_mon_num() を呼ぶ前に get_mon_num_prep() 系関数のいずれかを呼ぶこと。
 */
void get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type &hook1, MonraceHookTerrain hook2, std::optional<summon_type> summon_specific_type)
{
    do_get_mon_num_prep(player_ptr, hook1, hook2, true, summon_specific_type, false);
}

/*!
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(PlayerType *player_ptr, MonraceId r_idx, short m_idx, short terrain_id, std::optional<short> summoner_m_idx)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(r_idx);
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

    if (!monster_can_cross_terrain(player_ptr, terrain_id, &monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        return !monster_has_hostile_align(player_ptr, &monster, 0, 0, &monrace);
    }

    return !summoner_m_idx || !monster_has_hostile_align(player_ptr, &floor.m_list[*summoner_m_idx], 0, 0, &monrace);
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(PlayerType *player_ptr, MonraceId r_idx, short m_idx, short terrain_id, std::optional<short> summoner_m_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
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

    if (!monster_can_cross_terrain(player_ptr, terrain_id, &monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        if (old_monrace.kind_flags.has(MonsterKindType::GOOD) && monrace.kind_flags.has_not(MonsterKindType::GOOD)) {
            return false;
        }

        if (old_monrace.kind_flags.has(MonsterKindType::EVIL) && monrace.kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }

        if (old_monrace.kind_flags.has_none_of(alignment_mask)) {
            return false;
        }
    } else if (summoner_m_idx && monster_has_hostile_align(player_ptr, &floor.m_list[*summoner_m_idx], 0, 0, &monrace)) {
        return false;
    }

    const auto hook_pf = get_monster_hook(player_ptr);
    return hook_pf(player_ptr, r_idx);
}

/*!
 * @brief モンスター生成テーブルの重み修正(カメレオン変身専用)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx カメレオンのフロア内インデックス
 * @param terrain_id カメレオンの足元にある地形のID
 * @param summoner_m_idx 召喚者のモンスターインデックス
 * @param is_unique ユニークであるか否か (実質、カメレオンの王であるか否か)
 * @details get_mon_num() を呼ぶ前に get_mon_num_prep 系関数のいずれかを呼ぶこと。
 */
void get_mon_num_prep_chameleon(PlayerType *player_ptr, short m_idx, short terrain_id, const std::optional<short> summoner_m_idx, bool is_unique)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto dungeon_level = floor.dun_level;
    const auto &system = AngbandSystem::get_instance();
    auto &table = MonraceAllocationTable::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    const auto hook_func = is_unique ? monster_hook_chameleon_lord : monster_hook_chameleon;
    MonraceFilterDebugInfo mfdi;
    for (auto &entry : table) {
        const auto monrace_id = entry.index;
        entry.prob2 = 0;
        if (entry.prob1 <= 0) {
            continue;
        }

        if (!hook_func(player_ptr, monrace_id, m_idx, terrain_id, summoner_m_idx)) {
            continue;
        }

        entry.prob2 = entry.prob1;
        const auto in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
        const auto cond = !system.is_phase_out() && floor.is_underground() && !in_random_quest;
        if (cond && !restrict_monster_to_dungeon(dungeon, dungeon_level, monrace_id, false, true)) {
            const int numer = entry.prob2 * dungeon.special_div;
            const int q = numer / 64;
            const int r = numer % 64;
            entry.prob2 = static_cast<short>(randint0(64) < r ? q + 1 : q);
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
    do_get_mon_num_prep(player_ptr, nullptr, MonraceHookTerrain::NONE, false, std::nullopt, false);
}

bool is_player(MONSTER_IDX m_idx)
{
    return m_idx == 0;
}

bool is_monster(MONSTER_IDX m_idx)
{
    return m_idx > 0;
}
