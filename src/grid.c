/*
 * File: grid.c
 * Purpose: low-level dungeon creation primitives
 */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"


/*
 * Returns random co-ordinates for player/monster/object
 */
bool new_player_spot(void)
{
	int	y, x;
	int max_attempts = 5000;

	cave_type *c_ptr;

	/* Place the player */
	while (max_attempts--)
	{
		/* Pick a legal spot */
		y = rand_range(1, cur_hgt - 2);
		x = rand_range(1, cur_wid - 2);

		c_ptr = &cave[y][x];

		/* Must be a "naked" floor grid */
		if (!cave_clean_bold(y, x) || c_ptr->m_idx) continue;
		if (!in_bounds(y,x)) continue;

		/* Refuse to start on anti-teleport grids */
		if (c_ptr->info & (CAVE_ICKY)) continue;

		/* Done */
		break;
	}

	if (max_attempts < 1) /* Should be -1, actually if we failed... */
		return FALSE;

	/* Save the new player grid */
	py = y;
	px = x;

	return TRUE;
}


/*
 * Place an up/down staircase at given location
 */
void place_random_stairs(int y, int x)
{
	bool up_stairs = TRUE;
	bool down_stairs = TRUE;
	cave_type *c_ptr;

	/* Paranoia */
	c_ptr = &cave[y][x];
	if (!is_floor_grid(c_ptr) || c_ptr->o_idx) return;

	/* Town */
	if (!dun_level)
		up_stairs = FALSE;

	/* Ironman */
	if (ironman_downward)
		up_stairs = FALSE;

	/* Bottom */
	if (dun_level >= d_info[dungeon_type].maxdepth)
		down_stairs = FALSE;

	/* Quest-level */
	if (quest_number(dun_level) && (dun_level > 1))
		down_stairs = FALSE;

	/* We can't place both */
	if (down_stairs && up_stairs)
	{
		/* Choose a staircase randomly */
		if (rand_int(100) < 50)
			up_stairs = FALSE;
		else
			down_stairs = FALSE;
	}

	/* Place the stairs */
	if (up_stairs)
		place_up_stairs(y, x);
	else if (down_stairs)
		place_down_stairs(y, x);
}


/*
 * Place a random type of door at the given location
 */
void place_random_door(int y, int x)
{
	int tmp;

	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
		return;
	}

	/* Choose an object */
	tmp = rand_int(1000);

	/* Open doors (300/1000) */
	if (tmp < 300)
	{
		/* Create open door */
		cave_set_feat(y, x, FEAT_OPEN);
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400)
	{
		/* Create broken door */
		cave_set_feat(y, x, FEAT_BROKEN);
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600)
	{
		/* Create secret door */
		cave_set_feat(y, x, FEAT_SECRET);
	}

	/* Closed, locked, or stuck doors (400/1000) */
	else place_closed_door(y, x);
}


/*
 * Place a random type of normal door at the given location.
 */
void place_closed_door(int y, int x)
{
	int tmp;

	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
		return;
	}

	/* Choose an object */
	tmp = rand_int(400);

	/* Closed doors (300/400) */
	if (tmp < 300)
	{
		/* Create closed door */
		cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
	}

	/* Locked doors (99/400) */
	else if (tmp < 399)
	{
		/* Create locked door */
		cave_set_feat(y, x, FEAT_DOOR_HEAD + randint(7));
	}

	/* Stuck doors (1/400) */
	else
	{
		/* Create jammed door */
		cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x08 + rand_int(8));
	}
}


/*
 * Make an empty square floor, for the middle of rooms
 */
void place_floor(int x1, int x2, int y1, int y2, bool light)
{
	int x, y;

	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			place_floor_bold(y, x);
			add_cave_info(y, x, CAVE_ROOM);
			if (light) add_cave_info(y, x, CAVE_GLOW);
		}
	}
}


/*
 * Make an empty square room, only floor and wall grids
 */
void place_room(int x1, int x2, int y1, int y2, bool light)
{
	int y, x;

	place_floor(x1, x2, y1, y2, light);

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		place_outer_bold(y, x1 - 1);
		place_outer_bold(y, x2 + 1);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		place_outer_bold(y1 - 1, x);
		place_outer_bold(y2 + 1, x);
	}
}


/*
 * Create up to "num" objects near the given coordinates
 * Only really called by some of the "vault" routines.
 */
void vault_objects(int y, int x, int num)
{
	int dummy = 0;
	int i = 0, j = y, k = x;

	cave_type *c_ptr;


	/* Attempt to place 'num' objects */
	for (; num > 0; --num)
	{
		/* Try up to 11 spots looking for empty space */
		for (i = 0; i < 11; ++i)
		{
			/* Pick a random location */
			while (dummy < SAFE_MAX_ATTEMPTS)
			{
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
				dummy++;
				if (!in_bounds(j, k)) continue;
				break;
			}


			if (dummy >= SAFE_MAX_ATTEMPTS)
			{
				if (cheat_room)
				{
#ifdef JP
msg_print("警告！地下室のアイテムを配置できません！");
#else
					msg_print("Warning! Could not place vault object!");
#endif

				}
			}


			/* Require "clean" floor space */
			c_ptr = &cave[j][k];
			if (!is_floor_grid(c_ptr) || c_ptr->o_idx) continue;

			/* Place an item */
			if (rand_int(100) < 75)
			{
				place_object(j, k, FALSE, FALSE);
			}

			/* Place gold */
			else
			{
				place_gold(j, k);
			}

			/* Placement accomplished */
			break;
		}
	}
}


/*
 * Place a trap with a given displacement of point
 */
void vault_trap_aux(int y, int x, int yd, int xd)
{
	int count = 0, y1 = y, x1 = x;
	int dummy = 0;

	cave_type *c_ptr;

	/* Place traps */
	for (count = 0; count <= 5; count++)
	{
		/* Get a location */
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			dummy++;
			if (!in_bounds(y1, x1)) continue;
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS)
		{
			if (cheat_room)
			{
#ifdef JP
msg_print("警告！地下室のトラップを配置できません！");
#else
				msg_print("Warning! Could not place vault trap!");
#endif

			}
		}

		/* Require "naked" floor grids */
		c_ptr = &cave[y1][x1];
		if (!is_floor_grid(c_ptr) || c_ptr->o_idx || c_ptr->m_idx) continue;

		/* Place the trap */
		place_trap(y1, x1);

		/* Done */
		break;
	}
}


/*
 * Place some traps with a given displacement of given location
 */
void vault_traps(int y, int x, int yd, int xd, int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		vault_trap_aux(y, x, yd, xd);
	}
}


/*
 * Hack -- Place some sleeping monsters near the given location
 */
void vault_monsters(int y1, int x1, int num)
{
	int k, i, y, x;
	cave_type *c_ptr;

	/* Try to summon "num" monsters "near" the given location */
	for (k = 0; k < num; k++)
	{
		/* Try nine locations */
		for (i = 0; i < 9; i++)
		{
			int d = 1;

			/* Pick a nearby location */
			scatter(&y, &x, y1, x1, d, 0);

			/* Require "empty" floor grids */
			c_ptr = &cave[y][x];
			if (!cave_empty_grid(c_ptr)) continue;

			/* Place the monster (allow groups) */
			monster_level = base_level + 2;
			(void)place_monster(y, x, TRUE, TRUE);
			monster_level = base_level;
		}
	}
}


/*
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds(y, x)"
 *
 * We count only granite walls and permanent walls.
 */
int next_to_walls(int y, int x)
{
	int	k = 0;

	if (cave_floor_grid(&cave[y + 1][x])) k++;
	if (cave_floor_grid(&cave[y - 1][x])) k++;
	if (cave_floor_grid(&cave[y][x + 1])) k++;
	if (cave_floor_grid(&cave[y][x - 1])) k++;

	return (k);
}


/*
 * Always picks a correct direction
 */
void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (rand_int(100) < 50)
			*rdir = 0;
		else
			*cdir = 0;
	}
}



/*
 * Pick a random direction
 */
void rand_dir(int *rdir, int *cdir)
{
	/* Pick a random direction */
	int i = rand_int(4);

	/* Extract the dy/dx components */
	*rdir = ddy_ddd[i];
	*cdir = ddx_ddd[i];
}


/* Function that sees if a square is a floor.  (Includes range checking.) */
bool get_is_floor(int x, int y)
{
	if (!in_bounds(y, x))
	{
		/* Out of bounds */
		return (FALSE);
	}

	/* Do the real check */
	if (is_floor_bold(y, x)) return (TRUE);

	return (FALSE);
}


/* Set a square to be floor.  (Includes range checking.) */
void set_floor(int x, int y)
{
	if (!in_bounds(y, x))
	{
		/* Out of bounds */
		return;
	}

	if (cave[y][x].info & CAVE_ROOM)
	{
		/* A room border don't touch. */
		return;
	}

	/* Set to be floor if is a wall (don't touch lakes). */
	if (is_extra_bold(y, x))
		place_floor_bold(y, x);
}



/*
 * Constructs a tunnel between two points
 *
 * This function must be called BEFORE any streamers are created,
 * since we use the special "granite wall" sub-types to keep track
 * of legal places for corridors to pierce rooms.
 *
 * We use "door_flag" to prevent excessive construction of doors
 * along overlapping corridors.
 *
 * We queue the tunnel grids to prevent door creation along a corridor
 * which intersects itself.
 *
 * We queue the wall piercing grids to prevent a corridor from leaving
 * a room and then coming back in through the same entrance.
 *
 * We "pierce" grids which are "outer" walls of rooms, and when we
 * do so, we change all adjacent "outer" walls of rooms into "solid"
 * walls so that no two corridors may use adjacent grids for exits.
 *
 * The "solid" wall check prevents corridors from "chopping" the
 * corners of rooms off, as well as "silly" door placement, and
 * "excessively wide" room entrances.
 *
 * Useful "feat" values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_EXTRA -- shop walls (perma)
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 */
void build_tunnel(int row1, int col1, int row2, int col2)
{
	int y, x;
	int tmp_row, tmp_col;
	int row_dir, col_dir;
	int start_row, start_col;
	int main_loop_count = 0;

	bool door_flag = FALSE;

	cave_type *c_ptr;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2))
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) break;

		/* Allow bends in the tunnel */
		if (rand_int(100) < dun_tun_chg)
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Extremely Important -- do not leave the dungeon */
		while (!in_bounds(tmp_row, tmp_col))
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < dun_tun_rnd)
			{
				rand_dir(&row_dir, &col_dir);
			}

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}


		/* Access the location */
		c_ptr = &cave[tmp_row][tmp_col];


		/* Avoid the edge of the dungeon */
		if (c_ptr->feat == FEAT_PERM_SOLID) continue;

		/* Avoid the edge of vaults */
		if (c_ptr->feat == FEAT_PERM_OUTER) continue;

		/* Avoid "solid" granite walls */
		if (is_solid_grid(c_ptr)) continue;

		/* Pierce "outer" walls of rooms */
		if (is_outer_grid(c_ptr))
		{
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid permanent walls */
			if (cave[y][x].feat == FEAT_PERM_SOLID) continue;
			if (cave[y][x].feat == FEAT_PERM_OUTER) continue;

			/* Hack -- Avoid outer/solid granite walls */
			if (is_outer_bold(y, x)) continue;
			if (is_solid_bold(y, x)) continue;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}

			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++)
			{
				for (x = col1 - 1; x <= col1 + 1; x++)
				{
					/* Convert adjacent "outer" walls as "solid" walls */
					if (is_outer_bold(y, x))
					{
						/* Change the wall to a "solid" wall */
						place_solid_noperm_bold(y, x);
					}
				}
			}
		}

		/* Travel quickly through rooms */
		else if (c_ptr->info & (CAVE_ROOM))
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if (is_extra_grid(c_ptr) || is_inner_grid(c_ptr) || is_solid_grid(c_ptr))
		{
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}

			/* Allow door in next grid */
			door_flag = FALSE;
		}

		/* Handle corridor intersections or overlaps */
		else
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag)
			{
				/* Save the door location */
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (rand_int(100) >= dun_tun_con)
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0) tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0) tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10)) break;
			}
		}
	}
}


/*
 * This routine adds the square to the tunnel
 * It also checks for SOLID walls - and returns a nearby
 * non-SOLID square in (x,y) so that a simple avoiding
 * routine can be used. The returned boolean value reflects
 * whether or not this routine hit a SOLID wall.
 *
 * "affectwall" toggles whether or not this new square affects
 * the boundaries of rooms. - This is used by the catacomb
 * routine.
 */
static bool set_tunnel(int *x, int *y, bool affectwall)
{
	int feat, i, j, dx, dy;

	cave_type *c_ptr = &cave[*y][*x];

	if (!in_bounds(*y, *x)) return TRUE;

	feat = c_ptr->feat;

	if ((feat == FEAT_PERM_OUTER) ||
	    (feat == FEAT_PERM_INNER) ||
	    is_inner_grid(c_ptr))
	{
		/*
		 * Ignore permanent walls - sometimes cannot tunnel around them anyway
		 * so don't try - it just complicates things unnecessarily.
		 */
		return TRUE;
	}

	if (is_extra_bold(*y,*x))
	{
		/* Save the tunnel location */
		if (dun->tunn_n < TUNN_MAX)
		{
				dun->tunn[dun->tunn_n].y = *y;
				dun->tunn[dun->tunn_n].x = *x;
				dun->tunn_n++;
		}

		return TRUE;
	}

	if (is_floor_bold(*y, *x))
	{
		/* Don't do anything */
		return TRUE;
	}

	if (is_outer_grid(c_ptr) && affectwall)
	{
		/* Save the wall location */
		if (dun->wall_n < WALL_MAX)
		{
			dun->wall[dun->wall_n].y = *y;
			dun->wall[dun->wall_n].x = *x;
			dun->wall_n++;
		}

		/* Forbid re-entry near this piercing */
		for (j = *y - 1; j <= *y + 1; j++)
		{
			for (i = *x - 1; i <= *x + 1; i++)
			{
				/* Convert adjacent "outer" walls as "solid" walls */
				if (is_outer_bold(j, i))
				{
					/* Change the wall to a "solid" wall */
					place_solid_noperm_bold(j, i);
				}
			}
		}
		place_floor_bold(*y, *x);

		return TRUE;
	}

	if (is_solid_grid(c_ptr) && affectwall)
	{
		/* cannot place tunnel here - use a square to the side */

		/* find usable square and return value in (x,y) */

		i = 50;

		dy = 0;
		dx = 0;
		while ((i > 0) && is_solid_bold(*y + dy, *x + dx))
		{
			dy = rand_int(3) - 1;
			dx = rand_int(3) - 1;

			if (!in_bounds(*y + dy, *x + dx))
			{
				dx = 0;
				dy = 0;
			}

			i--;
		}

		if (i == 0)
		{
			/* Failed for some reason: hack - ignore the solidness */
			place_outer_grid(c_ptr);
			dx = 0;
			dy = 0;
		}

		/* Give new, acceptable coordinate. */
		*x = *x + dx;
		*y = *y + dy;

		return FALSE;
	}

	return TRUE;
}


/*
 * This routine creates the catacomb-like tunnels by removing extra rock.
 * Note that this routine is only called on "even" squares - so it gives
 * a natural checkerboard pattern.
 */
static void create_cata_tunnel(int x, int y)
{
	int x1, y1;

	/* Build tunnel */
	x1 = x - 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x + 1;
	y1 = y;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y - 1;
	set_tunnel(&x1, &y1, FALSE);

	x1 = x;
	y1 = y + 1;
	set_tunnel(&x1, &y1, FALSE);
}


/*
 * This routine does the bulk of the work in creating the new types of tunnels.
 * It is designed to use very simple algorithms to go from (x1,y1) to (x2,y2)
 * It doesn't need to add any complexity - straight lines are fine.
 * The SOLID walls are avoided by a recursive algorithm which tries random ways
 * around the obstical until it works.  The number of itterations is counted, and it
 * this gets too large the routine exits. This should stop any crashes - but may leave
 * small gaps in the tunnel where there are too many SOLID walls.
 *
 * Type 1 tunnels are extremely simple - straight line from A to B.  This is only used
 * as a part of the dodge SOLID walls algorithm.
 *
 * Type 2 tunnels are made of two straight lines at right angles. When this is used with
 * short line segments it gives the "cavelike" tunnels seen deeper in the dungeon.
 *
 * Type 3 tunnels are made of two straight lines like type 2, but with extra rock removed.
 * This, when used with longer line segments gives the "catacomb-like" tunnels seen near
 * the surface.
 */
static void short_seg_hack(int x1, int y1, int x2, int y2, int type, int count, bool *fail)
{
	int i, x, y;
	int length;

	/* Check for early exit */
	if (!(*fail)) return;

	length = distance(x1, y1, x2, y2);

	count++;

	if ((type == 1) && (length != 0))
	{

		for (i = 0; i <= length; i++)
		{
			x = x1 + i * (x2 - x1) / length;
			y = y1 + i * (y2 - y1) / length;
			if (!set_tunnel(&x, &y, TRUE))
			{
				if (count > 50)
				{
					/* This isn't working - probably have an infinite loop */
					*fail = FALSE;
					return;
				}

				/* solid wall - so try to go around */
				short_seg_hack(x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
				short_seg_hack(x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
			}
		}
	}
	else if ((type == 2) || (type == 3))
	{
		if (x1 < x2)
		{
			for (i = x1; i <= x2; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}
		}
		else
		{
			for (i = x2; i <= x1; i++)
			{
				x = i;
				y = y1;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, i - 1, y1, 1, count, fail);
					short_seg_hack(x, y, i + 1, y1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(i, y1);
				}
			}

		}
		if (y1 < y2)
		{
			for (i = y1; i <= y2; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
		else
		{
			for (i = y2; i <= y1; i++)
			{
				x = x2;
				y = i;
				if (!set_tunnel(&x, &y, TRUE))
				{
					/* solid wall - so try to go around */
					short_seg_hack(x, y, x2, i - 1, 1, count, fail);
					short_seg_hack(x, y, x2, i + 1, 1, count, fail);
				}
				if ((type == 3) && ((x + y) % 2))
				{
					create_cata_tunnel(x2, i);
				}
			}
		}
	}
}


/*
 * This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
 * Permanent rock is ignored in this path finding- sometimes there is no
 * path around anyway -so there will be a crash if we try to find one.
 * This routine is much like the river creation routine in Zangband.
 * It works by dividing a line segment into two.  The segments are divided
 * until they are less than "cutoff" - when the corresponding routine from
 * "short_seg_hack" is called.
 * Note it is VERY important that the "stop if hit another passage" logic
 * stays as is.  Without this the dungeon turns into Swiss Cheese...
 */
bool build_tunnel2(int x1, int y1, int x2, int y2, int type, int cutoff)
{
	int x3, y3, dx, dy;
	int changex, changey;
	int midval;
	int length;
	int i;
	bool retval, firstsuccede;
	cave_type *c_ptr;

	length = distance(x1, y1, x2, y2);

	if (length > cutoff)
	{
		/*
		* Divide path in half and call routine twice.
		 */
		dx = (x2 - x1) / 2;
		dy = (y2 - y1) / 2;

		/* perturbation perpendicular to path */
		changex = (rand_int(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;

		/* perturbation perpendicular to path */
		changey = (rand_int(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;

		/* Work out "mid" ponit */
		x3 = x1 + dx + changex;
		y3 = y1 + dy + changey;

		/* See if in bounds - if not - do not perturb point */
		if (!in_bounds(y3, x3))
		{
			x3 = (x1 + x2) / 2;
			y3 = (y1 + y2) / 2;
		}
		/* cache midvalue */
		c_ptr = &cave[y3][x3];
		midval = cave[y3][x3].feat;
		if (is_solid_grid(c_ptr))
		{
			/* move midpoint a bit to avoid problem. */

			i = 50;

			dy = 0;
			dx = 0;
			while ((i > 0) && is_solid_bold(y3 + dy, x3 + dx))
			{
				dy = rand_int(3) - 1;
				dx = rand_int(3) - 1;
				if (!in_bounds(y3 + dy, x3 + dx))
				{
					dx = 0;
					dy = 0;
				}
				i--;
			}

			if (i == 0)
			{
				/* Failed for some reason: hack - ignore the solidness */
				place_outer_bold(y3, x3);
				dx = 0;
				dy = 0;
			}
			y3 += dy;
			x3 += dx;
			c_ptr = &cave[y3][x3];
			midval = cave[y3][x3].feat;
		}

		if (is_floor_grid(c_ptr))
		{
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				if ((cave[y3][x3].info & CAVE_ROOM) || (randint(100) > 95))
				{
					/* do second half only if works + if have hit a room */
					retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				}
				else
				{
					/* have hit another tunnel - make a set of doors here */
					retval = FALSE;

					/* Save the door location */
					if (dun->door_n < DOOR_MAX)
					{
						dun->door[dun->door_n].y = y3;
						dun->door[dun->door_n].x = x3;
						dun->door_n++;
					}
				}
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		else
		{
			/* tunnel through walls */
			if (build_tunnel2(x1, y1, x3, y3, type, cutoff))
			{
				retval = build_tunnel2(x3, y3, x2, y2, type, cutoff);
				firstsuccede = TRUE;
			}
			else
			{
				/* false- didn't work all the way */
				retval = FALSE;
				firstsuccede = FALSE;
			}
		}
		if (firstsuccede)
		{
			/* only do this if the first half has worked */
			set_tunnel(&x3, &y3, TRUE);
		}
		/* return value calculated above */
		return retval;
	}
	else
	{
		/* Do a short segment */
		retval = TRUE;
		short_seg_hack(x1, y1, x2, y2, type, 0, &retval);

		/* Hack - ignore return value so avoid infinite loops */
		return TRUE;
	}
}
