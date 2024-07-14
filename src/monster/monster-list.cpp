/*!
 * @brief モンスター処理 / misc code for monsters
 * @date 2014/07/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster-list.h"
#include "core/speed-table.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "system/alloc-entries.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cmath>
#include <iterator>

#define HORDE_NOGOOD 0x01 /*!< (未実装フラグ)HORDE生成でGOODなモンスターの生成を禁止する？ */
#define HORDE_NOEVIL 0x02 /*!< (未実装フラグ)HORDE生成でEVILなモンスターの生成を禁止する？ */

/*!
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(FloorType *floor_ptr)
{
    /* Normal allocation */
    if (floor_ptr->m_max < MAX_FLOOR_MONSTERS) {
        const auto i = floor_ptr->m_max;
        floor_ptr->m_max++;
        floor_ptr->m_cnt++;
        return i;
    }

    /* Recycle dead monsters */
    for (short i = 1; i < floor_ptr->m_max; i++) {
        const auto &monster = floor_ptr->m_list[i];
        if (monster.is_valid()) {
            continue;
        }

        floor_ptr->m_cnt++;
        return i;
    }

    if (AngbandWorld::get_instance().character_dungeon) {
        msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
    }

    return 0;
}

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param min_level 最小生成階
 * @param max_level 最大生成階
 * @return 選択されたモンスター生成種族
 * @details nasty生成 (ゲーム内経過日数に応じて、現在フロアより深いフロアのモンスターを出現させる仕様)は
 */
MonsterRaceId get_mon_num(PlayerType *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS mode)
{
    /* town max_level : same delay as 10F, no nasty mons till day18 */
    auto delay = static_cast<int>(std::sqrt(max_level * 10000)) + (max_level * 5);
    if (!max_level) {
        delay = 360;
    }

    if (max_level > MAX_DEPTH - 1) {
        max_level = MAX_DEPTH - 1;
    }

    /* +1 per day after the base date */
    /* base dates : day5(1F), day18(10F,0F), day34(30F), day53(60F), day69(90F) */
    const auto over_days = std::max<int>(0, AngbandWorld::get_instance().dungeon_turn / (TURNS_PER_TICK * 10000L) - delay / 20);

    /* Probability starts from 1/25, reaches 1/3 after 44days from a max_level dependent base date */
    /* Boost level starts from 0, reaches +25lv after 75days from a max_level dependent base date */
    constexpr auto chance_nasty_monster = 25;
    constexpr auto max_num_nasty_monsters = 3;
    constexpr auto max_depth_nasty_monster = 25;
    auto chance_nasty = std::max(max_num_nasty_monsters, chance_nasty_monster - over_days / 2);
    auto nasty_level = std::min(max_depth_nasty_monster, over_days / 3);
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::MAZE)) {
        chance_nasty = std::min(chance_nasty / 2, chance_nasty - 10);
        if (chance_nasty < 2) {
            chance_nasty = 2;
        }

        nasty_level += 2;
        max_level += 3;
    }

    /* Boost the max_level */
    if (any_bits(mode, PM_ARENA) || dungeon.flags.has_not(DungeonFeatureType::BEGINNER)) {
        /* Nightmare mode allows more out-of depth monsters */
        if (ironman_nightmare && !randint0(chance_nasty)) {
            /* What a bizarre calculation */
            max_level = 1 + (max_level * MAX_DEPTH / randint1(MAX_DEPTH));
        } else {
            /* Occasional "nasty" monster */
            if (!randint0(chance_nasty)) {
                /* Pick a max_level bonus */
                max_level += nasty_level;
            }
        }
    }

    ProbabilityTable<int> prob_table;

    /* Process probabilities */
    const auto &monraces = MonraceList::get_instance();
    for (auto i = 0U; i < alloc_race_table.size(); i++) {
        const auto &entry = alloc_race_table[i];
        if (entry.level < min_level) {
            continue;
        }
        if (max_level < entry.level) {
            break;
        } // sorted by depth array,
        auto r_idx = i2enum<MonsterRaceId>(entry.index);
        auto r_ptr = &monraces_info[r_idx];
        if (none_bits(mode, PM_ARENA | PM_CHAMELEON)) {
            if ((r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) && (r_ptr->cur_num >= r_ptr->max_num) && none_bits(mode, PM_CLONE)) {
                continue;
            }

            if (r_ptr->population_flags.has(MonsterPopulationType::ONLY_ONE) && (r_ptr->cur_num >= 1)) {
                continue;
            }

            if (!monraces.is_selectable(r_idx)) {
                continue;
            }
        }

        prob_table.entry_item(i, entry.prob2);
    }

    if (cheat_hear) {
        msg_format(_("モンスター第3次候補数:%lu(%d-%dF)%d ", "monster third selection:%lu(%d-%dF)%d "), prob_table.item_count(), min_level, max_level,
            prob_table.total_prob());
    }

    if (prob_table.empty()) {
        return MonraceList::empty_id();
    }

    // 40%で1回、50%で2回、10%で3回抽選し、その中で一番レベルが高いモンスターを選択する
    int n = 1;

    const int p = randint0(100);
    if (p < 60) {
        n++;
    }
    if (p < 10) {
        n++;
    }

    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), prob_table, n);

    auto it = std::max_element(result.begin(), result.end(), [](int a, int b) { return alloc_race_table[a].level < alloc_race_table[b].level; });

    return i2enum<MonsterRaceId>(alloc_race_table[*it].index);
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(PlayerType *player_ptr, MonsterRaceId r_idx, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *r_ptr = &monraces_info[r_idx];
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    MonsterRaceInfo *old_r_ptr = &m_ptr->get_monrace();

    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY) || r_ptr->misc_flags.has(MonsterMiscType::CHAMELEON)) {
        return false;
    }

    if (std::abs(r_ptr->level - monraces_info[MonsterRaceId::CHAMELEON_K].level) > 5) {
        return false;
    }

    if (r_ptr->is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, grid.feat, r_ptr, 0)) {
        return false;
    }

    if (old_r_ptr->misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, r_ptr)) {
            return false;
        }
    } else if (summoner_m_idx && monster_has_hostile_align(player_ptr, &floor_ptr->m_list[*summoner_m_idx], 0, 0, r_ptr)) {
        return false;
    }

    return true;
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(PlayerType *player_ptr, MonsterRaceId r_idx, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *r_ptr = &monraces_info[r_idx];
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    MonsterRaceInfo *old_r_ptr = &m_ptr->get_monrace();

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }
    if (r_ptr->misc_flags.has(MonsterMiscType::MULTIPLY)) {
        return false;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY) || (r_ptr->misc_flags.has(MonsterMiscType::CHAMELEON))) {
        return false;
    }

    if (r_ptr->is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, grid.feat, r_ptr, 0)) {
        return false;
    }

    if (old_r_ptr->misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        if (old_r_ptr->kind_flags.has(MonsterKindType::GOOD) && r_ptr->kind_flags.has_not(MonsterKindType::GOOD)) {
            return false;
        }
        if (old_r_ptr->kind_flags.has(MonsterKindType::EVIL) && r_ptr->kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }
        if (old_r_ptr->kind_flags.has_none_of(alignment_mask)) {
            return false;
        }
    } else if (summoner_m_idx && monster_has_hostile_align(player_ptr, &floor_ptr->m_list[*summoner_m_idx], 0, 0, r_ptr)) {
        return false;
    }

    auto hook_pf = get_monster_hook(player_ptr);
    return hook_pf(player_ptr, r_idx);
}

static std::optional<MonsterRaceId> polymorph_of_chameleon(PlayerType *player_ptr, MONSTER_IDX m_idx, const Grid &grid, const std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];

    bool old_unique = false;
    if (m_ptr->get_monrace().kind_flags.has(MonsterKindType::UNIQUE)) {
        old_unique = true;
    }

    auto hook_fp = old_unique ? monster_hook_chameleon_lord : monster_hook_chameleon;
    auto hook = [m_idx, grid, summoner_m_idx, hook_fp](PlayerType *player_ptr, MonsterRaceId r_idx) {
        return hook_fp(player_ptr, r_idx, m_idx, grid, summoner_m_idx);
    };
    get_mon_num_prep_chameleon(player_ptr, std::move(hook));

    int level;

    if (old_unique) {
        level = monraces_info[MonsterRaceId::CHAMELEON_K].level;
    } else if (!floor_ptr->dun_level) {
        level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
    } else {
        level = floor_ptr->dun_level;
    }

    if (floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::CHAMELEON)) {
        level += 2 + randint1(3);
    }

    const auto new_monrace_id = get_mon_num(player_ptr, 0, level, PM_CHAMELEON);

    if (!MonraceList::is_valid(new_monrace_id)) {
        return std::nullopt;
    }

    return new_monrace_id;
}

/*!
 * @brief カメレオンの変身処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 */
void choose_chameleon_polymorph(PlayerType *player_ptr, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];

    auto new_monrace_id = polymorph_of_chameleon(player_ptr, m_idx, grid, summoner_m_idx);
    if (!new_monrace_id) {
        return;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(*new_monrace_id);

    monster.r_idx = *new_monrace_id;
    monster.ap_r_idx = *new_monrace_id;
    update_monster(player_ptr, m_idx, false);
    lite_spot(player_ptr, monster.fy, monster.fx);

    const auto &new_monrace = monster.get_monrace();
    if (new_monrace.brightness_flags.has_any_of(ld_mask) || monrace.brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }
}

/*!
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(FloorType *floor_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION my = m_ptr->fy;
    POSITION mx = m_ptr->fx;
    int count = 0;
    for (int i = 0; i < 7; i++) {
        int ay = my + ddy_ddd[i];
        int ax = mx + ddx_ddd[i];

        if (!in_bounds(floor_ptr, ay, ax)) {
            continue;
        }
        if (floor_ptr->grid_array[ay][ax].has_monster()) {
            count++;
        }
    }

    return count;
}
