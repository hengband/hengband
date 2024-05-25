#include "target/target-sorter.h"
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
#include "system/terrain-type-definition.h"

namespace {
/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(const Pos2D &p_pos, std::vector<int> &ys, std::vector<int> &xs, int a, int b)
{
    /* Absolute distance components */
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
bool ang_sort_comp_importance(const FloorType &floor, const Pos2D &p_pos, std::vector<int> &ys, std::vector<int> &xs, int a, int b)
{
    const auto &grid1 = floor.get_grid({ ys[a], xs[a] });
    const auto &grid2 = floor.get_grid({ ys[b], xs[b] });
    const auto &monster_a = floor.m_list[grid1.m_idx];
    const auto &monster_b = floor.m_list[grid2.m_idx];
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

    return ang_sort_comp_distance(p_pos, ys, xs, a, b);
}

/*
 * @brief クイックソートの実行 / Quick sort in place
 * @param u アイテムやモンスター等への配列
 * @param v 条件基準IDへの参照ポインタ
 * @param a 比較対象のID1
 * @param b 比較対象のID2
 * @param ang_sort_comp 比較用の関数ポインタ
 * @param ang_sort_swap スワップ用の関数ポインタ
 */
void exe_ang_sort(const FloorType &floor, const Pos2D &p_pos, std::vector<int> &ys, std::vector<int> &xs, int p, int q, SortKind kind)
{
    if (p >= q) {
        return;
    }

    int z = p;
    int a = p;
    int b = q;
    while (true) {
        /* Slide i2 */
        auto is_less_i2 = false;
        do {
            switch (kind) {
            case SortKind::DISTANCE:
                is_less_i2 = ang_sort_comp_distance(p_pos, ys, xs, b, z);
                break;
            case SortKind::IMPORTANCE:
                is_less_i2 = ang_sort_comp_importance(floor, p_pos, ys, xs, b, z);
                break;
            default:
                THROW_EXCEPTION(std::logic_error, "Invalid Sort Kind was specified!");
            }

            if (!is_less_i2) {
                b--;
            }
        } while (!is_less_i2);

        /* Slide i1 */
        auto is_less_i1 = false;
        do {
            switch (kind) {
            case SortKind::DISTANCE:
                is_less_i1 = ang_sort_comp_distance(p_pos, ys, xs, z, a);
                break;
            case SortKind::IMPORTANCE:
                is_less_i1 = ang_sort_comp_importance(floor, p_pos, ys, xs, z, a);
                break;
            default:
                THROW_EXCEPTION(std::logic_error, "Invalid Sort Kind was specified!");
            }

            if (!is_less_i1) {
                a++;
            }
        } while (!is_less_i1);

        if (a >= b) {
            break;
        }

        std::swap(xs[a], xs[b]);
        std::swap(ys[a], ys[b]);
        a++, b--;
    }

    /* Recurse left side */
    exe_ang_sort(floor, p_pos, ys, xs, p, b, kind);

    /* Recurse right side */
    exe_ang_sort(floor, p_pos, ys, xs, b + 1, q, kind);
}
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
void ang_sort(const FloorType &floor, const Pos2D &p_pos, std::vector<int> &ys, std::vector<int> &xs, SortKind kind)
{
    exe_ang_sort(floor, p_pos, ys, xs, 0, std::ssize(ys) - 1, kind);
}
