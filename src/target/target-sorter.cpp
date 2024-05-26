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

TargetSorter::TargetSorter(const Pos2D &p_pos, const std::vector<int> &ys, const std::vector<int> &xs, SortKind kind)
    : p_pos(p_pos)
    , ys(ys)
    , xs(xs)
    , kind(kind)
{
}

/*
 * @brief クイックソートの受け付け
 * @param floor フロアへの参照
 */
void TargetSorter::sort(const FloorType &floor)
{
    this->exe_sort(floor, 0, std::ssize(this->ys) - 1);
}

const std::vector<int> &TargetSorter::get_result_y() const
{
    return this->ys;
}

const std::vector<int> &TargetSorter::get_result_x() const
{
    return this->xs;
}

/*
 * @brief クイックソートの実行
 * @param floor フロアへの参照
 * @param a ソート対象の座標1
 * @param b ソート対象の座標2
 */
void TargetSorter::exe_sort(const FloorType &floor, int a, int b)
{
    if (a >= b) {
        return;
    }

    auto z = a;
    auto p = a;
    auto q = b;
    while (true) {
        /* Slide i2 */
        auto is_less_i2 = false;
        do {
            switch (this->kind) {
            case SortKind::DISTANCE:
                is_less_i2 = this->compare_distance(q, z);
                break;
            case SortKind::IMPORTANCE:
                is_less_i2 = this->compare_importance(floor, q, z);
                break;
            default:
                THROW_EXCEPTION(std::logic_error, "Invalid Sort Kind was specified!");
            }

            if (!is_less_i2) {
                q--;
            }
        } while (!is_less_i2);

        /* Slide i1 */
        auto is_less_i1 = false;
        do {
            switch (this->kind) {
            case SortKind::DISTANCE:
                is_less_i1 = this->compare_distance(z, p);
                break;
            case SortKind::IMPORTANCE:
                is_less_i1 = this->compare_importance(floor, z, p);
                break;
            default:
                THROW_EXCEPTION(std::logic_error, "Invalid Sort Kind was specified!");
            }

            if (!is_less_i1) {
                p++;
            }
        } while (!is_less_i1);

        if (p >= q) {
            break;
        }

        std::swap(this->xs[p], this->xs[q]);
        std::swap(this->ys[p], this->ys[q]);
        p++, q--;
    }

    /* Recurse left side */
    this->exe_sort(floor, a, q);

    /* Recurse right side */
    this->exe_sort(floor, q + 1, b);
}

/*
 * @brief 座標の重要度でソートする
 * @param a ソート対象の座標番号1. 座標そのものはPos2D(ys[a], xs[a])
 * @param b ソート対象の座標番号2. 座標そのものはPos2D(ys[b], xs[b])
 * @return aの座標がbの座標よりプレイヤー座標に近いか同一ならtrue、遠いならfalse
 */
bool TargetSorter::compare_importance(const FloorType &floor, int a, int b) const
{
    const auto &grid1 = floor.get_grid({ this->ys[a], this->xs[a] });
    const auto &grid2 = floor.get_grid({ this->ys[b], this->xs[b] });
    const auto &monster_a = floor.m_list[grid1.m_idx];
    const auto &monster_b = floor.m_list[grid2.m_idx];
    if (this->p_pos == Pos2D(this->ys[a], this->xs[a])) {
        return true;
    }

    if (this->p_pos == Pos2D(this->ys[b], this->xs[b])) {
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

    return this->compare_distance(a, b);
}

/*
 * @brief プレイヤーからの距離でソートする
 * @param a ソート対象の座標番号1. 座標そのものはPos2D(ys[a], xs[a])
 * @param b ソート対象の座標番号2. 座標そのものはPos2D(ys[b], xs[b])
 * @return aの座標がbの座標よりプレイヤー座標に近いか同一ならtrue、遠いならfalse
 * @details
 * アルゴリズムは以下の通り.
 * 1. 点aと点bの、プレイヤーからの絶対距離を測る
 * 2. 点aと点bの、Double Distance (座標の大きい方を2倍して小さい方を足し、距離とする手法) を測る
 * 3. Double Distance の大小を測る.
 * 注意：「同じならfalse」とSTLのソート関数と同様に扱うと正常に動作しなくなる
 */
bool TargetSorter::compare_distance(int a, int b) const
{
    auto ya = this->ys[a];
    ya -= this->p_pos.y;
    ya = std::abs(ya);
    auto xa = this->xs[a];
    xa -= this->p_pos.x;
    xa = std::abs(xa);
    auto distance_a = (xa > ya) ? (2 * xa + ya) : (2 * ya + xa);

    auto yb = ys[b];
    yb -= this->p_pos.y;
    yb = std::abs(yb);
    auto xb = this->xs[b];
    xb -= this->p_pos.x;
    xb = std::abs(xb);
    auto distance_b = (xb > yb) ? (2 * xb + yb) : (2 * yb + xb);

    return distance_a <= distance_b;
}
