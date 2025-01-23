#include "floor/line-of-sight.h"
#include "floor/cave.h"
#include "system/floor/floor-info.h"

/*!
 * @brief LOS(Line Of Sight / 視線が通っているか)の判定を行う。
 * @param floor フロアへの参照
 * @param pos_from 始点の座標
 * @param pos_to 終点の座標
 * @return LOSが通っているならTRUEを返す。
 * @details
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,\n
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.\n
 *\n
 * Returns TRUE if a line of sight can be traced from (pos_from.x,pos_from.y) to (pos_to.x,pos_to.y).\n
 *\n
 * The LOS begins at the center of the tile (pos_from.x,pos_from.y) and ends at the center of\n
 * the tile (pos_to.x,pos_to.y).  If los() is to return TRUE, all of the tiles this line\n
 * passes through must be floor tiles, except for (pos_from.x,pos_from.y) and (pos_to.x,pos_to.y).\n
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
bool los(const FloorType &floor, const Pos2D &pos_from, const Pos2D &pos_to)
{
    const auto dy = pos_to.y - pos_from.y;
    const auto dx = pos_to.x - pos_from.x;
    const auto ay = std::abs(dy);
    const auto ax = std::abs(dx);
    if ((ax < 2) && (ay < 2)) {
        return true;
    }

    /* Directly South/North */
    if (!dx) {
        /* South -- check for walls */
        if (dy > 0) {
            for (auto ty = pos_from.y + 1; ty < pos_to.y; ty++) {
                if (!cave_los_bold(&floor, ty, pos_from.x)) {
                    return false;
                }
            }
        }

        /* North -- check for walls */
        else {
            for (auto ty = pos_from.y - 1; ty > pos_to.y; ty--) {
                if (!cave_los_bold(&floor, ty, pos_from.x)) {
                    return false;
                }
            }
        }

        /* Assume los */
        return true;
    }

    /* Directly East/West */
    if (!dy) {
        /* East -- check for walls */
        if (dx > 0) {
            for (auto tx = pos_from.x + 1; tx < pos_to.x; tx++) {
                if (!cave_los_bold(&floor, pos_from.y, tx)) {
                    return false;
                }
            }
        }

        /* West -- check for walls */
        else {
            for (auto tx = pos_from.x - 1; tx > pos_to.x; tx--) {
                if (!cave_los_bold(&floor, pos_from.y, tx)) {
                    return false;
                }
            }
        }

        return true;
    }

    const auto sx = (dx < 0) ? -1 : 1;
    const auto sy = (dy < 0) ? -1 : 1;

    if (ax == 1) {
        if (ay == 2) {
            if (cave_los_bold(&floor, pos_from.y + sy, pos_from.x)) {
                return true;
            }
        }
    } else if (ay == 1) {
        if (ax == 2) {
            if (cave_los_bold(&floor, pos_from.y, pos_from.x + sx)) {
                return true;
            }
        }
    }

    const auto f2 = (ax * ay);
    const auto f1 = f2 << 1;
    if (ax >= ay) {
        auto qy = ay * ay;
        const auto m = qy << 1;
        auto ty = pos_from.y;
        auto tx = pos_from.x + sx;
        if (qy == f2) {
            ty += sy;
            qy -= f1;
        }

        /* Note (below) the case (qy == f2), where */
        /* the LOS exactly meets the corner of a tile. */
        while (pos_to.x - tx) {
            if (!cave_los_bold(&floor, ty, tx)) {
                return false;
            }

            qy += m;

            if (qy < f2) {
                tx += sx;
                continue;
            }

            if (qy > f2) {
                ty += sy;
                if (!cave_los_bold(&floor, ty, tx)) {
                    return false;
                }
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
    auto qx = ax * ax;
    const auto m = qx << 1;
    auto ty = pos_from.y + sy;
    auto tx = pos_from.x;
    if (qx == f2) {
        tx += sx;
        qx -= f1;
    }

    /* Note (below) the case (qx == f2), where */
    /* the LOS exactly meets the corner of a tile. */
    while (pos_to.y - ty) {
        if (!cave_los_bold(&floor, ty, tx)) {
            return false;
        }

        qx += m;

        if (qx < f2) {
            ty += sy;
            continue;
        }

        if (qx > f2) {
            tx += sx;
            if (!cave_los_bold(&floor, ty, tx)) {
                return false;
            }
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
