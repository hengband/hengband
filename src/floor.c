#include "angband.h"
#include "floor.h"
#include "floor-save.h"
#include "grid.h"
#include "dungeon.h"
#include "rooms.h"
#include "quest.h"

/*
 * The array of floor [MAX_WID][MAX_HGT].
 * Not completely allocated, that would be inefficient
 * Not completely hardcoded, that would overflow memory
 */
floor_type floor_info;

/*
 * The array of saved floors
 */
saved_floor_type saved_floors[MAX_SAVED_FLOORS];

/*!
* @brief 鍵のかかったドアを配置する
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @return なし
*/
void place_locked_door(floor_type *floor_ptr, POSITION y, POSITION x)
{
	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(floor_ptr, y, x);
	}
	else
	{
		set_cave_feat(floor_ptr, y, x, feat_locked_door_random((d_info[p_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
		floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
		delete_monster(y, x);
	}
}


/*!
* @brief 隠しドアを配置する
* @param y 配置したいフロアのY座標
* @param x 配置したいフロアのX座標
* @param type DOOR_DEFAULT / DOOR_DOOR / DOOR_GLASS_DOOR / DOOR_CURTAIN のいずれか
* @return なし
*/
void place_secret_door(floor_type *floor_ptr, POSITION y, POSITION x, int type)
{
	if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(floor_ptr, y, x);
	}
	else
	{
		grid_type *g_ptr = &floor_ptr->grid_array[y][x];

		if (type == DOOR_DEFAULT)
		{
			type = ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_CURTAIN) &&
				one_in_((d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
				((d_info[floor_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
		}

		/* Create secret door */
		place_closed_door(y, x, type);

		if (type != DOOR_CURTAIN)
		{
			/* Hide by inner wall because this is used in rooms only */
			g_ptr->mimic = feat_wall_inner;

			/* Floor type terrain cannot hide a door */
			if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat))
			{
				if (have_flag(f_info[g_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[g_ptr->mimic].flags, FF_CAN_FLY))
				{
					g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
				}
				g_ptr->mimic = 0;
			}
		}

		g_ptr->info &= ~(CAVE_FLOOR);
		delete_monster(y, x);
	}
}

static int scent_when = 0;

/*
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Smell is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away from him.
 *
 * Smell is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent of the current age is laid down.
 * Speedy characters leave more scent, true, but it also ages faster,
 * which makes it harder to hunt them down.
 *
 * Whenever the age count loops, most of the scent trail is erased and
 * the age of the remainder is recalculated.
 */
void update_smell(floor_type *floor_ptr, player_type *subject_ptr)
{
	POSITION i, j;
	POSITION y, x;

	/* Create a table that controls the spread of scent */
	const int scent_adjust[5][5] =
	{
		{ -1, 0, 0, 0,-1 },
		{  0, 1, 1, 1, 0 },
		{  0, 1, 2, 1, 0 },
		{  0, 1, 1, 1, 0 },
		{ -1, 0, 0, 0,-1 },
	};

	/* Loop the age and adjust scent values when necessary */
	if (++scent_when == 254)
	{
		/* Scan the entire dungeon */
		for (y = 0; y < floor_ptr->height; y++)
		{
			for (x = 0; x < floor_ptr->width; x++)
			{
				int w = floor_ptr->grid_array[y][x].when;
				floor_ptr->grid_array[y][x].when = (w > 128) ? (w - 128) : 0;
			}
		}

		/* Restart */
		scent_when = 126;
	}


	/* Lay down new scent */
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			grid_type *g_ptr;

			/* Translate table to map grids */
			y = i + subject_ptr->y - 2;
			x = j + subject_ptr->x - 2;

			/* Check Bounds */
			if (!in_bounds(floor_ptr, y, x)) continue;

			g_ptr = &floor_ptr->grid_array[y][x];

			/* Walls, water, and lava cannot hold scent. */
			if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(g_ptr->feat)) continue;

			/* Grid must not be blocked by walls from the character */
			if (!player_has_los_bold(subject_ptr, y, x)) continue;

			/* Note grids that are too far away */
			if (scent_adjust[i][j] == -1) continue;

			/* Mark the grid with new scent */
			g_ptr->when = scent_when + scent_adjust[i][j];
		}
	}
}


/*
 * Hack -- forget the "flow" information
 */
void forget_flow(floor_type *floor_ptr)
{
	POSITION x, y;

	/* Check the entire dungeon */
	for (y = 0; y < floor_ptr->height; y++)
	{
		for (x = 0; x < floor_ptr->width; x++)
		{
			/* Forget the old data */
			floor_ptr->grid_array[y][x].dist = 0;
			floor_ptr->grid_array[y][x].cost = 0;
			floor_ptr->grid_array[y][x].when = 0;
		}
	}
}

/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(floor_type* floor_ptr, POSITION x, POSITION y)
{
	/* Need to have a wall in the center square */
	if (!is_outer_bold(floor_ptr, y, x)) return;

	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x=don't care
	*  .=floor, #=wall
	*/

	if (is_floor_bold(floor_ptr, y - 1, x) && is_floor_bold(floor_ptr, y + 1, x) &&
		(is_outer_bold(floor_ptr, y, x - 1) && is_outer_bold(floor_ptr, y, x + 1)))
	{
		/* secret door */
		place_secret_door(floor_ptr, y, x, DOOR_DEFAULT);

		/* set boundarys so don't get wide doors */
		place_solid_bold(y, x - 1);
		place_solid_bold(y, x + 1);
	}


	/* look at:
	*  x#x
	*  .#.
	*  x#x
	*
	*  where x = don't care
	*  .=floor, #=wall
	*/
	if (is_outer_bold(floor_ptr, y - 1, x) && is_outer_bold(floor_ptr, y + 1, x) &&
		is_floor_bold(floor_ptr, y, x - 1) && is_floor_bold(floor_ptr, y, x + 1))
	{
		/* secret door */
		place_secret_door(floor_ptr, y, x, DOOR_DEFAULT);

		/* set boundarys so don't get wide doors */
		place_solid_bold(y - 1, x);
		place_solid_bold(y + 1, x);
	}
}

/*!
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 * @return なし
 */
void place_random_stairs(floor_type *floor_ptr, POSITION y, POSITION x)
{
	bool up_stairs = TRUE;
	bool down_stairs = TRUE;
	grid_type *g_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];
	if (!is_floor_grid(g_ptr) || g_ptr->o_idx) return;

	if (!floor_ptr->dun_level) up_stairs = FALSE;
	if (ironman_downward) up_stairs = FALSE;
	if (floor_ptr->dun_level >= d_info[p_ptr->dungeon_idx].maxdepth) down_stairs = FALSE;
	if (quest_number(floor_ptr->dun_level) && (floor_ptr->dun_level > 1)) down_stairs = FALSE;

	/* We can't place both */
	if (down_stairs && up_stairs)
	{
		/* Choose a staircase randomly */
		if (randint0(100) < 50) up_stairs = FALSE;
		else down_stairs = FALSE;
	}

	/* Place the stairs */
	if (up_stairs) set_cave_feat(floor_ptr, y, x, feat_up_stair);
	else if (down_stairs) set_cave_feat(floor_ptr, y, x, feat_down_stair);
}

/*!
 * @brief LOS(Line Of Sight / 視線が通っているか)の判定を行う。
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
bool los(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	/* Delta */
	POSITION dx, dy;

	/* Absolute */
	POSITION ax, ay;

	/* Signs */
	POSITION sx, sy;

	/* Fractions */
	POSITION qx, qy;

	/* Scanners */
	POSITION tx, ty;

	/* Scale factors */
	POSITION f1, f2;

	/* Slope, or 1/Slope, of LOS */
	POSITION m;


	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);


	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return TRUE;


	/* Paranoia -- require "safe" origin */
	/* if (!in_bounds(floor_ptr, y1, x1)) return FALSE; */
	/* if (!in_bounds(floor_ptr, y2, x2)) return FALSE; */


	/* Directly South/North */
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (!cave_los_bold(floor_ptr, ty, x1)) return FALSE;
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (!cave_los_bold(floor_ptr, ty, x1)) return FALSE;
			}
		}

		/* Assume los */
		return TRUE;
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (!cave_los_bold(floor_ptr, y1, tx)) return FALSE;
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (!cave_los_bold(floor_ptr, y1, tx)) return FALSE;
			}
		}

		/* Assume los */
		return TRUE;
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1)
	{
		if (ay == 2)
		{
			if (cave_los_bold(floor_ptr, y1 + sy, x1)) return TRUE;
		}
	}

	/* Horizontal "knights" */
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (cave_los_bold(floor_ptr, y1, x1 + sx)) return TRUE;
		}
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay)
	{
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;

			qy += m;

			if (qy < f2)
			{
				tx += sx;
			}
			else if (qy > f2)
			{
				ty += sy;
				if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;
				qy -= f1;
				tx += sx;
			}
			else
			{
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else
	{
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2)
		{
			tx = x1 + sx;
			qx -= f1;
		}
		else
		{
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty)
		{
			if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;

			qx += m;

			if (qx < f2)
			{
				ty += sy;
			}
			else if (qx > f2)
			{
				tx += sx;
				if (!cave_los_bold(floor_ptr, ty, tx)) return FALSE;
				qx -= f1;
				ty += sy;
			}
			else
			{
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return TRUE;
}


/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION y, x;

	int grid_n = 0;
	u16b grid_g[512];

	/* Check the projection path */
	grid_n = project_path(grid_g, (project_length ? project_length : MAX_RANGE), y1, x1, y2, x2, 0);

	/* Identical grid */
	if (!grid_n) return TRUE;

	/* Final grid */
	y = GRID_Y(grid_g[grid_n - 1]);
	x = GRID_X(grid_g[grid_n - 1]);

	/* May not end in an unrequested grid */
	if ((y != y2) || (x != x2)) return (FALSE);

	/* Assume okay */
	return (TRUE);
}

