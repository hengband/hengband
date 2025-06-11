/*!
 * @brief モンスターの逃走・隠匿に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-safety-hiding.h"
#include "monster-floor/monster-dist-offsets.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "mspell/mspell-checker.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include <span>

/*!
 * @brief モンスターが逃げ込める地点を走査する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param offsets モンスターがいる地点から逃げ込もうとする地点へのオフセットの候補リスト
 * @param d モンスターがいる地点からの距離
 * @return 逃げ込める地点の候補地
 */
static coordinate_candidate sweep_safe_coordinate(PlayerType *player_ptr, MONSTER_IDX m_idx, std::span<const Pos2DVec> offsets, int d)
{
    coordinate_candidate candidate;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = monster.get_position();
    for (const auto &vec : offsets) {
        const auto pos = m_pos + vec;
        if (!floor.contains(pos)) {
            continue;
        }

        const auto &monrace = monster.get_monrace();
        const auto &grid = floor.get_grid(pos);
        BIT_FLAGS16 riding_mode = monster.is_riding() ? CEM_RIDING : 0;
        if (!monster_can_cross_terrain(player_ptr, grid.feat, monrace, riding_mode)) {
            continue;
        }

        if (monster.mflag2.has_not(MonsterConstantFlagType::NOFLOW)) {
            const auto dist = grid.get_distance(monrace.get_grid_flow_type());
            if (dist == 0) {
                continue;
            }
            if (dist > floor.get_grid(m_pos).get_distance(monrace.get_grid_flow_type()) + 2 * d) {
                continue;
            }
        }

        if (projectable(floor, p_pos, p_pos, pos)) {
            continue;
        }

        const auto dis = Grid::calc_distance(pos, p_pos);
        if (dis <= candidate.gdis) {
            continue;
        }

        candidate.pos = pos;
        candidate.gdis = dis;
    }

    return candidate;
}

/*!
 * @brief モンスターが逃げ込める安全な地点を返す /
 * Choose a "safe" location near a monster for it to run toward.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param yp 移動先のマスのY座標を返す参照ポインタ
 * @param xp 移動先のマスのX座標を返す参照ポインタ
 * @return 有効なマスがあった場合TRUEを返す
 * @details
 * A location is "safe" if it can be reached quickly and the player\n
 * is not able to fire into it (it isn't a "clean shot").  So, this will\n
 * cause monsters to "duck" behind walls.  Hopefully, monsters will also\n
 * try to run towards corridor openings if they are in a room.\n
 *\n
 * This function may take lots of CPU time if lots of monsters are\n
 * fleeing.\n
 *\n
 * Return TRUE if a safe location is available.\n
 */
tl::optional<Pos2D> find_safety(PlayerType *player_ptr, short m_idx)
{
    for (auto d = 1; d < 10; d++) {
        const auto candidate = sweep_safe_coordinate(player_ptr, m_idx, DIST_OFFSETS[d], d);
        if (candidate.gdis <= 0) {
            continue;
        }

        return candidate.pos;
    }

    return tl::nullopt;
}

/*!
 * @brief モンスターが隠れられる地点を走査する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param offsets モンスターがいる地点から隠れようとする地点へのオフセットの候補リスト
 * @param candidate 隠れられる地点の候補地
 */
static void sweep_hiding_candidate(
    PlayerType *player_ptr, const MonsterEntity &monster, std::span<const Pos2DVec> offsets, coordinate_candidate &candidate)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monrace = monster.get_monrace();
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = monster.get_position();
    for (const auto &vec : offsets) {
        const auto pos = m_pos + vec;
        if (!floor.contains(pos)) {
            continue;
        }
        if (!monster_can_enter(player_ptr, pos.y, pos.x, monrace, 0)) {
            continue;
        }
        if (projectable(floor, p_pos, p_pos, pos) || !clean_shot(player_ptr, monster.fy, monster.fx, pos.y, pos.x, false)) {
            continue;
        }

        const auto dis = Grid::calc_distance(pos, p_pos);
        if (dis < candidate.gdis && dis >= 2) {
            candidate.pos = pos;
            candidate.gdis = dis;
        }
    }
}

/*!
 * @brief モンスターが隠れ潜める地点を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @return 有効なマスがあった場合、その座標。なかったらnullopt
 */
tl::optional<Pos2D> find_hiding(PlayerType *player_ptr, short m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    coordinate_candidate candidate;
    candidate.gdis = 999;
    for (auto d = 1; d < 10; d++) {
        sweep_hiding_candidate(player_ptr, monster, DIST_OFFSETS[d], candidate);
        if (candidate.gdis >= 999) {
            continue;
        }

        return candidate.pos;
    }

    return tl::nullopt;
}
