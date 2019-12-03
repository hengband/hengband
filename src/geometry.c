#include "angband.h"
#include "floor.h"
#include "spells.h"


/*!
 * キーパッドの方向を南から反時計回り順に列挙 / Global array for looping through the "keypad directions"
 */
const POSITION ddd[9] =
{ 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/*!
 * dddで定義した順にベクトルのX軸成分を定義 / Global arrays for converting "keypad direction" into offsets
 */
const POSITION ddx[10] =
{ 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };

/*!
 * dddで定義した順にベクトルのY軸成分を定義 / Global arrays for converting "keypad direction" into offsets
 */
const POSITION ddy[10] =
{ 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 };

/*!
 * ddd越しにベクトルのX軸成分を定義 / Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
const POSITION ddx_ddd[9] =
{ 0, 0, 1, -1, 1, -1, 1, -1, 0 };

/*!
 * ddd越しにベクトルのY軸成分を定義 / Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
const POSITION ddy_ddd[9] =
{ 1, -1, 0, 0, 1, 1, -1, -1, 0 };


/*!
 * キーパッドの円環状方向配列 / Circular keypad direction array
 */
const POSITION cdd[8] =
{ 2, 3, 6, 9, 8, 7, 4, 1 };

/*!
 * cdd越しにベクトルのX軸成分を定義 / Global arrays for optimizing "ddx[cdd[i]]" and "ddy[cdd[i]]"
 */
const POSITION ddx_cdd[8] =
{ 0, 1, 1, 1, 0, -1, -1, -1 };

/*!
 * cdd越しにベクトルのY軸成分を定義 / Global arrays for optimizing "ddx[cdd[i]]" and "ddy[cdd[i]]"
 */
const POSITION ddy_cdd[8] =
{ 1, 1, 0, -1, -1, -1, 0, 1 };


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
	if (!dy || !dx) return d;

	while (1)
	{
		/* Approximate error */
		err = (target - d * d) / (2 * d);

		/* No error - we are done */
		if (!err) break;

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
DIRECTION coords_to_dir(player_type *creature_ptr, POSITION y, POSITION x)
{
	DIRECTION d[3][3] = { {7, 4, 1}, {8, 5, 2}, {9, 6, 3} };
	POSITION dy, dx;

	dy = y - creature_ptr->y;
	dx = x - creature_ptr->x;
	if (ABS(dx) > 1 || ABS(dy) > 1) return (0);

	return d[dx + 1][dy + 1];
}

/*!
 * @brief 始点から終点への直線経路を返す /
 * Determine the path taken by a projection.
 * @param gp 経路座標リストを返す参照ポインタ
 * @param range 距離
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @param flg フラグID
 * @return リストの長さ
 * @details
 * <pre>
 * The projection will always start from the grid (y1,x1), and will travel
 * towards the grid (y2,x2), touching one grid per unit of distance along
 * the major axis, and stopping when it enters the destination grid or a
 * wall grid, or has travelled the maximum legal distance of "range".
 *
 * Note that "distance" in this function (as in the "update_view()" code)
 * is defined as "MAX(dy,dx) + MIN(dy,dx)/2", which means that the player
 * actually has an "octagon of projection" not a "circle of projection".
 *
 * The path grids are saved into the grid array pointed to by "gp", and
 * there should be room for at least "range" grids in "gp".  Note that
 * due to the way in which distance is calculated, this function normally
 * uses fewer than "range" grids for the projection path, so the result
 * of this function should never be compared directly to "range".  Note
 * that the initial grid (y1,x1) is never saved into the grid array, not
 * even if the initial grid is also the final grid.
 *
 * The "flg" flags can be used to modify the behavior of this function.
 *
 * In particular, the "PROJECT_STOP" and "PROJECT_THRU" flags have the same
 * semantics as they do for the "project" function, namely, that the path
 * will stop as soon as it hits a monster, or that the path will continue
 * through the destination grid, respectively.
 *
 * The "PROJECT_JUMP" flag, which for the "project()" function means to
 * start at a special grid (which makes no sense in this function), means
 * that the path should be "angled" slightly if needed to avoid any wall
 * grids, allowing the player to "target" any grid which is in "view".
 * This flag is non-trivial and has not yet been implemented, but could
 * perhaps make use of the "vinfo" array (above).
 *
 * This function returns the number of grids (if any) in the path.  This
 * function will return zero if and only if (y1,x1) and (y2,x2) are equal.
 *
 * This algorithm is similar to, but slightly different from, the one used
 * by "update_view_los()", and very different from the one used by "los()".
 * </pre>
 */
sint project_path(u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg)
{
	POSITION y, x;

	int n = 0;
	int k = 0;

	/* Absolute */
	POSITION ay, ax;

	/* Offsets */
	POSITION sy, sx;

	/* Fractions */
	int frac;

	/* Scale factors */
	int full, half;

	/* Slope */
	int m;

	/* No path necessary (or allowed) */
	if ((x1 == x2) && (y1 == y2)) return (0);


	/* Analyze "dy" */
	if (y2 < y1)
	{
		ay = (y1 - y2);
		sy = -1;
	}
	else
	{
		ay = (y2 - y1);
		sy = 1;
	}

	/* Analyze "dx" */
	if (x2 < x1)
	{
		ax = (x1 - x2);
		sx = -1;
	}
	else
	{
		ax = (x2 - x1);
		sx = 1;
	}


	/* Number of "units" in one "half" grid */
	half = (ay * ax);

	/* Number of "units" in one "full" grid */
	full = half << 1;

	/* Vertical */
	if (ay > ax)
	{
		/* Let m = ((dx/dy) * full) = (dx * dx * 2) */
		m = ax * ax * 2;

		/* Start */
		y = y1 + sy;
		x = x1;

		frac = m;

		if (frac > half)
		{
			/* Advance (X) part 2 */
			x += sx;

			/* Advance (X) part 3 */
			frac -= full;

			/* Track distance */
			k++;
		}

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y, x);

			/* Hack -- Check maximum range */
			if ((n + (k >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			if (flg & (PROJECT_DISI))
			{
				if ((n > 0) && cave_stop_disintegration(y, x)) break;
			}
			else if (flg & (PROJECT_LOS))
			{
				if ((n > 0) && !cave_los_bold(p_ptr->current_floor_ptr, y, x)) break;
			}
			else if (!(flg & (PROJECT_PATH)))
			{
				/* Always stop at non-initial wall grids */
				if ((n > 0) && !cave_have_flag_bold(y, x, FF_PROJECT)) break;
			}

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) &&
					(player_bold(p_ptr, y, x) || p_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0))
					break;
			}

			if (!in_bounds(p_ptr->current_floor_ptr, y, x)) break;

			/* Slant */
			if (m)
			{
				/* Advance (X) part 1 */
				frac += m;

				/* Horizontal change */
				if (frac > half)
				{
					/* Advance (X) part 2 */
					x += sx;

					/* Advance (X) part 3 */
					frac -= full;

					/* Track distance */
					k++;
				}
			}

			/* Advance (Y) */
			y += sy;
		}
	}

	/* Horizontal */
	else if (ax > ay)
	{
		/* Let m = ((dy/dx) * full) = (dy * dy * 2) */
		m = ay * ay * 2;

		/* Start */
		y = y1;
		x = x1 + sx;

		frac = m;

		/* Vertical change */
		if (frac > half)
		{
			/* Advance (Y) part 2 */
			y += sy;

			/* Advance (Y) part 3 */
			frac -= full;

			/* Track distance */
			k++;
		}

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y, x);

			/* Hack -- Check maximum range */
			if ((n + (k >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			if (flg & (PROJECT_DISI))
			{
				if ((n > 0) && cave_stop_disintegration(y, x)) break;
			}
			else if (flg & (PROJECT_LOS))
			{
				if ((n > 0) && !cave_los_bold(p_ptr->current_floor_ptr, y, x)) break;
			}
			else if (!(flg & (PROJECT_PATH)))
			{
				/* Always stop at non-initial wall grids */
				if ((n > 0) && !cave_have_flag_bold(y, x, FF_PROJECT)) break;
			}

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) &&
					(player_bold(p_ptr, y, x) || p_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0))
					break;
			}

			if (!in_bounds(p_ptr->current_floor_ptr, y, x)) break;

			/* Slant */
			if (m)
			{
				/* Advance (Y) part 1 */
				frac += m;

				/* Vertical change */
				if (frac > half)
				{
					/* Advance (Y) part 2 */
					y += sy;

					/* Advance (Y) part 3 */
					frac -= full;

					/* Track distance */
					k++;
				}
			}

			/* Advance (X) */
			x += sx;
		}
	}

	/* Diagonal */
	else
	{
		/* Start */
		y = y1 + sy;
		x = x1 + sx;

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y, x);

			/* Hack -- Check maximum range */
			if ((n + (n >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			if (flg & (PROJECT_DISI))
			{
				if ((n > 0) && cave_stop_disintegration(y, x)) break;
			}
			else if (flg & (PROJECT_LOS))
			{
				if ((n > 0) && !cave_los_bold(p_ptr->current_floor_ptr, y, x)) break;
			}
			else if (!(flg & (PROJECT_PATH)))
			{
				/* Always stop at non-initial wall grids */
				if ((n > 0) && !cave_have_flag_bold(y, x, FF_PROJECT)) break;
			}

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) &&
					(player_bold(p_ptr, y, x) || p_ptr->current_floor_ptr->grid_array[y][x].m_idx != 0))
					break;
			}

			if (!in_bounds(p_ptr->current_floor_ptr, y, x)) break;

			/* Advance (Y) */
			y += sy;

			/* Advance (X) */
			x += sx;
		}
	}

	/* Length */
	return (n);
}


/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 */
void scatter(POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode)
{
	POSITION nx, ny;

	/* Pick a location */
	while (TRUE)
	{
		/* Pick a new location */
		ny = rand_spread(y, d);
		nx = rand_spread(x, d);

		/* Ignore annoying locations */
		if (!in_bounds(p_ptr->current_floor_ptr, ny, nx)) continue;

		/* Ignore "excessively distant" locations */
		if ((d > 1) && (distance(y, x, ny, nx) > d)) continue;

		if (mode & PROJECT_LOS)
		{
			if (los(p_ptr->current_floor_ptr, y, x, ny, nx)) break;
		}
		else
		{
			if (projectable(p_ptr->current_floor_ptr, y, x, ny, nx)) break;
		}

	}

	/* Save the location */
	(*yp) = ny;
	(*xp) = nx;
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
bool player_can_see_bold(POSITION y, POSITION x)
{
	grid_type *g_ptr;

	/* Blind players see nothing */
	if (p_ptr->blind) return FALSE;

	g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	/* Note that "torch-lite" yields "illumination" */
	if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) return TRUE;

	/* Require line of sight to the grid */
	if (!player_has_los_bold(p_ptr, y, x)) return FALSE;

	/* Noctovision of Ninja */
	if (p_ptr->see_nocto) return TRUE;

	/* Require "perma-lite" of the grid */
	if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) return FALSE;

	/* Feature code (applying "mimic" field) */
	/* Floors are simple */
	if (feat_supports_los(get_feat_mimic(g_ptr))) return TRUE;

	/* Check for "local" illumination */
	return check_local_illumination(y, x);
}

/*
 * Calculate "incremental motion". Used by project() and shoot().
 * Assumes that (*y,*x) lies on the path from (y1,x1) to (y2,x2).
 */
void mmove2(POSITION *y, POSITION *x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION dy, dx, dist, shift;

	/* Extract the distance travelled */
	dy = (*y < y1) ? y1 - *y : *y - y1;
	dx = (*x < x1) ? x1 - *x : *x - x1;

	/* Number of steps */
	dist = (dy > dx) ? dy : dx;

	/* We are calculating the next location */
	dist++;


	/* Calculate the total distance along each axis */
	dy = (y2 < y1) ? (y1 - y2) : (y2 - y1);
	dx = (x2 < x1) ? (x1 - x2) : (x2 - x1);

	/* Paranoia -- Hack -- no motion */
	if (!dy && !dx) return;


	/* Move mostly vertically */
	if (dy > dx)
	{
		/* Extract a shift factor */
		shift = (dist * dx + (dy - 1) / 2) / dy;

		/* Sometimes move along the minor axis */
		(*x) = (x2 < x1) ? (x1 - shift) : (x1 + shift);

		/* Always move along major axis */
		(*y) = (y2 < y1) ? (y1 - dist) : (y1 + dist);
	}

	/* Move mostly horizontally */
	else
	{
		/* Extract a shift factor */
		shift = (dist * dy + (dx - 1) / 2) / dx;

		/* Sometimes move along the minor axis */
		(*y) = (y2 < y1) ? (y1 - shift) : (y1 + shift);

		/* Always move along major axis */
		(*x) = (x2 < x1) ? (x1 - dist) : (x1 + dist);
	}
}

