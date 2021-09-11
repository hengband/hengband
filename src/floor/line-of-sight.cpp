#include "floor/line-of-sight.h"
#include "floor/cave.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief LOS(Line Of Sight / 視線が通っているか)の判定を行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 始点のy座標
 * @param x1 始点のx座標
 * @param y2 終点のy座標
 * @param x2 終点のx座標
 * @return LOSが通っているならTRUEを返す。
 * @details
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,\n
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.\n
 *\n
 * Returns TRUE if a line of sight can be traced from (x1,y1) to (x2,y2).\n
 *\n
 * The LOS begins at the center of the tile (x1,y1) and ends at the center of\n
 * the tile (x2,y2).  If los() is to return TRUE, all of the tiles this line\n
 * passes through must be floor tiles, except for (x1,y1) and (x2,y2).\n
 *\n
 * We assume that the "mathematical corner" of a non-floor tile does not\n
 * block line of sight.\n
 *\n
 * Because this function uses (short) ints for all calculations, overflow may\n
 * occur if dx and dy exceed 90.\n
 *\n
 * Once all the degenerate cases are eliminated, the values "qx", "qy", and\n
 * "m" are multiplied by a scale factor "f1 = abs(dx * dy * 2)", so that\n
 * we can use integer arithmetic.\n
 *\n
 * We travel from start to finish along the longer axis, starting at the border\n
 * between the first and second tiles, where the y offset = .5 * slope, taking\n
 * into account the scale factor.  See below.\n
 *\n
 * Also note that this function and the "move towards target" code do NOT\n
 * share the same properties.  Thus, you can see someone, target them, and\n
 * then fire a bolt at them, but the bolt may hit a wall, not them.  However\n,
 * by clever choice of target locations, you can sometimes throw a "curve".\n
 *\n
 * Note that "line of sight" is not "reflexive" in all cases.\n
 *\n
 * Use the "projectable()" routine to test "spell/missile line of sight".\n
 *\n
 * Use the "update_view()" function to determine player line-of-sight.\n
 */
bool los(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    POSITION dy = y2 - y1;
    POSITION dx = x2 - x1;
    POSITION ay = ABS(dy);
    POSITION ax = ABS(dx);
    if ((ax < 2) && (ay < 2))
        return true;

    /* Directly South/North */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    POSITION tx, ty;
    if (!dx) {
        /* South -- check for walls */
        if (dy > 0) {
            for (ty = y1 + 1; ty < y2; ty++) {
                if (!cave_los_bold(floor_ptr, ty, x1))
                    return false;
            }
        }

        /* North -- check for walls */
        else {
            for (ty = y1 - 1; ty > y2; ty--) {
                if (!cave_los_bold(floor_ptr, ty, x1))
                    return false;
            }
        }

        /* Assume los */
        return true;
    }

    /* Directly East/West */
    if (!dy) {
        /* East -- check for walls */
        if (dx > 0) {
            for (tx = x1 + 1; tx < x2; tx++) {
                if (!cave_los_bold(floor_ptr, y1, tx))
                    return false;
            }
        }

        /* West -- check for walls */
        else {
            for (tx = x1 - 1; tx > x2; tx--) {
                if (!cave_los_bold(floor_ptr, y1, tx))
                    return false;
            }
        }

        return true;
    }

    POSITION sx = (dx < 0) ? -1 : 1;
    POSITION sy = (dy < 0) ? -1 : 1;

    if (ax == 1) {
        if (ay == 2) {
            if (cave_los_bold(floor_ptr, y1 + sy, x1))
                return true;
        }
    } else if (ay == 1) {
        if (ax == 2) {
            if (cave_los_bold(floor_ptr, y1, x1 + sx))
                return true;
        }
    }

    POSITION f2 = (ax * ay);
    POSITION f1 = f2 << 1;
    POSITION qy;
    POSITION m;
    if (ax >= ay) {
        qy = ay * ay;
        m = qy << 1;
        tx = x1 + sx;
        if (qy == f2) {
            ty = y1 + sy;
            qy -= f1;
        } else {
            ty = y1;
        }

        /* Note (below) the case (qy == f2), where */
        /* the LOS exactly meets the corner of a tile. */
        while (x2 - tx) {
            if (!cave_los_bold(floor_ptr, ty, tx))
                return false;

            qy += m;

            if (qy < f2) {
                tx += sx;
                continue;
            }

            if (qy > f2) {
                ty += sy;
                if (!cave_los_bold(floor_ptr, ty, tx))
                    return false;
                qy -= f1;
                tx += sx;
                continue;
            }

            ty += sy;
            qy -= f1;
            tx += sx;
        }

        return true;
    }

    /* Travel vertically */
    POSITION qx = ax * ax;
    m = qx << 1;
    ty = y1 + sy;
    if (qx == f2) {
        tx = x1 + sx;
        qx -= f1;
    } else {
        tx = x1;
    }

    /* Note (below) the case (qx == f2), where */
    /* the LOS exactly meets the corner of a tile. */
    while (y2 - ty) {
        if (!cave_los_bold(floor_ptr, ty, tx))
            return false;

        qx += m;

        if (qx < f2) {
            ty += sy;
            continue;
        }

        if (qx > f2) {
            tx += sx;
            if (!cave_los_bold(floor_ptr, ty, tx))
                return false;
            qx -= f1;
            ty += sy;
            continue;
        }

        tx += sx;
        qx -= f1;
        ty += sy;
    }

    return true;
}
