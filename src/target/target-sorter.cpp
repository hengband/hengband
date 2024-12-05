#include "target/target-sorter.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "grid/grid.h"
#include "monster/monster-flag-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/terrain-type-definition.h"

TargetSorter::TargetSorter(const Pos2D &p_pos)
    : p_pos(p_pos)
{
}

/*!
 * @brief 座標の重要度を比較する
 * @param pos_a 比較対象の座標番号1.
 * @param pos_b 比較対象の座標番号2.
 * @return pos_aの座標がpos_bの座標よりプレイヤーにとって重要ならtrue、そうでないならfalse
 */
bool TargetSorter::compare_importance(const FloorType &floor, const Pos2D &pos_a, const Pos2D &pos_b) const
{
    if (pos_a == pos_b) {
        return false;
    }

    const auto &grid1 = floor.get_grid(pos_a);
    const auto &grid2 = floor.get_grid(pos_b);
    const auto &monster_a = floor.m_list[grid1.m_idx];
    const auto &monster_b = floor.m_list[grid2.m_idx];
    if (this->p_pos == pos_a) {
        return true;
    }

    if (this->p_pos == pos_b) {
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
            const auto order_level = appearent_monrace1.order_level(appearent_monrace2);
            if (order_level) {
                return *order_level;
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

    return this->compare_distance(pos_a, pos_b);
}

/*!
 * @brief プレイヤーからの距離を比較する
 * @param pos_a 比較対象の座標番号1.
 * @param pos_b 比較対象の座標番号2.
 * @return pos_aの座標がpos_bの座標よりプレイヤー座標に近いかならtrue、同一か遠いならfalse
 * @details
 * アルゴリズムは以下の通り.
 * 1. pos_aとpos_bの、プレイヤーからの絶対距離を測る
 * 2. pos_aとpso_bの、Double Distance (座標の大きい方を2倍して小さい方を足し、距離とする手法) を測る
 * 3. Double Distance の大小を測る.
 */
bool TargetSorter::compare_distance(const Pos2D &pos_a, const Pos2D &pos_b) const
{
    if (pos_a == pos_b) {
        return false;
    }

    const auto distance_a = this->calc_double_distance(pos_a);
    const auto distance_b = this->calc_double_distance(pos_b);
    return distance_a < distance_b;
}

int TargetSorter::calc_double_distance(const Pos2D &pos) const
{
    auto vec = pos - this->p_pos;
    vec = Pos2DVec(std::abs(vec.y), std::abs(vec.x));
    return (vec.x > vec.y) ? (2 * vec.x + vec.y) : (2 * vec.y + vec.x);
}
