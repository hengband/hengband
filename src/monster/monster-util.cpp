#include "monster/monster-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/wild.h"
#include "game-option/cheat-options.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "spell/summon-types.h"
#include "system/alloc-entries.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <algorithm>
#include <iterator>

enum dungeon_mode_type {
    DUNGEON_MODE_AND = 1,
    DUNGEON_MODE_NAND = 2,
    DUNGEON_MODE_OR = 3,
    DUNGEON_MODE_NOR = 4,
};

MONSTER_IDX hack_m_idx = 0; /* Hack -- see "process_monsters()" */
MONSTER_IDX hack_m_idx_ii = 0;

/*!
 * @var chameleon_change_m_idx
 * @brief カメレオンの変身先モンスターIDを受け渡すためのグローバル変数
 * @todo 変数渡しの問題などもあるができればchameleon_change_m_idxのグローバル変数を除去し、関数引き渡しに移行すること
 */
int chameleon_change_m_idx = 0;

/*!
 * @var summon_specific_type
 * @brief 召喚条件を指定するグローバル変数 / Hack -- the "type" of the current "summon specific"
 * @todo summon_specific_typeグローバル変数の除去と関数引数への代替を行う
 */
summon_type summon_specific_type = SUMMON_NONE;

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
 * @brief 指定されたモンスター種族がダンジョンの制限にかかるかどうかをチェックする / Some dungeon types restrict the possible monsters.
 * @param floor_ptr フロアへの参照ポインタ
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚条件が一致するならtrue / Return TRUE is the monster is OK and FALSE otherwise
 */
static bool restrict_monster_to_dungeon(const FloorType *floor_ptr, MonsterRaceId r_idx)
{
    const auto *d_ptr = &floor_ptr->get_dungeon_definition();
    const auto *r_ptr = &monraces_info[r_idx];
    if (d_ptr->flags.has(DungeonFeatureType::CHAMELEON)) {
        if (chameleon_change_m_idx) {
            return true;
        }
    }

    if (d_ptr->flags.has(DungeonFeatureType::NO_MAGIC)) {
        if (r_idx != MonsterRaceId::CHAMELEON && r_ptr->freq_spell && r_ptr->ability_flags.has_none_of(RF_ABILITY_NOMAGIC_MASK)) {
            return false;
        }
    }

    if (d_ptr->flags.has(DungeonFeatureType::NO_MELEE)) {
        if (r_idx == MonsterRaceId::CHAMELEON) {
            return true;
        }
        if (r_ptr->ability_flags.has_none_of(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK) && r_ptr->ability_flags.has_none_of(
                                                                                                                        { MonsterAbilityType::CAUSE_1, MonsterAbilityType::CAUSE_2, MonsterAbilityType::CAUSE_3, MonsterAbilityType::CAUSE_4, MonsterAbilityType::MIND_BLAST, MonsterAbilityType::BRAIN_SMASH })) {
            return false;
        }
    }

    if (d_ptr->flags.has(DungeonFeatureType::BEGINNER)) {
        if (r_ptr->level > floor_ptr->dun_level) {
            return false;
        }
    }

    if (d_ptr->special_div >= 64) {
        return true;
    }
    if (summon_specific_type && d_ptr->flags.has_not(DungeonFeatureType::CHAMELEON)) {
        return true;
    }

    switch (d_ptr->mode) {
    case DUNGEON_MODE_AND:
    case DUNGEON_MODE_NAND: {
        std::vector<bool> is_possible = {
            all_bits(r_ptr->flags1, d_ptr->mflags1),
            all_bits(r_ptr->flags2, d_ptr->mflags2),
            all_bits(r_ptr->flags3, d_ptr->mflags3),
            all_bits(r_ptr->flags7, d_ptr->mflags7),
            all_bits(r_ptr->flags8, d_ptr->mflags8),
            is_possible_monster_and(r_ptr->ability_flags, d_ptr->mon_ability_flags),
            is_possible_monster_and(r_ptr->behavior_flags, d_ptr->mon_behavior_flags),
            is_possible_monster_and(r_ptr->resistance_flags, d_ptr->mon_resistance_flags),
            is_possible_monster_and(r_ptr->drop_flags, d_ptr->mon_drop_flags),
            is_possible_monster_and(r_ptr->kind_flags, d_ptr->mon_kind_flags),
            is_possible_monster_and(r_ptr->wilderness_flags, d_ptr->mon_wilderness_flags),
            is_possible_monster_and(r_ptr->feature_flags, d_ptr->mon_feature_flags),
            is_possible_monster_and(r_ptr->population_flags, d_ptr->mon_population_flags),
            is_possible_monster_and(r_ptr->speak_flags, d_ptr->mon_speak_flags),
            is_possible_monster_and(r_ptr->brightness_flags, d_ptr->mon_brightness_flags),
        };

        auto result = std::all_of(is_possible.begin(), is_possible.end(), [](const auto &v) { return v; });
        result &= std::all_of(d_ptr->r_chars.begin(), d_ptr->r_chars.end(), [r_ptr](const auto &v) { return v == r_ptr->d_char; });

        return d_ptr->mode == DUNGEON_MODE_AND ? result : !result;
    }
    case DUNGEON_MODE_OR:
    case DUNGEON_MODE_NOR: {
        std::vector<bool> is_possible = {
            any_bits(r_ptr->flags1, d_ptr->mflags1),
            any_bits(r_ptr->flags2, d_ptr->mflags2),
            any_bits(r_ptr->flags3, d_ptr->mflags3),
            any_bits(r_ptr->flags7, d_ptr->mflags7),
            any_bits(r_ptr->flags8, d_ptr->mflags8),
            is_possible_monster_or(r_ptr->ability_flags, d_ptr->mon_ability_flags),
            is_possible_monster_or(r_ptr->behavior_flags, d_ptr->mon_behavior_flags),
            is_possible_monster_or(r_ptr->resistance_flags, d_ptr->mon_resistance_flags),
            is_possible_monster_or(r_ptr->drop_flags, d_ptr->mon_drop_flags),
            is_possible_monster_or(r_ptr->kind_flags, d_ptr->mon_kind_flags),
            is_possible_monster_or(r_ptr->wilderness_flags, d_ptr->mon_wilderness_flags),
            is_possible_monster_or(r_ptr->feature_flags, d_ptr->mon_feature_flags),
            is_possible_monster_or(r_ptr->population_flags, d_ptr->mon_population_flags),
            is_possible_monster_or(r_ptr->speak_flags, d_ptr->mon_speak_flags),
            is_possible_monster_or(r_ptr->brightness_flags, d_ptr->mon_brightness_flags),
        };

        auto result = std::any_of(is_possible.begin(), is_possible.end(), [](const auto &v) { return v; });
        result |= std::any_of(d_ptr->r_chars.begin(), d_ptr->r_chars.end(), [r_ptr](const auto &v) { return v == r_ptr->d_char; });

        return d_ptr->mode == DUNGEON_MODE_OR ? result : !result;
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
    if ((player_ptr->current_floor_ptr->dun_level > 0) || (inside_quest(player_ptr->current_floor_ptr->quest_number))) {
        return (monsterrace_hook_type)mon_hook_dungeon;
    }

    switch (wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].terrain) {
    case TERRAIN_TOWN:
        return (monsterrace_hook_type)mon_hook_town;
    case TERRAIN_DEEP_WATER:
        return (monsterrace_hook_type)mon_hook_ocean;
    case TERRAIN_SHALLOW_WATER:
    case TERRAIN_SWAMP:
        return (monsterrace_hook_type)mon_hook_shore;
    case TERRAIN_DIRT:
    case TERRAIN_DESERT:
        return (monsterrace_hook_type)mon_hook_waste;
    case TERRAIN_GRASS:
        return (monsterrace_hook_type)mon_hook_grass;
    case TERRAIN_TREES:
        return (monsterrace_hook_type)mon_hook_wood;
    case TERRAIN_SHALLOW_LAVA:
    case TERRAIN_DEEP_LAVA:
        return (monsterrace_hook_type)mon_hook_volcano;
    case TERRAIN_MOUNTAIN:
        return (monsterrace_hook_type)mon_hook_mountain;
    default:
        return (monsterrace_hook_type)mon_hook_dungeon;
    }
}

/*!
 * @brief 指定された広域マップ座標の地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook2(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto *f_ptr = &terrains_info[player_ptr->current_floor_ptr->grid_array[y][x].feat];
    if (f_ptr->flags.has(TerrainCharacteristics::WATER)) {
        return f_ptr->flags.has(TerrainCharacteristics::DEEP) ? (monsterrace_hook_type)mon_hook_deep_water : (monsterrace_hook_type)mon_hook_shallow_water;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::LAVA)) {
        return (monsterrace_hook_type)mon_hook_lava;
    }

    return (monsterrace_hook_type)mon_hook_floor;
}

/*!
 * @brief モンスター生成テーブルの重みを指定条件に従って変更する。
 * @param player_ptr
 * @param hook1 生成制約関数1 (nullptr の場合、制約なし)
 * @param hook2 生成制約関数2 (nullptr の場合、制約なし)
 * @param restrict_to_dungeon 現在プレイヤーのいるダンジョンの制約を適用するか
 * @return 常に 0
 *
 * モンスター生成テーブル alloc_race_table の各要素の基本重み prob1 を指定条件
 * に従って変更し、結果を prob2 に書き込む。
 */
static errr do_get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type hook1, const monsterrace_hook_type hook2, const bool restrict_to_dungeon)
{
    const FloorType *const floor_ptr = player_ptr->current_floor_ptr;

    // デバッグ用統計情報。
    int mon_num = 0; // 重み(prob2)が正の要素数
    DEPTH lev_min = MAX_DEPTH; // 重みが正の要素のうち最小階
    DEPTH lev_max = 0; // 重みが正の要素のうち最大階
    int prob2_total = 0; // 重みの総和

    // モンスター生成テーブルの各要素について重みを修正する。
    for (auto i = 0U; i < alloc_race_table.size(); i++) {
        alloc_entry *const entry = &alloc_race_table[i];
        const auto entry_r_idx = i2enum<MonsterRaceId>(entry->index);
        const MonsterRaceInfo *const r_ptr = &monraces_info[entry_r_idx];

        // 生成を禁止する要素は重み 0 とする。
        entry->prob2 = 0;

        // 基本重みが 0 以下なら生成禁止。
        // テーブル内の無効エントリもこれに該当する(alloc_race_table は生成時にゼロクリアされるため)。
        if (entry->prob1 <= 0) {
            continue;
        }

        // いずれかの生成制約関数が偽を返したら生成禁止。
        if ((hook1 && !hook1(player_ptr, entry_r_idx)) || (hook2 && !hook2(player_ptr, entry_r_idx))) {
            continue;
        }

        // 原則生成禁止するものたち(フェイズアウト状態 / カメレオンの変身先 / ダンジョンの主召喚 は例外)。
        if (!player_ptr->phase_out && !chameleon_change_m_idx && summon_specific_type != SUMMON_GUARDIANS) {
            // クエストモンスターは生成禁止。
            if (r_ptr->flags1 & RF1_QUESTOR) {
                continue;
            }

            // ダンジョンの主は生成禁止。
            if (r_ptr->flags7 & RF7_GUARDIAN) {
                continue;
            }

            // RF1_FORCE_DEPTH フラグ持ちは指定階未満では生成禁止。
            if ((r_ptr->flags1 & RF1_FORCE_DEPTH) && (r_ptr->level > floor_ptr->dun_level)) {
                continue;
            }

            // クエスト内でRES_ALLの生成を禁止する (殲滅系クエストの詰み防止)
            if (inside_quest(player_ptr->current_floor_ptr->quest_number) && r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
                continue;
            }
        }

        // 生成を許可するものは基本重みをそのまま引き継ぐ。
        entry->prob2 = entry->prob1;

        // 引数で指定されていればさらにダンジョンによる制約を試みる。
        if (restrict_to_dungeon) {
            // ダンジョンによる制約を適用する条件:
            //
            //   * フェイズアウト状態でない
            //   * 1階かそれより深いところにいる
            //   * ランダムクエスト中でない
            const bool in_random_quest = inside_quest(floor_ptr->quest_number) && !QuestType::is_fixed(floor_ptr->quest_number);
            const bool cond = !player_ptr->phase_out && floor_ptr->dun_level > 0 && !in_random_quest;

            if (cond && !restrict_monster_to_dungeon(floor_ptr, entry_r_idx)) {
                // ダンジョンによる制約に掛かった場合、重みを special_div/64 倍する。
                // 丸めは確率的に行う。
                const int numer = entry->prob2 * floor_ptr->get_dungeon_definition().special_div;
                const int q = numer / 64;
                const int r = numer % 64;
                entry->prob2 = (PROB)(randint0(64) < r ? q + 1 : q);
            }
        }

        // 統計情報更新。
        if (entry->prob2 > 0) {
            mon_num++;
            if (lev_min > entry->level) {
                lev_min = entry->level;
            }
            if (lev_max < entry->level) {
                lev_max = entry->level;
            }
            prob2_total += entry->prob2;
        }
    }

    // チートオプションが有効なら統計情報を出力。
    if (cheat_hear) {
        msg_format(_("モンスター第2次候補数:%d(%d-%dF)%d ", "monster second selection:%d(%d-%dF)%d "), mon_num, lev_min, lev_max, prob2_total);
    }

    return 0;
}

/*!
 * @brief モンスター生成テーブルの重み修正
 * @param player_ptr
 * @param hook1 生成制約関数1 (nullptr の場合、制約なし)
 * @param hook2 生成制約関数2 (nullptr の場合、制約なし)
 * @return 常に 0
 *
 * get_mon_num() を呼ぶ前に get_mon_num_prep() 系関数のいずれかを呼ぶこと。
 */
errr get_mon_num_prep(PlayerType *player_ptr, const monsterrace_hook_type hook1, const monsterrace_hook_type hook2)
{
    return do_get_mon_num_prep(player_ptr, hook1, hook2, true);
}

/*!
 * @brief モンスター生成テーブルの重み修正(賞金首選定用)
 * @return 常に 0
 *
 * get_mon_num() を呼ぶ前に get_mon_num_prep 系関数のいずれかを呼ぶこと。
 */
errr get_mon_num_prep_bounty(PlayerType *player_ptr)
{
    return do_get_mon_num_prep(player_ptr, nullptr, nullptr, false);
}
