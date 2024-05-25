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

/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
static void ang_sort_swap_position(std::vector<int> &ys, std::vector<int> &xs, int a, int b)
{
    std::swap(xs[a], xs[b]);
    std::swap(ys[a], ys[b]);
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

        ang_sort_swap_position(ys, xs, a, b);

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
    POSITION kx = xs[a];
    kx -= player_ptr->x;
    kx = std::abs(kx);
    POSITION ky = ys[a];
    ky -= player_ptr->y;
    ky = std::abs(ky);

    /* Approximate Double Distance to the first point */
    POSITION da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

    /* Absolute distance components */
    kx = xs[b];
    kx -= player_ptr->x;
    kx = std::abs(kx);
    ky = ys[b];
    ky -= player_ptr->y;
    ky = std::abs(ky);

    /* Approximate Double Distance to the first point */
    POSITION db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

    /* Compare the distances */
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
    const auto &grid_a = floor.get_grid({ ys[a], xs[a] });
    const auto &grid_b = floor.get_grid({ ys[b], xs[b] });
    const auto &monster_a = floor.m_list[grid_a.m_idx];
    const auto &monster_b = floor.m_list[grid_b.m_idx];

    /* The player grid */
    if (ys[a] == player_ptr->y && xs[a] == player_ptr->x) {
        return true;
    }

    if (ys[b] == player_ptr->y && xs[b] == player_ptr->x) {
        return false;
    }

    /* Extract monster race */
    MonsterRaceInfo *ap_r_ptr_a;
    if (grid_a.has_monster() && monster_a.ml) {
        ap_r_ptr_a = &monster_a.get_appearance_monrace();
    } else {
        ap_r_ptr_a = nullptr;
    }

    MonsterRaceInfo *ap_r_ptr_b;
    if (grid_b.has_monster() && monster_b.ml) {
        ap_r_ptr_b = &monster_b.get_appearance_monrace();
    } else {
        ap_r_ptr_b = nullptr;
    }

    if (ap_r_ptr_a && !ap_r_ptr_b) {
        return true;
    }

    if (!ap_r_ptr_a && ap_r_ptr_b) {
        return false;
    }

    /* Compare two monsters */
    if (ap_r_ptr_a && ap_r_ptr_b) {
        /* Unique monsters first */
        if (ap_r_ptr_a->kind_flags.has(MonsterKindType::UNIQUE) && ap_r_ptr_b->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            return true;
        }
        if (ap_r_ptr_a->kind_flags.has_not(MonsterKindType::UNIQUE) && ap_r_ptr_b->kind_flags.has(MonsterKindType::UNIQUE)) {
            return false;
        }

        /* Shadowers first (あやしい影) */
        if (monster_a.mflag2.has(MonsterConstantFlagType::KAGE) && monster_b.mflag2.has_not(MonsterConstantFlagType::KAGE)) {
            return true;
        }
        if (monster_a.mflag2.has_not(MonsterConstantFlagType::KAGE) && monster_b.mflag2.has(MonsterConstantFlagType::KAGE)) {
            return false;
        }

        /* Unknown monsters first */
        if (!ap_r_ptr_a->r_tkills && ap_r_ptr_b->r_tkills) {
            return true;
        }
        if (ap_r_ptr_a->r_tkills && !ap_r_ptr_b->r_tkills) {
            return false;
        }

        /* Higher level monsters first (if known) */
        if (ap_r_ptr_a->r_tkills && ap_r_ptr_b->r_tkills) {
            if (ap_r_ptr_a->level > ap_r_ptr_b->level) {
                return true;
            }
            if (ap_r_ptr_a->level < ap_r_ptr_b->level) {
                return false;
            }
        }

        /* Sort by index if all conditions are same */
        if (monster_a.ap_r_idx > monster_b.ap_r_idx) {
            return true;
        }
        if (monster_a.ap_r_idx < monster_b.ap_r_idx) {
            return false;
        }
    }

    /* An object get higher priority */
    if (!grid_a.o_idx_list.empty() && grid_b.o_idx_list.empty()) {
        return true;
    }

    if (grid_a.o_idx_list.empty() && !grid_b.o_idx_list.empty()) {
        return false;
    }

    /* Priority from the terrain */
    const auto &terrain_a = grid_a.get_terrain();
    const auto &terrain_b = grid_b.get_terrain();
    if (terrain_a.priority > terrain_b.priority) {
        return true;
    }

    if (terrain_a.priority < terrain_b.priority) {
        return false;
    }

    /* If all conditions are same, compare distance */
    return ang_sort_comp_distance(player_ptr, ys, xs, a, b);
}
