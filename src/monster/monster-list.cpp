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
#include "monster/monster-describer.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-allocation.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cmath>
#include <iterator>

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param min_level 最小生成階
 * @param max_level 最大生成階
 * @return 選択されたモンスター生成種族
 * @details nasty生成 (ゲーム内経過日数に応じて、現在フロアより深いフロアのモンスターを出現させる仕様)は
 */
MonraceId get_mon_num(PlayerType *player_ptr, int min_level, int max_level, uint32_t mode)
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

    ProbabilityTable<MonraceId> prob_table;

    /* Process probabilities */
    const auto &monraces = MonraceList::get_instance();
    const auto &table = MonraceAllocationTable::get_instance();
    for (auto i = 0U; i < table.size(); i++) {
        const auto &entry = table.get_entry(i);
        if (entry.level < min_level) {
            continue;
        }
        if (max_level < entry.level) {
            break;
        } // sorted by depth array,

        auto monrace_id = entry.index;
        if (none_bits(mode, PM_ARENA | PM_CHAMELEON)) {
            if (monraces.is_unified(monrace_id) && monraces.get_monrace(monrace_id).is_dead_unique()) {
                monrace_id = monraces.select_random_separated_unique_of(monrace_id);
            }

            auto &monrace = monraces.get_monrace(monrace_id);
            if (!monrace.can_generate() && none_bits(mode, PM_CLONE)) {
                continue;
            }

            if (monrace.population_flags.has(MonsterPopulationType::ONLY_ONE) && monrace.has_entity()) {
                continue;
            }

            if (monrace.population_flags.has(MonsterPopulationType::BUNBUN_STRIKER) && (monrace.cur_num >= MAX_BUNBUN_NUM)) {
                continue;
            }

            if (!monraces.is_selectable(monrace_id)) {
                continue;
            }
        }

        prob_table.entry_item(monrace_id, entry.prob2);
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

    std::vector<MonraceId> result;
    ProbabilityTable<MonraceId>::lottery(std::back_inserter(result), prob_table, n);
    const auto it = std::max_element(result.begin(), result.end(),
        [&monraces](MonraceId id1, MonraceId id2) { return monraces.get_monrace(id1).level < monraces.get_monrace(id2).level; });
    return *it;
}

static std::optional<MonraceId> polymorph_of_chameleon(PlayerType *player_ptr, short m_idx, short terrain_id, std::optional<short> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    const auto old_unique = monster.get_monrace().kind_flags.has(MonsterKindType::UNIQUE);
    ChameleonTransformation ct(m_idx, terrain_id, old_unique, std::move(summoner_m_idx));
    get_mon_num_prep_chameleon(player_ptr, ct);

    int level;
    if (old_unique) {
        level = MonraceList::get_instance().get_monrace(MonraceId::CHAMELEON_K).level;
    } else if (!floor.is_underground()) {
        level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
    } else {
        level = floor.dun_level;
    }

    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::CHAMELEON)) {
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
void choose_chameleon_polymorph(PlayerType *player_ptr, short m_idx, short terrain_id, std::optional<short> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    auto new_monrace_id = polymorph_of_chameleon(player_ptr, m_idx, terrain_id, summoner_m_idx);
    if (!new_monrace_id) {
        return;
    }

    monster.r_idx = *new_monrace_id;
    monster.ap_r_idx = *new_monrace_id;
}

/*!
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(const FloorType &floor, short m_idx)
{
    const auto &monster = floor.m_list[m_idx];
    const auto m_pos = monster.get_position();
    auto count = 0;
    for (const auto &d : Direction::directions_8()) {
        const auto pos = m_pos + d.vec();
        if (!in_bounds(floor, pos.y, pos.x)) {
            continue;
        }

        if (floor.get_grid(pos).has_monster()) {
            count++;
        }
    }

    return count;
}
