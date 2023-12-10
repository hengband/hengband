#include "floor/geometry.h"
#include "floor/cave.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"

/*!
 * キーパッドの方向を南から反時計回り順に列挙 / Global array for looping through the "keypad directions"
 */
const int ddd[9] = { 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/*!
 * dddで定義した順にベクトルのX軸成分を定義 / Global arrays for converting "keypad direction" into offsets
 */
const int ddx[10] = { 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };

/*!
 * dddで定義した順にベクトルのY軸成分を定義 / Global arrays for converting "keypad direction" into offsets
 */
const int ddy[10] = { 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 };

/*!
 * ddd越しにベクトルのX軸成分を定義 / Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
const int ddx_ddd[9] = { 0, 0, 1, -1, 1, -1, 1, -1, 0 };

/*!
 * ddd越しにベクトルのY軸成分を定義 / Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
const int ddy_ddd[9] = { 1, -1, 0, 0, 1, 1, -1, -1, 0 };

/*!
 * キーパッドの円環状方向配列 / Circular keypad direction array
 */
const int cdd[8] = { 2, 3, 6, 9, 8, 7, 4, 1 };

/*!
 * cdd越しにベクトルのX軸成分を定義 / Global arrays for optimizing "ddx[cdd[i]]" and "ddy[cdd[i]]"
 */
const int ddx_cdd[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

/*!
 * cdd越しにベクトルのY軸成分を定義 / Global arrays for optimizing "ddx[cdd[i]]" and "ddy[cdd[i]]"
 */
const int ddy_cdd[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

/*!
 * @brief 2点間の距離をニュートン・ラプソン法で算出する / Distance between two points via Newton-Raphson technique
 * @param y1 1点目のy座標
 * @param x1 1点目のx座標
 * @param y2 2点目のy座標
 * @param x2 2点目のx座標
 * @return 2点間の距離
 */
POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    POSITION dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
    POSITION dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);

    /* Squared distance */
    POSITION target = (dy * dy) + (dx * dx);

    /* Approximate distance: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2 */
    POSITION d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

    POSITION err;

    /* Simple case */
    if (!dy || !dx) {
        return d;
    }

    while (true) {
        /* Approximate error */
        err = (target - d * d) / (2 * d);

        /* No error - we are done */
        if (!err) {
            break;
        }

        /* Adjust distance */
        d += err;
    }

    return d;
}

/*!
 * @brief プレイヤーから指定の座標がどの方角にあるかを返す /
 * Convert an adjacent location to a direction.
 * @param y 方角を確認したY座標
 * @param x 方角を確認したX座標
 * @return 方向ID
 */
DIRECTION coords_to_dir(PlayerType *player_ptr, POSITION y, POSITION x)
{
    DIRECTION d[3][3] = { { 7, 4, 1 }, { 8, 5, 2 }, { 9, 6, 3 } };
    POSITION dy, dx;

    dy = y - player_ptr->y;
    dx = x - player_ptr->x;
    if (std::abs(dx) > 1 || std::abs(dy) > 1) {
        return 0;
    }

    return d[dx + 1][dy + 1];
}

/*!
 * @brief 指定された座標をプレイヤーが視覚に収められるかを返す。 / Can the player "see" the given grid in detail?
 * @param y y座標
 * @param x x座標
 * @return 視覚に収められる状態ならTRUEを返す
 * @details
 * He must have vision, illumination, and line of sight.\n
 * \n
 * Note -- "CAVE_LITE" is only set if the "torch" has "los()".\n
 * So, given "CAVE_LITE", we know that the grid is "fully visible".\n
 *\n
 * Note that "CAVE_GLOW" makes little sense for a wall, since it would mean\n
 * that a wall is visible from any direction.  That would be odd.  Except\n
 * under wizard light, which might make sense.  Thus, for walls, we require\n
 * not only that they be "CAVE_GLOW", but also, that they be adjacent to a\n
 * grid which is not only "CAVE_GLOW", but which is a non-wall, and which is\n
 * in line of sight of the player.\n
 *\n
 * This extra check is expensive, but it provides a more "correct" semantics.\n
 *\n
 * Note that we should not run this check on walls which are "outer walls" of\n
 * the dungeon, or we will induce a memory fault, but actually verifying all\n
 * of the locations would be extremely expensive.\n
 *\n
 * Thus, to speed up the function, we assume that all "perma-walls" which are\n
 * "CAVE_GLOW" are "illuminated" from all sides.  This is correct for all cases\n
 * except "vaults" and the "buildings" in town.  But the town is a hack anyway,\n
 * and the player has more important things on his mind when he is attacking a\n
 * monster vault.  It is annoying, but an extremely important optimization.\n
 *\n
 * Note that "glowing walls" are only considered to be "illuminated" if the\n
 * grid which is next to the wall in the direction of the player is also a\n
 * "glowing" grid.  This prevents the player from being able to "see" the\n
 * walls of illuminated rooms from a corridor outside the room.\n
 */
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    /* Blind players see nothing */
    if (player_ptr->effects()->blindness()->is_blind()) {
        return false;
    }

    const Pos2D pos(y, x);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);

    /* Note that "torch-lite" yields "illumination" */
    if (grid.info & (CAVE_LITE | CAVE_MNLT)) {
        return true;
    }

    /* Require line of sight to the grid */
    if (!grid.has_los()) {
        return false;
    }

    /* Noctovision of Ninja */
    if (player_ptr->see_nocto) {
        return true;
    }

    /* Require "perma-lite" of the grid */
    if ((grid.info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
        return false;
    }

    /* Feature code (applying "mimic" field) */
    /* Floors are simple */
    if (feat_supports_los(grid.get_feat_mimic())) {
        return true;
    }

    /* Check for "local" illumination */
    return check_local_illumination(player_ptr, y, x);
}

/*
 * Calculate "incremental motion". Used by project() and shoot().
 * Assumes that (*y,*x) lies on the path from (y1,x1) to (y2,x2).
 */
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2)
{
    /* Extract the distance travelled */
    auto dy = (pos_orig.y < pos1.y) ? pos1.y - pos_orig.y : pos_orig.y - pos1.y;
    auto dx = (pos_orig.x < pos1.x) ? pos1.x - pos_orig.x : pos_orig.x - pos1.x;

    /* Number of steps */
    auto dist = (dy > dx) ? dy : dx;

    /* We are calculating the next location */
    dist++;

    /* Calculate the total distance along each axis */
    dy = (pos2.y < pos1.y) ? (pos1.y - pos2.y) : (pos2.y - pos1.y);
    dx = (pos2.x < pos1.x) ? (pos1.x - pos2.x) : (pos2.x - pos1.x);

    /* Paranoia -- Hack -- no motion */
    if (!dy && !dx) {
        return pos_orig;
    }

    /* Move mostly vertically */
    if (dy > dx) {
        /* Extract a shift factor */
        auto shift = (dist * dx + (dy - 1) / 2) / dy;

        /* Sometimes move along the minor axis, Always move along major axis */
        const auto y = (pos2.y < pos1.y) ? (pos1.y - dist) : (pos1.y + dist);
        const auto x = (pos2.x < pos1.x) ? (pos1.x - shift) : (pos1.x + shift);
        return { y, x };
    }

    /* Move mostly horizontally */
    auto shift = (dist * dy + (dx - 1) / 2) / dx;
    const auto y = (pos2.y < pos1.y) ? (pos1.y - shift) : (pos1.y + shift);
    const auto x = (pos2.x < pos1.x) ? (pos1.x - dist) : (pos1.x + dist);
    return { y, x };
}

/*!
 * @brief Is the monster seen by the player?
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 個々のモンスターへの参照ポインタ
 * @return 個々のモンスターがプレイヤーが見えたらTRUE
 * @todo is_seen() の関数マクロをバラそうとしたがインクルード関係のコンパイルエラーで失敗
 */
bool is_seen(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    bool is_inside_view = !ignore_unview;
    is_inside_view |= AngbandSystem::get_instance().is_phase_out();
    is_inside_view |= player_can_see_bold(player_ptr, m_ptr->fy, m_ptr->fx) && projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx);
    return m_ptr->ml && is_inside_view;
}
