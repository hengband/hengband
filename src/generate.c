/* File: generate.c */

/* Purpose: Dungeon generation */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/*
 * Note that Level generation is *not* an important bottleneck,
 * though it can be annoyingly slow on older machines...  Thus
 * we emphasize "simplicity" and "correctness" over "speed".
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * Consider the "v_info.txt" file for vault generation.
 *
 * In this file, we use the "special" granite and perma-wall sub-types,
 * where "basic" is normal, "inner" is inside a room, "outer" is the
 * outer wall of a room, and "solid" is the outer wall of the dungeon
 * or any walls that may not be pierced by corridors.  Thus the only
 * wall type that may be pierced by a corridor is the "outer granite"
 * type.  The "basic granite" type yields the "actual" corridors.
 *
 * Note that we use the special "solid" granite wall type to prevent
 * multiple corridors from piercing a wall in two adjacent locations,
 * which would be messy, and we use the special "outer" granite wall
 * to indicate which walls "surround" rooms, and may thus be "pierced"
 * by corridors entering or leaving the room.
 *
 * Note that a tunnel which attempts to leave a room near the "edge"
 * of the dungeon in a direction toward that edge will cause "silly"
 * wall piercings, but will have no permanently incorrect effects,
 * as long as the tunnel can *eventually* exit from another side.
 * And note that the wall may not come back into the room by the
 * hole it left through, so it must bend to the left or right and
 * then optionally re-enter the room (at least 2 grids away).  This
 * is not a problem since every room that is large enough to block
 * the passage of tunnels is also large enough to allow the tunnel
 * to pierce the room itself several times.
 *
 * Note that no two corridors may enter a room through adjacent grids,
 * they must either share an entryway or else use entryways at least
 * two grids apart.  This prevents "large" (or "silly") doorways.
 *
 * To create rooms in the dungeon, we first divide the dungeon up
 * into "blocks" of 11x11 grids each, and require that all rooms
 * occupy a rectangular group of blocks.  As long as each room type
 * reserves a sufficient number of blocks, the room building routines
 * will not need to check bounds.  Note that most of the normal rooms
 * actually only use 23x11 grids, and so reserve 33x11 grids.
 *
 * Note that the use of 11x11 blocks (instead of the old 33x11 blocks)
 * allows more variability in the horizontal placement of rooms, and
 * at the same time has the disadvantage that some rooms (two thirds
 * of the normal rooms) may be "split" by panel boundaries.  This can
 * induce a situation where a player is in a room and part of the room
 * is off the screen.  It may be annoying enough to go back to 33x11
 * blocks to prevent this visual situation.
 *
 * Note that the dungeon generation routines are much different (2.7.5)
 * and perhaps "DUN_ROOMS" should be less than 50.
 *
 * XXX XXX XXX Note that it is possible to create a room which is only
 * connected to itself, because the "tunnel generation" code allows a
 * tunnel to leave a room, wander around, and then re-enter the room.
 *
 * XXX XXX XXX Note that it is possible to create a set of rooms which
 * are only connected to other rooms in that set, since there is nothing
 * explicit in the code to prevent this from happening.  But this is less
 * likely than the "isolated room" problem, because each room attempts to
 * connect to another room, in a giant cycle, thus requiring at least two
 * bizarre occurances to create an isolated section of the dungeon.
 *
 * Note that (2.7.9) monster pits have been split into monster "nests"
 * and monster "pits".  The "nests" have a collection of monsters of a
 * given type strewn randomly around the room (jelly, animal, or undead),
 * while the "pits" have a collection of monsters of a given type placed
 * around the room in an organized manner (orc, troll, giant, dragon, or
 * demon).  Note that both "nests" and "pits" are now "level dependant",
 * and both make 16 "expensive" calls to the "get_mon_num()" function.
 *
 * Note that the cave grid flags changed in a rather drastic manner
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain
 * features, such as doors and stairs and traps and rubble and walls,
 * are all handled as a set of 64 possible "terrain features", and
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.
 *
 * The 64 new "dungeon features" will also be used for "visual display"
 * but we must be careful not to allow, for example, the user to display
 * hidden traps in a different way from floors, or secret doors in a way
 * different from granite walls, or even permanent granite in a different
 * way from granite.  XXX XXX XXX
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"
#include "rooms.h"
#include "streams.h"

int dun_rooms;

int dun_tun_rnd;
int dun_tun_chg;
int dun_tun_con;
int dun_tun_pen;
int dun_tun_jct;


/*
 * Dungeon generation data -- see "cave_gen()"
 */
dun_data *dun;

/*
 * Places some staircases near walls
 */
static bool alloc_stairs(int feat, int num, int walls)
{
	int         y, x, i, j, flag;
	int         more_num = 0;
	cave_type   *c_ptr;

	if (feat == FEAT_LESS)
	{
		/* No up stairs in town or in ironman mode */
		if (ironman_downward || !dun_level) return TRUE;

		if (dun_level > d_info[dungeon_type].mindepth)
			more_num = (randint1(num+1))/2;
	}
	else if (feat == FEAT_MORE)
	{
		/* No downstairs on quest levels */
		if ((dun_level > 1) && quest_number(dun_level)) return TRUE;

		/* No downstairs at the bottom */
		if (dun_level >= d_info[dungeon_type].maxdepth) return TRUE;

		if ((dun_level < d_info[dungeon_type].maxdepth-1) && !quest_number(dun_level+1))
			more_num = (randint1(num)+1)/2;
	}

	/* Place "num" stairs */
	for (i = 0; i < num; i++)
	{
		/* Place some stairs */
		for (flag = FALSE; !flag; )
		{
			/* Try several times, then decrease "walls" */
			for (j = 0; !flag && j <= 3000; j++)
			{
				/* Pick a random grid */
				y = randint1(cur_hgt-2);
				x = randint1(cur_wid-2);

				/* Access the grid */
				c_ptr = &cave[y][x];

				/* Require "naked" floor grid */
				if (!is_floor_grid(c_ptr) || pattern_tile(y,x) || c_ptr->o_idx || c_ptr->m_idx) continue;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(y, x) < walls) continue;

				/* Clear previous contents, add stairs */
				if (i < more_num) c_ptr->feat = feat+0x07;
				else c_ptr->feat = feat;

				/* All done */
				flag = TRUE;
			}

			if (!flag) return FALSE;
			/* Require fewer walls */
			if (walls) walls--;
		}
	}
	return TRUE;
}


/*
 * Allocates some objects (using "place" and "type")
 */
static void alloc_object(int set, int typ, int num)
{
	int y = 0, x = 0, k;
	int dummy = 0;
	cave_type *c_ptr;

	/* Place some objects */
	for (k = 0; k < num; k++)
	{
		/* Pick a "legal" spot */
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			bool room;

			dummy++;

			/* Location */
			y = randint0(cur_hgt);
			x = randint0(cur_wid);

			c_ptr = &cave[y][x];

			/* Require "naked" floor grid */
			if (!is_floor_grid(c_ptr) || c_ptr->o_idx || c_ptr->m_idx) continue;

			/* Check for "room" */
			room = (cave[y][x].info & CAVE_ROOM) ? TRUE : FALSE;

			/* Require corridor? */
			if ((set == ALLOC_SET_CORR) && room) continue;

			/* Require room? */
			if ((set == ALLOC_SET_ROOM) && !room) continue;

			/* Accept it */
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS)
		{
			if (cheat_room)
			{
#ifdef JP
msg_print("警告！アイテムを配置できません！");
#else
				msg_print("Warning! Could not place object!");
#endif

			}
			return;
		}


		/* Place something */
		switch (typ)
		{
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(y, x);
				cave[y][x].info &= ~(CAVE_FLOOR);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(y, x);
				cave[y][x].info &= ~(CAVE_FLOOR);
				break;
			}

			case ALLOC_TYP_GOLD:
			{
				place_gold(y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(y, x, FALSE, FALSE);
				break;
			}
		}
	}
}


/*
 * Count the number of "corridor" grids adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds(y1, x1)"
 *
 * XXX XXX This routine currently only counts actual "empty floor"
 * grids which are not in rooms.  We might want to also count stairs,
 * open doors, closed doors, etc.
 */
static int next_to_corr(int y1, int x1)
{
	int i, y, x, k = 0;

	cave_type *c_ptr;

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Access the grid */
		c_ptr = &cave[y][x];

		/* Skip non floors */
		if (!cave_floor_grid(c_ptr)) continue;

		/* Skip non "empty floor" grids */
		if (!is_floor_grid(c_ptr))
                        continue;

		/* Skip grids inside rooms */
		if (c_ptr->info & (CAVE_ROOM)) continue;

		/* Count these grids */
		k++;
	}

	/* Return the number of corridors */
	return (k);
}


/*
 * Determine if the given location is "between" two walls,
 * and "next to" two corridor spaces.  XXX XXX XXX
 *
 * Assumes "in_bounds(y, x)"
 */
static bool possible_doorway(int y, int x)
{
	/* Count the adjacent corridors */
	if (next_to_corr(y, x) >= 2)
	{
		/* Check Vertical */
		if ((cave[y-1][x].feat >= FEAT_MAGMA) &&
		    (cave[y+1][x].feat >= FEAT_MAGMA))
		{
			return (TRUE);
		}

		/* Check Horizontal */
		if ((cave[y][x-1].feat >= FEAT_MAGMA) &&
		    (cave[y][x+1].feat >= FEAT_MAGMA))
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}


/*
 * Places door at y, x position if at least 2 walls found
 */
static void try_door(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Ignore walls */
	if (cave[y][x].feat >= FEAT_MAGMA) return;

	/* Ignore room grids */
	if (cave[y][x].info & (CAVE_ROOM)) return;

	/* Occasional door (if allowed) */
	if ((randint0(100) < dun_tun_jct) && possible_doorway(y, x) && !(d_info[dungeon_type].flags1 & DF1_NO_DOORS))
	{
		/* Place a door */
		place_random_door(y, x);
	}
}



/*
 * Generate a new dungeon level
 *
 * Note that "dun_body" adds about 4000 bytes of memory to the stack.
 */
static bool cave_gen(void)
{
	int i, j, k, y, x, y1, x1;

	int max_vault_ok = 2;

	int feat1 = 0, feat2 = 0;

	cave_type *c_ptr;

	bool destroyed = FALSE;
	bool empty_level = FALSE;
	bool cavern = FALSE;
	int laketype = 0;


	dun_data dun_body;

        /* Fill the arrays of floors and walls in the good proportions */
        for (i = 0; i < 100; i++)
        {
                int lim1, lim2, lim3;

                lim1 = d_info[dungeon_type].floor_percent1;
                lim2 = lim1 + d_info[dungeon_type].floor_percent2;
                lim3 = lim2 + d_info[dungeon_type].floor_percent3;

		if (i < lim1)
			floor_type[i] = d_info[dungeon_type].floor1;
		else if (i < lim2)
			floor_type[i] = d_info[dungeon_type].floor2;
		else if (i < lim3)
			floor_type[i] = d_info[dungeon_type].floor3;

		lim1 = d_info[dungeon_type].fill_percent1;
		lim2 = lim1 + d_info[dungeon_type].fill_percent2;
		lim3 = lim2 + d_info[dungeon_type].fill_percent3;
		if (i < lim1)
			fill_type[i] = d_info[dungeon_type].fill_type1;
		else if (i < lim2)
			fill_type[i] = d_info[dungeon_type].fill_type2;
		else if (i < lim3)
			fill_type[i] = d_info[dungeon_type].fill_type3;
        }

	/* Prepare allocation table */
	get_mon_num_prep(get_monster_hook(), NULL);

        feat_wall_outer = d_info[dungeon_type].outer_wall;
        feat_wall_inner = d_info[dungeon_type].inner_wall;
        feat_wall_solid = d_info[dungeon_type].outer_wall;

	/* Global data */
	dun = &dun_body;

	if (cur_hgt <= SCREEN_HGT / 2 - 2) max_vault_ok--;
	if (cur_wid <= SCREEN_WID / 2 - 2) max_vault_ok--;

	/* Randomize the dungeon creation values */
	dun_rooms = rand_range(DUN_ROOMS_MIN, DUN_ROOMS_MAX);
	dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
	dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
	dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
	dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
	dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);

	/* Empty arena levels */
	if (ironman_empty_levels || ((d_info[dungeon_type].flags1 & DF1_ARENA) && (empty_levels && one_in_(EMPTY_LEVEL))))
	{
		empty_level = TRUE;

		if (cheat_room)
#ifdef JP
msg_print("アリーナレベル");
#else
			msg_print("Arena level.");
#endif

	}


	/* Hack -- Start with basic granite */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			if (empty_level)
			{
				place_floor_grid(c_ptr);
			}
			else
			{
			  /* Create granite wall */
				place_extra_grid(c_ptr);
			}
		}
	}

#ifdef ALLOW_CAVERNS_AND_LAKES
	/* Possible "destroyed" level */
	if ((dun_level > 30) && one_in_(DUN_DEST*2) && (small_levels) && (d_info[dungeon_type].flags1 & DF1_DESTROY))
	{
		destroyed = TRUE;

		/* extra rubble around the place looks cool */
		build_lake(3+randint0(2));
	}

	/* Make a lake some of the time */
	if (one_in_(LAKE_LEVEL) && !empty_level && !destroyed &&
	    (d_info[dungeon_type].flags1 & DF1_LAKE_MASK))
	{
		int count = 0;
		if (d_info[dungeon_type].flags1 & DF1_LAKE_WATER) count += 3;
		if (d_info[dungeon_type].flags1 & DF1_LAKE_LAVA) count += 3;
		if (d_info[dungeon_type].flags1 & DF1_LAKE_RUBBLE) count += 3;
		if (d_info[dungeon_type].flags1 & DF1_LAKE_TREE) count += 3;

		if (d_info[dungeon_type].flags1 & DF1_LAKE_LAVA)
		{
			/* Lake of Lava */
			if ((dun_level > 80) && (randint0(count) < 2)) laketype = 1;
			count -= 2;

			/* Lake of Lava2 */
			if (!laketype && (dun_level > 80) && one_in_(count)) laketype = 7;
			count --;
		}

		if ((d_info[dungeon_type].flags1 & DF1_LAKE_WATER) && !laketype)
		{
			/* Lake of Water */
			if ((dun_level > 50) && randint0(count) < 2) laketype = 2;
			count -= 2;

			/* Lake of Water2 */
			if (!laketype && (dun_level > 50) && one_in_(count)) laketype = 6;
			count --;
		}

		if ((d_info[dungeon_type].flags1 & DF1_LAKE_RUBBLE) && !laketype)
		{
			/* Lake of rubble */
			if ((dun_level > 35) && (randint0(count) < 2)) laketype = 3;
			count -= 2;

			/* Lake of rubble2 */
			if (!laketype && (dun_level > 35) && one_in_(count)) laketype = 4;
			count --;
		}

		/* Lake of tree */
		if ((dun_level > 5) && (d_info[dungeon_type].flags1 & DF1_LAKE_TREE) && !laketype) laketype = 5;

		if (laketype != 0)
		{
			if (cheat_room)
#ifdef JP
msg_print("湖を生成。");
#else
				msg_print("Lake on the level.");
#endif

			build_lake(laketype);
		}
	}

	if ((dun_level > DUN_CAVERN) && !empty_level &&
	    (d_info[dungeon_type].flags1 & DF1_CAVERN) &&
	    (laketype == 0) && !destroyed && (randint1(1000) < dun_level))
	{
		cavern = TRUE;

		/* make a large fractal cave in the middle of the dungeon */

		if (cheat_room)
#ifdef JP
msg_print("洞窟を生成。");
#else
			msg_print("Cavern on level.");
#endif


		build_cavern();
	}
#endif /* ALLOW_CAVERNS_AND_LAKES */

	/* Hack -- No destroyed "quest" levels */
	if (quest_number(dun_level)) destroyed = FALSE;

	/* Actual maximum number of rooms on this level */
	dun->row_rooms = cur_hgt / BLOCK_HGT;
	dun->col_rooms = cur_wid / BLOCK_WID;

	/* Initialize the room table */
	for (y = 0; y < dun->row_rooms; y++)
	{
		for (x = 0; x < dun->col_rooms; x++)
		{
			dun->room_map[y][x] = FALSE;
		}
	}


	/* No "crowded" rooms yet */
	dun->crowded = 0;


	/* No rooms yet */
	dun->cent_n = 0;

	
	/* Build some rooms */
	if (!(d_info[dungeon_type].flags1 & DF1_MAZE))
	{
	for (i = 0; i < dun_rooms; i++)
	{
		bool force_rooms = (ironman_rooms && !((d_info[dungeon_type].flags1 & DF1_BEGINNER) || (d_info[dungeon_type].flags1 & DF1_CHAMELEON)));

		/* Pick a block for the room */
		y = randint0(dun->row_rooms);
		x = randint0(dun->col_rooms);

		/* Align dungeon rooms */
		if (d_info[dungeon_type].flags1 & DF1_NO_CAVE)
		{
			/* Slide some rooms right */
			if ((x % 3) == 0) x++;

			/* Slide some rooms left */
			if ((x % 3) == 2) x--;
		}

		/* Attempt an "unusual" room */
		if (force_rooms || (randint0(DUN_UNUSUAL) < dun_level))
		{
			/* Roll for room type */
			while(1)
			{
				k = (force_rooms ? 0 : randint0(100));
				if (force_rooms) break;
				if ((d_info[dungeon_type].flags1 & DF1_NO_VAULT) && (k < 14)) continue;
				break;
			}

			/* Attempt a very unusual room */
			if (force_rooms || (randint0(DUN_UNUSUAL) < dun_level))
			{
#ifdef FORCE_V_IDX
				if (room_build(y, x, 8)) continue;
#else
				/* Type 8 -- Greater vault (4%) */
				if (k < 4)
				{
					if (max_vault_ok > 1)
					{
						if (room_build(y, x, 8)) continue;
					}
					else
					{
#ifdef JP
if (cheat_room) msg_print("巨大な地下室を却下します。");
#else
						if (cheat_room) msg_print("Refusing a greater vault.");
#endif

					}
				}

				/* Type 7 -- Lesser vault (6%) */
				if (k < 10)
				{
					if (max_vault_ok > 0)
					{
						if (room_build(y, x, 7)) continue;
					}
					else
					{
#ifdef JP
if (cheat_room) msg_print("小さな地下室を却下します。");
#else
						if (cheat_room) msg_print("Refusing a lesser vault.");
#endif

					}
				}


				/* Type 10 -- Random vault (4%) */
				if ((k < 14) && room_build(y, x, 10)) continue;

				/* Type 5 -- Monster nest (8%) */
				if ((k < 22) && room_build(y, x, 5)) continue;

				/* Type 6 -- Monster pit (10%) */
				if ((k < 32) && room_build(y, x, 6)) continue;
#endif

			}

			/* Type 2 -- Overlapping (25%) */
			if ((k < 25) && room_build(y, x, 2)) continue;

			/* Type 3 -- Cross room (25%) */
			if ((k < 50) && room_build(y, x, 3)) continue;

			if (d_info[dungeon_type].flags1 & DF1_NO_CAVE)
			{
				if (room_build(y, x, 4)) continue;
			}
			else
			{
				/* Type 4 -- Large room (25%) */
				if ((k < 75) && room_build(y, x, 4)) continue;

				/* Type 11 -- Circular (10%) */
				if ((k < 85) && room_build(y, x, 11)) continue;

				/* Type 12 -- Crypt (15%) */
				if ((k < 100) && room_build(y, x, 12)) continue;
			}
		}

		/* The deeper you are, the more cavelike the rooms are */
		k = randint1(100);

		/* No caves when a cavern exists: they look bad */
		if (((k < dun_level) || (d_info[dungeon_type].flags1 & DF1_CAVE)) && (!cavern) && (!empty_level) && (laketype == 0) && !(d_info[dungeon_type].flags1 & DF1_NO_CAVE))
		{
			/* Type 9 -- Fractal cave */
			if (room_build(y, x, 9)) continue;
		}
		else
			/* Attempt a "trivial" room */

		  if (room_build(y, x, 1)) continue;
		continue;
	}

	/* Make a hole in the dungeon roof sometimes at level 1 */
	if ((dun_level == 1) && terrain_streams)
	{
		while (one_in_(DUN_MOS_DEN))
		{
			place_trees(randint1(cur_wid - 2), randint1(cur_hgt - 2));
		}
	}

	/* Destroy the level if necessary */
	if (destroyed) destroy_level();

	/* Hack -- Add some rivers */
	if (one_in_(3) && (randint1(dun_level) > 5) && terrain_streams)
	{
	 	/* Choose water or lava */
		if ((randint1(MAX_DEPTH * 2) - 1 > dun_level) && (d_info[dungeon_type].flags1 & DF1_WATER_RIVER))
		{
			feat1 = FEAT_DEEP_WATER;
			feat2 = FEAT_SHAL_WATER;
		}
		else if  (d_info[dungeon_type].flags1 & DF1_LAVA_RIVER)
		{
			feat1 = FEAT_DEEP_LAVA;
			feat2 = FEAT_SHAL_LAVA;
		}
		else feat1 = 0;


	 	/* Only add river if matches lake type or if have no lake at all */
	 	if ((((laketype == 1) && (feat1 == FEAT_DEEP_LAVA)) ||
	 	    ((laketype == 2) && (feat1 == FEAT_DEEP_WATER)) ||
		     (laketype == 0)) && feat1)
	 	{
			add_river(feat1, feat2);
		}
	}
	}

	/* Special boundary walls -- Top */
	for (x = 0; x < cur_wid; x++)
	{
		cave_type *c_ptr = &cave[0][x];

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
		c_ptr->info &= ~(CAVE_MASK);
	}

	/* Special boundary walls -- Bottom */
	for (x = 0; x < cur_wid; x++)
	{
		cave_type *c_ptr = &cave[cur_hgt-1][x];

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
		c_ptr->info &= ~(CAVE_MASK);
	}

	/* Special boundary walls -- Left */
	for (y = 0; y < cur_hgt; y++)
	{
		cave_type *c_ptr = &cave[y][0];

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
		c_ptr->info &= ~(CAVE_MASK);
	}

	/* Special boundary walls -- Right */
	for (y = 0; y < cur_hgt; y++)
	{
		cave_type *c_ptr = &cave[y][cur_wid-1];

		/* Clear previous contents, add "solid" perma-wall */
		c_ptr->feat = FEAT_PERM_SOLID;
		c_ptr->info &= ~(CAVE_MASK);
	}


	if (d_info[dungeon_type].flags1 & DF1_MAZE)
	{
		build_maze_vault(cur_wid/2-1, cur_hgt/2-1, cur_wid-4, cur_hgt-4, FALSE);

		/* Place 3 or 4 down stairs near some walls */
		if (!alloc_stairs(FEAT_MORE, rand_range(2, 3), 3)) return FALSE;

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(FEAT_LESS, 1, 3)) return FALSE;
	}
	else
	{
	/* Hack -- Scramble the room order */
	for (i = 0; i < dun->cent_n; i++)
	{
		int pick1 = randint0(dun->cent_n);
		int pick2 = randint0(dun->cent_n);
		y1 = dun->cent[pick1].y;
		x1 = dun->cent[pick1].x;
		dun->cent[pick1].y = dun->cent[pick2].y;
		dun->cent[pick1].x = dun->cent[pick2].x;
		dun->cent[pick2].y = y1;
		dun->cent[pick2].x = x1;
	}

	/* Start with no tunnel doors */
	dun->door_n = 0;

	/* Hack -- connect the first room to the last room */
	y = dun->cent[dun->cent_n-1].y;
	x = dun->cent[dun->cent_n-1].x;

	/* Connect all the rooms together */
	for (i = 0; i < dun->cent_n; i++)
	{

		/* Reset the arrays */
		dun->tunn_n = 0;
		dun->wall_n = 0;

		/* Connect the room to the previous room */
		if (randint1(dun_level) > d_info[dungeon_type].tunnel_percent)
		{
			/* make cave-like tunnel */
			build_tunnel2(dun->cent[i].x, dun->cent[i].y, x, y, 2, 2);
		}
		else
		{
			/* make normal tunnel */
			build_tunnel(dun->cent[i].y, dun->cent[i].x, y, x);
		}

		/* Turn the tunnel into corridor */
		for (j = 0; j < dun->tunn_n; j++)
		{
			/* Access the grid */
			y = dun->tunn[j].y;
			x = dun->tunn[j].x;

			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Clear previous contents (if not a lake), add a floor */
			if ((c_ptr->feat < FEAT_DEEP_WATER) ||
			    (c_ptr->feat > FEAT_SHAL_LAVA))
			{
				place_floor_grid(c_ptr);
			}
		}

		/* Apply the piercings that we found */
		for (j = 0; j < dun->wall_n; j++)
		{
			/* Access the grid */
			y = dun->wall[j].y;
			x = dun->wall[j].x;

			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Clear previous contents, add up floor */
			place_floor_grid(c_ptr);

			/* Occasional doorway */
			if ((randint0(100) < dun_tun_pen) && !(d_info[dungeon_type].flags1 & DF1_NO_DOORS))
			{
				/* Place a random door */
				place_random_door(y, x);
			}
		}

		/* Remember the "previous" room */
		y = dun->cent[i].y;
		x = dun->cent[i].x;
	}

	/* Place intersection doors	 */
	for (i = 0; i < dun->door_n; i++)
	{
		/* Extract junction location */
		y = dun->door[i].y;
		x = dun->door[i].x;

		/* Try placing doors */
		try_door(y, x - 1);
		try_door(y, x + 1);
		try_door(y - 1, x);
		try_door(y + 1, x);
	}

	/* Place 3 or 4 down stairs near some walls */
	if (!alloc_stairs(FEAT_MORE, rand_range(3, 4), 3)) return FALSE;

	/* Place 1 or 2 up stairs near some walls */
	if (!alloc_stairs(FEAT_LESS, rand_range(1, 2), 3)) return FALSE;

	}

	if (!laketype)
	{
		if (d_info[dungeon_type].stream1)
		{
			/* Hack -- Add some magma streamers */
			for (i = 0; i < DUN_STR_MAG; i++)
			{
				build_streamer(d_info[dungeon_type].stream1, DUN_STR_MC);
			}
		}

		if (d_info[dungeon_type].stream2)
		{
			/* Hack -- Add some quartz streamers */
			for (i = 0; i < DUN_STR_QUA; i++)
			{
				build_streamer(d_info[dungeon_type].stream2, DUN_STR_QC);
			}
		}
	}

	/* Handle the quest monster placements */
	for (i = 0; i < max_quests; i++)
	{
		if ((quest[i].status == QUEST_STATUS_TAKEN) &&
		    ((quest[i].type == QUEST_TYPE_KILL_LEVEL) ||
		    (quest[i].type == QUEST_TYPE_RANDOM)) &&
		    (quest[i].level == dun_level) && (dungeon_type == quest[i].dungeon) &&
			!(quest[i].flags & QUEST_FLAG_PRESET))
		{
			monster_race *r_ptr = &r_info[quest[i].r_idx];

			/* Hack -- "unique" monsters must be "unique" */
			if ((r_ptr->flags1 & RF1_UNIQUE) &&
			    (r_ptr->cur_num >= r_ptr->max_num))
			{
				/* The unique is already dead */
				quest[i].status = QUEST_STATUS_FINISHED;
			}
			else
			{
				u32b mode = (PM_NO_KAGE | PM_NO_PET);

				for (j = 0; j < (quest[i].max_num - quest[i].cur_num); j++)
				{
					for (k = 0; k < SAFE_MAX_ATTEMPTS; k++)
					{
						/* Find an empty grid */
						while (TRUE)
						{
							y = randint0(cur_hgt);
							x = randint0(cur_wid);

							/* Access the grid */
							c_ptr = &cave[y][x];

							if (!is_floor_grid(c_ptr) || c_ptr->o_idx || c_ptr->m_idx) continue;
							if (distance(y, x, py, px) < 10) continue;
							else break;
						}

						if (!(r_ptr->flags1 & RF1_FRIENDS))
							mode |= PM_ALLOW_GROUP;

						/* Try to place the monster */
						if (place_monster_aux(0, y, x, quest[i].r_idx, mode))
						{
							/* Success */
							break;
						}
						else
						{
							/* Failure - Try again */
							continue;
						}
					}
				}
			}
		}
	}


	/* Basic "amount" */
	k = (dun_level / 3);
	if (k > 10) k = 10;
	if (k < 2) k = 2;

	/* Pick a base number of monsters */
	i = d_info[dungeon_type].min_m_alloc_level;

	/* To make small levels a bit more playable */
	if (cur_hgt < MAX_HGT || cur_wid < MAX_WID)
	{
		int small_tester = i;

		i = (i * cur_hgt) / MAX_HGT;
		i = (i * cur_wid) / MAX_WID;
		i += 1;

		if (i > small_tester) i = small_tester;
		else if (cheat_hear)
		{
#ifdef JP
msg_format("モンスター数基本値を %d から %d に減らします", small_tester, i);
#else
			msg_format("Reduced monsters base from %d to %d", small_tester, i);
#endif

		}
	}

	i += randint1(8);

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
	{
		(void)alloc_monster(0, TRUE);
	}

	/* Place some traps in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

	/* Put some rubble in corridors */
	alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

	/* Put some objects in rooms */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));

        /* Put an Artifact and Artifact Guardian is requested */
        if(d_info[dungeon_type].final_guardian && (d_info[dungeon_type].maxdepth == dun_level))
        {
                int oy;
                int ox;
                int try = 4000;

                /* Find a good position */
                while(try)
                {
                        /* Get a random spot */
                        oy = randint1(cur_hgt - 4) + 2;
                        ox = randint1(cur_wid - 4) + 2;

                        /* Is it a good spot ? */
                        if (cave_empty_bold2(oy, ox) && monster_can_cross_terrain(cave[oy][ox].feat, &r_info[d_info[dungeon_type].final_guardian]))
			{
				/* Place the guardian */
				if (place_monster_aux(0, oy, ox, d_info[dungeon_type].final_guardian, (PM_ALLOW_GROUP | PM_NO_KAGE | PM_NO_PET))) break;
			}
                        /* One less try */
                        try--;
                }
	}

	if ((empty_level && (!one_in_(DARK_EMPTY) || (randint1(100) > dun_level))) && !(d_info[dungeon_type].flags1 & DF1_DARKNESS))
	{
		/* Lite the cave */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				cave[y][x].info |= (CAVE_GLOW);
			}
		}
	}

	/* Determine the character location */
	if (!new_player_spot())
		return FALSE;

	return TRUE;
}


/*
 * Builds the arena after it is entered -KMW-
 */
static void build_arena(void)
{
	int yval, y_height, y_depth, xval, x_left, x_right;
	register int i, j;

	yval = SCREEN_HGT / 2;
	xval = SCREEN_WID / 2;
	y_height = yval - 10;
	y_depth = yval + 10;
	x_left = xval - 32;
	x_right = xval + 32;

	for (i = y_height; i <= y_height + 5; i++)
		for (j = x_left; j <= x_right; j++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 5; i--)
		for (j = x_left; j <= x_right; j++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	cave[y_height+6][x_left+18].feat = FEAT_PERM_EXTRA;
	cave[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_depth-6][x_left+18].feat = FEAT_PERM_EXTRA;
	cave[y_depth-6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_height+6][x_right-18].feat = FEAT_PERM_EXTRA;
	cave[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_depth-6][x_right-18].feat = FEAT_PERM_EXTRA;
	cave[y_depth-6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	i = y_height + 5;
	j = xval;
	cave[i][j].feat = FEAT_BLDG_HEAD + 2;
	cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i + 1, j);
}


/*
 * Town logic flow for generation of arena -KMW-
 */
static void arena_gen(void)
{
	int y, x;
	int qy = 0;
	int qx = 0;

	/* Smallest area */
	cur_hgt = SCREEN_HGT;
	cur_wid = SCREEN_WID;

	/* Start with solid walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			cave[y][x].feat = FEAT_PERM_SOLID;

			/* Illuminate and memorize the walls */
			cave[y][x].info |= (CAVE_GLOW | CAVE_MARK);
		}
	}

	/* Then place some floors */
	for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
	{
		for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
		{
			/* Create empty floor */
			cave[y][x].feat = FEAT_FLOOR;
		}
	}

	build_arena();

	place_monster_aux(0, py + 5, px, arena_monsters[p_ptr->arena_number],
	    (PM_NO_KAGE | PM_NO_PET));
}



/*
 * Builds the arena after it is entered -KMW-
 */
static void build_battle(void)
{
	int yval, y_height, y_depth, xval, x_left, x_right;
	register int i, j;

	yval = SCREEN_HGT / 2;
	xval = SCREEN_WID / 2;
	y_height = yval - 10;
	y_depth = yval + 10;
	x_left = xval - 32;
	x_right = xval + 32;

	for (i = y_height; i <= y_height + 5; i++)
		for (j = x_left; j <= x_right; j++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 3; i--)
		for (j = x_left; j <= x_right; j++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			cave[i][j].feat = FEAT_PERM_EXTRA;
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	cave[y_height+6][x_left+18].feat = FEAT_PERM_EXTRA;
	cave[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_depth-4][x_left+18].feat = FEAT_PERM_EXTRA;
	cave[y_depth-4][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_height+6][x_right-18].feat = FEAT_PERM_EXTRA;
	cave[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	cave[y_depth-4][x_right-18].feat = FEAT_PERM_EXTRA;
	cave[y_depth-4][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	i = y_height + 4;
	j = xval;
	cave[i][j].feat = FEAT_BLDG_HEAD + 3;
	cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i, j);
}


/*
 * Town logic flow for generation of arena -KMW-
 */
static void battle_gen(void)
{
	int y, x, i;
	int qy = 0;
	int qx = 0;

	/* Start with solid walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			cave[y][x].feat = FEAT_PERM_SOLID;

			/* Illuminate and memorize the walls */
			cave[y][x].info |= (CAVE_GLOW | CAVE_MARK);
		}
	}

	/* Then place some floors */
	for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
	{
		for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
		{
			/* Create empty floor */
			cave[y][x].feat = FEAT_FLOOR;
		}
	}

	build_battle();

	for(i=0;i<4;i++)
	{
		place_monster_aux(0, py + 5 + (i/2)*4, px - 2 + (i%2)*4, battle_mon[i],
				  (PM_NO_KAGE | PM_NO_PET));
		set_friendly(&m_list[cave[py+5+(i/2)*4][px-2+(i%2)*4].m_idx]);
	}
	for(i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		if (!m_ptr->r_idx) continue;

		/* Hack -- Detect monster */
		m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}
}


/*
 * Generate a quest level
 */
static void quest_gen(void)
{
	int x, y;


	/* Start with perm walls */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave[y][x].feat = FEAT_PERM_SOLID;
		}
	}

	/* Set the quest level */
	base_level = quest[p_ptr->inside_quest].level;
	dun_level = base_level;
	object_level = base_level;
	monster_level = base_level;

	if (record_stair) do_cmd_write_nikki(NIKKI_TO_QUEST, p_ptr->inside_quest, NULL);

	/* Prepare allocation table */
	get_mon_num_prep(get_monster_hook(), NULL);

	init_flags = INIT_CREATE_DUNGEON | INIT_ASSIGN;

process_dungeon_file("q_info_j.txt", 0, 0, MAX_HGT, MAX_WID);
}

/* Make a real level */
static bool level_gen(cptr *why)
{
	int level_height, level_width;

	if ((always_small_levels || ironman_small_levels ||
	    (one_in_(SMALL_LEVEL) && small_levels) ||
	     (d_info[dungeon_type].flags1 & DF1_BEGINNER) ||
	    (d_info[dungeon_type].flags1 & DF1_SMALLEST)) &&
	    !(d_info[dungeon_type].flags1 & DF1_BIG))
	{
		if (cheat_room)
#ifdef JP
msg_print("小さなフロア");
#else
		  msg_print("A 'small' dungeon level.");
#endif


		if (d_info[dungeon_type].flags1 & DF1_SMALLEST)
		{
			level_height = 1;
			level_width = 1;
		}
		else if (d_info[dungeon_type].flags1 & DF1_BEGINNER)
		{
			level_height = 2;
			level_width = 2;
		}
		else
		{
			do
			{
				level_height = randint1(MAX_HGT/SCREEN_HGT);
				level_width = randint1(MAX_WID/SCREEN_WID);
			}
			while ((level_height == MAX_HGT/SCREEN_HGT) &&
				   (level_width == MAX_WID/SCREEN_WID));
		}

		cur_hgt = level_height * SCREEN_HGT;
		cur_wid = level_width * SCREEN_WID;

		/* Assume illegal panel */
		panel_row_min = cur_hgt;
		panel_col_min = cur_wid;

		if (cheat_room)
		  msg_format("X:%d, Y:%d.", cur_hgt, cur_wid);
	}
	else
	{
		/* Big dungeon */
		cur_hgt = MAX_HGT;
		cur_wid = MAX_WID;

		/* Assume illegal panel */
		panel_row_min = cur_hgt;
		panel_col_min = cur_wid;
	}

	/* Make a dungeon */
	if (!cave_gen())
	{
#ifdef JP
*why = "ダンジョン生成に失敗";
#else
		*why = "could not place player";
#endif

		return FALSE;
	}
	else return TRUE;
}

static byte extract_feeling(void)
{
	/* Hack -- no feeling in the town */
	if (!dun_level) return 0;

	/* Hack -- Have a special feeling sometimes */
	if (good_item_flag && !preserve_mode) return 1;

 	if (rating > 100) return 2;
	if (rating > 80) return 3;
	if (rating > 60) return 4;
	if (rating > 40) return 5;
	if (rating > 30) return 6;
	if (rating > 20) return 7;
	if (rating > 10) return 8;
	if (rating > 0) return 9;

	if((turn - old_turn) > TURNS_PER_TICK * TOWN_DAWN /2)
		chg_virtue(V_PATIENCE, 1);

	return 10;
}


static void place_pet(void)
{
	int i, max_num;

	if (p_ptr->wild_mode)
		max_num = 1;
	else
		max_num = 21;

	for (i = 0; i < max_num; i++)
	{
		int cy, cx, m_idx;

		if (!(party_mon[i].r_idx)) continue;


		if (i == 0)
		{
			m_idx = m_pop();
			p_ptr->riding = m_idx;
			if (m_idx)
			{
				cy = py;
				cx = px;
			}
		}
		else
		{
			int j, d;

			for(d = 1; d < 6; d++)
			{
				for(j = 1000; j > 0; j--)
				{
					scatter(&cy, &cx, py, px, d, 0);
					if ((cave_floor_bold(cy, cx) || (cave[cy][cx].feat == FEAT_TREES)) && !cave[cy][cx].m_idx && !((cy == py) && (cx == px))) break;
				}
				if (j) break;
			}
			if (d == 6 || p_ptr->inside_arena || p_ptr->inside_battle)
				m_idx = 0;
			else
				m_idx = m_pop();
		}
		
		if (m_idx)
		{
			monster_type *m_ptr = &m_list[m_idx];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			
			cave[cy][cx].m_idx = m_idx;
			m_ptr->r_idx = party_mon[i].r_idx;
			m_ptr->ap_r_idx = party_mon[i].ap_r_idx;
			m_ptr->sub_align = party_mon[i].sub_align;
			m_ptr->fy = cy;
			m_ptr->fx = cx;
			m_ptr->cdis = party_mon[i].cdis;
			m_ptr->mflag = party_mon[i].mflag;
			m_ptr->mflag2 = party_mon[i].mflag2;
			m_ptr->ml = TRUE;
			m_ptr->hp = party_mon[i].hp;
			m_ptr->maxhp = party_mon[i].maxhp;
			m_ptr->max_maxhp = party_mon[i].max_maxhp;
			m_ptr->mspeed = party_mon[i].mspeed;
			m_ptr->fast = party_mon[i].fast;
			m_ptr->slow = party_mon[i].slow;
			m_ptr->stunned = party_mon[i].stunned;
			m_ptr->confused = party_mon[i].confused;
			m_ptr->monfear = party_mon[i].monfear;
			m_ptr->invulner = party_mon[i].invulner;
			m_ptr->smart = party_mon[i].smart;
			m_ptr->csleep = 0;
			m_ptr->nickname = party_mon[i].nickname;
			m_ptr->energy_need = party_mon[i].energy_need;
			m_ptr->exp = party_mon[i].exp;
			set_pet(m_ptr);

			if ((r_ptr->flags1 & RF1_FORCE_SLEEP) && !ironman_nightmare)
			{
				/* Monster is still being nice */
				m_ptr->mflag |= (MFLAG_NICE);
				
				/* Must repair monsters */
				repair_monsters = TRUE;
			}
			
			/* Update the monster */
			update_mon(m_idx, TRUE);
			lite_spot(cy, cx);
			
			r_ptr->cur_num++;
			
			/* Hack -- Count the number of "reproducers" */
			if (r_ptr->flags2 & RF2_MULTIPLY) num_repro++;
			
			/* Hack -- Notice new multi-hued monsters */
			if (r_ptr->flags1 & RF1_ATTR_MULTI) shimmer_monsters = TRUE;
		}
		else
		{
			char m_name[80];
			
			monster_desc(m_name, &party_mon[i], 0);
#ifdef JP
			msg_format("%sとはぐれてしまった。", m_name);
#else
			msg_format("You have lost sight of %s.", m_name);
#endif
			if (record_named_pet && party_mon[i].nickname)
			{
				monster_desc(m_name, &party_mon[i], 0x08);
				do_cmd_write_nikki(NIKKI_NAMED_PET, 5, m_name);
			}
		}
	}
}

/*
 * Wipe all unnecessary flags after cave generation
 */
static void wipe_generate_cave_flags(void)
{
	int x, y;

	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			/* Wipe unused flags */
			cave[y][x].info &= ~(CAVE_MASK);
		}
	}

	if (dun_level)
	{
		for (y = 1; y < cur_hgt - 1; y++)
		{
			for (x = 1; x < cur_wid - 1; x++)
			{
				/* There might be trap */
				cave[y][x].info |= CAVE_UNSAFE;
			}
		}
	}
}

/*
 * Generates a random dungeon level			-RAK-
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 */
void generate_cave(void)
{
	int y, x, num;
	int i;

	/* The dungeon is not ready */
	character_dungeon = FALSE;

	/* No longer in the trap detecteded region */
	p_ptr->dtrap = FALSE;

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;

		cptr why = NULL;


		/* XXX XXX XXX XXX */
		o_max = 1;
		m_max = 1;

		/* Start with a blank cave */
		for (y = 0; y < MAX_HGT; y++)
		{
			for (x = 0; x < MAX_WID; x++)
			{
				/* No flags */
				cave[y][x].info = 0;

				/* No features */
				cave[y][x].feat = 0;

				/* No objects */
				cave[y][x].o_idx = 0;

				/* No monsters */
				cave[y][x].m_idx = 0;

				/* No mimic */
				cave[y][x].mimic = 0;

				/* No flow */
				cave[y][x].cost = 0;
				cave[y][x].dist = 0;
				cave[y][x].when = 0;
			}
		}

		/* Mega-Hack -- no player yet */
		px = py = 0;

		/* Mega-Hack -- no panel yet */
		panel_row_min = 0;
		panel_row_max = 0;
		panel_col_min = 0;
		panel_col_max = 0;

		/* Set the base level */
		base_level = dun_level;

		/* Reset the monster generation level */
		monster_level = base_level;

		/* Reset the object generation level */
		object_level = base_level;

		/* Nothing special here yet */
		good_item_flag = FALSE;

		/* Nothing good here yet */
		rating = 0;

		if ((d_info[dungeon_type].fill_type1 == FEAT_MAGMA_K) || (d_info[dungeon_type].fill_type2 == FEAT_MAGMA_K) || (d_info[dungeon_type].fill_type3 == FEAT_MAGMA_K)) rating += 40;

		ambush_flag = FALSE;

		/* Fill the arrays of floors and walls in the good proportions */
		for (i = 0; i < 100; i++)
		{
			int lim1, lim2, lim3;

			lim1 = d_info[0].floor_percent1;
			lim2 = lim1 + d_info[0].floor_percent2;
			lim3 = lim2 + d_info[0].floor_percent3;

			if (i < lim1)
				floor_type[i] = d_info[0].floor1;
			else if (i < lim2)
				floor_type[i] = d_info[0].floor2;
			else if (i < lim3)
				floor_type[i] = d_info[0].floor3;

			lim1 = d_info[0].fill_percent1;
			lim2 = lim1 + d_info[0].fill_percent2;
			lim3 = lim2 + d_info[0].fill_percent3;
			if (i < lim1)
				fill_type[i] = d_info[0].fill_type1;
			else if (i < lim2)
				fill_type[i] = d_info[0].fill_type2;
			else if (i < lim3)
				fill_type[i] = d_info[0].fill_type3;
		}

		/* Build the arena -KMW- */
		if (p_ptr->inside_arena)
		{
				/* Small arena */
			arena_gen();
		}

		/* Build the battle -KMW- */
		else if (p_ptr->inside_battle)
		{
				/* Small arena */
			battle_gen();
		}

		else if (p_ptr->inside_quest)
		{
			quest_gen();
		}

		/* Build the town */
		else if (!dun_level)
		{
				/* Make the wilderness */
			if (p_ptr->wild_mode) wilderness_gen_small();
			else wilderness_gen();
		}

		/* Build a real level */
		else
		{
			okay = level_gen(&why);
		}

		/* Extract the feeling */
		feeling = extract_feeling();

		/* Prevent object over-flow */
		if (o_max >= max_o_idx)
		{
			/* Message */
#ifdef JP
why = "アイテムが多すぎる";
#else
			why = "too many objects";
#endif


			/* Message */
			okay = FALSE;
		}
		/* Prevent monster over-flow */
		else if (m_max >= max_m_idx)
		{
			/* Message */
#ifdef JP
why = "モンスターが多すぎる";
#else
			why = "too many monsters";
#endif


			/* Message */
			okay = FALSE;
		}

		/* Mega-Hack -- "auto-scum" */
		else if ((auto_scum || ironman_autoscum) && (num < 100) &&
				 !p_ptr->inside_quest && !(d_info[dungeon_type].flags1 & DF1_BEGINNER))
		{
			/* Require "goodness" */
			if ((feeling > 9) ||
			    ((dun_level >= 7) && (feeling > 8)) ||
			    ((dun_level >= 15) && (feeling > 7)) ||
			    ((dun_level >= 35) && (feeling > 6)) ||
			    ((dun_level >= 70) && (feeling > 5)))
			{
				/* Give message to cheaters */
				if (cheat_room || cheat_hear ||
				    cheat_peek || cheat_xtra)
				{
					/* Message */
#ifdef JP
why = "退屈な階";
#else
					why = "boring level";
#endif

				}

				/* Try again */
				okay = FALSE;
			}
		}

		/* Accept */
		if (okay) break;

		/* Message */
#ifdef JP
if (why) msg_format("生成やり直し(%s)", why);
#else
		if (why) msg_format("Generation restarted (%s)", why);
#endif


		/* Wipe the objects */
		wipe_o_list();

		/* Wipe the monsters */
		wipe_m_list();
	}

	for (y = 1; y < cur_hgt - 1; y++)
	{
		for (x = 1; x < cur_wid - 1; x++)
		{
			if ((cave[y][x].feat == FEAT_DEEP_LAVA))
			{
				int i;
				for (i = 0; i < 9; i++)
				{
					cave[y+ddy_ddd[i]][x+ddx_ddd[i]].info |= CAVE_GLOW;
				}
			}
		}
	}

	wipe_generate_cave_flags();

	place_pet();

	/* The dungeon is ready */
	character_dungeon = TRUE;

	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN) wiz_lite(TRUE, (bool)(p_ptr->pclass == CLASS_NINJA));

	/* Remember when this level was "created" */
	old_turn = turn;
}
