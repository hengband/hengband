#include "util/sort.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster/monster-flag-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/point-2d.h"

/*
 * @brief クイックソートの実行 / Quick sort in place
 * @param u アイテムやモンスター等への配列
 * @param v 条件基準IDへの参照ポインタ
 * @param a 比較対象のID1
 * @param b 比較対象のID2
 * @param ang_sort_comp 比較用の関数ポインタ
 * @param ang_sort_swap スワップ用の関数ポインタ
 */
static void exe_ang_sort(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int p, int q, bool (*ang_sort_comp)(PlayerType *, std::vector<int> &, std::vector<int> &, int, int))
{
    if (p >= q) {
        return;
    }

    int z = p;
    int a = p;
    int b = q;
    while (true) {
        /* Slide i2 */
        while (!(*ang_sort_comp)(player_ptr, ys, xs, b, z)) {
            b--;
        }

        /* Slide i1 */
        while (!(*ang_sort_comp)(player_ptr, ys, xs, z, a)) {
            a++;
        }

        if (a >= b) {
            break;
        }

        std::swap(xs[a], xs[b]);
        std::swap(ys[a], ys[b]);
        a++, b--;
    }

    /* Recurse left side */
    exe_ang_sort(player_ptr, ys, xs, p, b, ang_sort_comp);

    /* Recurse right side */
    exe_ang_sort(player_ptr, ys, xs, b + 1, q, ang_sort_comp);
}

/*
 * @brief クイックソートの受け付け / Accepting auick sort in place
 * @param u アイテムやモンスター等への配列
 * @param v 条件基準IDへの参照ポインタ
 * @param a 比較対象のID1
 * @param b 比較対象のID2
 * @param ang_sort_comp 比較用の関数ポインタ
 * @param ang_sort_swap スワップ用の関数ポインタ
 */
void ang_sort(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int n, bool (*ang_sort_comp)(PlayerType *, std::vector<int> &, std::vector<int> &, int, int))
{
    exe_ang_sort(player_ptr, ys, xs, 0, n - 1, ang_sort_comp);
}

/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b)
{
    /* Absolute distance components */
    const auto p_pos = player_ptr->get_position();
    auto xa = xs[a];
    xa -= p_pos.x;
    xa = std::abs(xa);
    auto ya = ys[a];
    ya -= p_pos.y;
    ya = std::abs(ya);

    /* Approximate Double Distance to the first point */
    auto da = (xa > ya) ? (xa + xa + ya) : (ya + ya + xa);

    /* Absolute distance components */
    auto xb = xs[b];
    xb -= p_pos.x;
    xb = std::abs(xb);
    auto yb = ys[b];
    yb -= p_pos.y;
    yb = std::abs(yb);

    /* Approximate Double Distance to the first point */
    auto db = (xb > yb) ? (xb + xb + yb) : (yb + yb + xb);
    return da <= db;
}

/*
 * Sorting hook -- comp function -- by importance level of grids
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by level of monster
 */
bool ang_sort_comp_importance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid1 = floor.get_grid({ ys[a], xs[a] });
    const auto &grid2 = floor.get_grid({ ys[b], xs[b] });
    const auto &monster_a = floor.m_list[grid1.m_idx];
    const auto &monster_b = floor.m_list[grid2.m_idx];
    const auto p_pos = player_ptr->get_position();
    if (p_pos == Pos2D(ys[a], xs[a])) {
        return true;
    }

    if (p_pos == Pos2D(ys[b], xs[b])) {
        return false;
    }

    const auto can_see_grid1 = grid1.has_monster() && monster_a.ml;
    const auto can_see_grid2 = grid2.has_monster() && monster_b.ml;
    if (can_see_grid1 && !can_see_grid2) {
        return true;
    }

    if (!can_see_grid1 && can_see_grid2) {
        return false;
    }

    if (can_see_grid1 && can_see_grid2) {
        const auto &appearent_monrace1 = monster_a.get_appearance_monrace();
        const auto &appearent_monrace2 = monster_b.get_appearance_monrace();
        if (appearent_monrace1.kind_flags.has(MonsterKindType::UNIQUE) && appearent_monrace2.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            return true;
        }

        if (appearent_monrace1.kind_flags.has_not(MonsterKindType::UNIQUE) && appearent_monrace2.kind_flags.has(MonsterKindType::UNIQUE)) {
            return false;
        }

        if (monster_a.mflag2.has(MonsterConstantFlagType::KAGE) && monster_b.mflag2.has_not(MonsterConstantFlagType::KAGE)) {
            return true;
        }

        if (monster_a.mflag2.has_not(MonsterConstantFlagType::KAGE) && monster_b.mflag2.has(MonsterConstantFlagType::KAGE)) {
            return false;
        }

        if (!appearent_monrace1.r_tkills && appearent_monrace2.r_tkills) {
            return true;
        }

        if (appearent_monrace1.r_tkills && !appearent_monrace2.r_tkills) {
            return false;
        }

        if (appearent_monrace1.r_tkills && appearent_monrace2.r_tkills) {
            if (appearent_monrace1.level > appearent_monrace2.level) {
                return true;
            }

            if (appearent_monrace1.level < appearent_monrace2.level) {
                return false;
            }
        }

        if (monster_a.ap_r_idx > monster_b.ap_r_idx) {
            return true;
        }

        if (monster_a.ap_r_idx < monster_b.ap_r_idx) {
            return false;
        }
    }

    if (!grid1.o_idx_list.empty() && grid2.o_idx_list.empty()) {
        return true;
    }

    if (grid1.o_idx_list.empty() && !grid2.o_idx_list.empty()) {
        return false;
    }

    const auto &terrain_a = grid1.get_terrain();
    const auto &terrain_b = grid2.get_terrain();
    if (terrain_a.priority > terrain_b.priority) {
        return true;
    }

    if (terrain_a.priority < terrain_b.priority) {
        return false;
    }

    return ang_sort_comp_distance(player_ptr, ys, xs, a, b);
}
