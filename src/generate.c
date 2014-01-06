/*!
 * @file generate.c
 * @brief ダンジョンの生成 / Dungeon generation
 * @date 2014/01/04
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 * @details
 * Note that Level generation is *not* an important bottleneck,\n
 * though it can be annoyingly slow on older machines...  Thus\n
 * we emphasize "simplicity" and "correctness" over "speed".\n
 *\n
 * This entire file is only needed for generating levels.\n
 * This may allow smart compilers to only load it when needed.\n
 *\n
 * Consider the "v_info.txt" file for vault generation.\n
 *\n
 * In this file, we use the "special" granite and perma-wall sub-types,\n
 * where "basic" is normal, "inner" is inside a room, "outer" is the\n
 * outer wall of a room, and "solid" is the outer wall of the dungeon\n
 * or any walls that may not be pierced by corridors.  Thus the only\n
 * wall type that may be pierced by a corridor is the "outer granite"\n
 * type.  The "basic granite" type yields the "actual" corridors.\n
 *\n
 * Note that we use the special "solid" granite wall type to prevent\n
 * multiple corridors from piercing a wall in two adjacent locations,\n
 * which would be messy, and we use the special "outer" granite wall\n
 * to indicate which walls "surround" rooms, and may thus be "pierced"\n
 * by corridors entering or leaving the room.\n
 *\n
 * Note that a tunnel which attempts to leave a room near the "edge"\n
 * of the dungeon in a direction toward that edge will cause "silly"\n
 * wall piercings, but will have no permanently incorrect effects,\n
 * as long as the tunnel can *eventually* exit from another side.\n
 * And note that the wall may not come back into the room by the\n
 * hole it left through, so it must bend to the left or right and\n
 * then optionally re-enter the room (at least 2 grids away).  This\n
 * is not a problem since every room that is large enough to block\n
 * the passage of tunnels is also large enough to allow the tunnel\n
 * to pierce the room itself several times.\n
 *\n
 * Note that no two corridors may enter a room through adjacent grids,\n
 * they must either share an entryway or else use entryways at least\n
 * two grids apart.  This prevents "large" (or "silly") doorways.\n
 *\n
 * To create rooms in the dungeon, we first divide the dungeon up\n
 * into "blocks" of 11x11 grids each, and require that all rooms\n
 * occupy a rectangular group of blocks.  As long as each room type\n
 * reserves a sufficient number of blocks, the room building routines\n
 * will not need to check bounds.  Note that most of the normal rooms\n
 * actually only use 23x11 grids, and so reserve 33x11 grids.\n
 *\n
 * Note that the use of 11x11 blocks (instead of the old 33x11 blocks)\n
 * allows more variability in the horizontal placement of rooms, and\n
 * at the same time has the disadvantage that some rooms (two thirds\n
 * of the normal rooms) may be "split" by panel boundaries.  This can\n
 * induce a situation where a player is in a room and part of the room\n
 * is off the screen.  It may be annoying enough to go back to 33x11\n
 * blocks to prevent this visual situation.\n
 *\n
 * Note that the dungeon generation routines are much different (2.7.5)\n
 * and perhaps "DUN_ROOMS" should be less than 50.\n
 *\n
 * XXX XXX XXX Note that it is possible to create a room which is only\n
 * connected to itself, because the "tunnel generation" code allows a\n
 * tunnel to leave a room, wander around, and then re-enter the room.\n
 *\n
 * XXX XXX XXX Note that it is possible to create a set of rooms which\n
 * are only connected to other rooms in that set, since there is nothing\n
 * explicit in the code to prevent this from happening.  But this is less\n
 * likely than the "isolated room" problem, because each room attempts to\n
 * connect to another room, in a giant cycle, thus requiring at least two\n
 * bizarre occurances to create an isolated section of the dungeon.\n
 *\n
 * Note that (2.7.9) monster pits have been split into monster "nests"\n
 * and monster "pits".  The "nests" have a collection of monsters of a\n
 * given type strewn randomly around the room (jelly, animal, or undead),\n
 * while the "pits" have a collection of monsters of a given type placed\n
 * around the room in an organized manner (orc, troll, giant, dragon, or\n
 * demon).  Note that both "nests" and "pits" are now "level dependant",\n
 * and both make 16 "expensive" calls to the "get_mon_num()" function.\n
 *\n
 * Note that the cave grid flags changed in a rather drastic manner\n
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain\n
 * features, such as doors and stairs and traps and rubble and walls,\n
 * are all handled as a set of 64 possible "terrain features", and\n
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.\n
 *\n
 * The 64 new "dungeon features" will also be used for "visual display"\n
 * but we must be careful not to allow, for example, the user to display\n
 * hidden traps in a different way from floors, or secret doors in a way\n
 * different from granite walls, or even permanent granite in a different\n
 * way from granite.  XXX XXX XXX\n
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"
#include "rooms.h"
#include "streams.h"

int dun_tun_rnd;
int dun_tun_chg;
int dun_tun_con;
int dun_tun_pen;
int dun_tun_jct;


/*!
 * Dungeon generation data -- see "cave_gen()"
 */
dun_data *dun;


/*!
 * @brief 上下左右の外壁数をカウントする / Count the number of walls adjacent to the given grid.
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @return 隣接する外壁の数
 * @note Assumes "in_bounds(y, x)"
 * @details We count only granite walls and permanent walls.
 */
static int next_to_walls(int y, int x)
{
	int k = 0;

	if (in_bounds(y + 1, x) && is_extra_bold(y + 1, x)) k++;
	if (in_bounds(y - 1, x) && is_extra_bold(y - 1, x)) k++;
	if (in_bounds(y, x + 1) && is_extra_bold(y, x + 1)) k++;
	if (in_bounds(y, x - 1) && is_extra_bold(y, x - 1)) k++;

	return (k);
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う / Helper function for alloc_stairs(). Is this a good location for stairs?
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(int y, int x, int walls)
{
	/* Access the grid */
	cave_type *c_ptr = &cave[y][x];

	/* Require "naked" floor grid */
	if (!is_floor_grid(c_ptr)) return FALSE;
	if (pattern_tile(y, x)) return FALSE;
	if (c_ptr->o_idx || c_ptr->m_idx) return FALSE;

	/* Require a certain number of adjacent walls */
	if (next_to_walls(y, x) < walls) return FALSE;

	return TRUE;
}


/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
static bool alloc_stairs(int feat, int num, int walls)
{
	int i;
	int shaft_num = 0;

	feature_type *f_ptr = &f_info[feat];

	if (have_flag(f_ptr->flags, FF_LESS))
	{
		/* No up stairs in town or in ironman mode */
		if (ironman_downward || !dun_level) return TRUE;

		if (dun_level > d_info[dungeon_type].mindepth)
			shaft_num = (randint1(num+1))/2;
	}
	else if (have_flag(f_ptr->flags, FF_MORE))
	{
		int q_idx = quest_number(dun_level);

		/* No downstairs on quest levels */
		if (dun_level > 1 && q_idx)
		{
			monster_race *r_ptr = &r_info[quest[q_idx].r_idx];

			/* The quest monster(s) is still alive? */
			if (!(r_ptr->flags1 & RF1_UNIQUE) || 0 < r_ptr->max_num)
				return TRUE;
		}

		/* No downstairs at the bottom */
		if (dun_level >= d_info[dungeon_type].maxdepth) return TRUE;

		if ((dun_level < d_info[dungeon_type].maxdepth-1) && !quest_number(dun_level+1))
			shaft_num = (randint1(num)+1)/2;
	}

	/* Paranoia */
	else return FALSE;


	/* Place "num" stairs */
	for (i = 0; i < num; i++)
	{
		while (TRUE)
		{
			int y = 0, x = 0;
			cave_type *c_ptr;

			int candidates = 0;
			int pick;

			for (y = 1; y < cur_hgt - 1; y++)
			{
				for (x = 1; x < cur_wid - 1; x++)
				{
					if (alloc_stairs_aux(y, x, walls))
					{
						/* A valid space found */
						candidates++;
					}
				}
			}

			/* No valid place! */
			if (!candidates)
			{
				/* There are exactly no place! */
				if (walls <= 0) return FALSE;

				/* Decrease walls limit, and try again */
				walls--;
				continue;
			}

			/* Choose a random one */
			pick = randint1(candidates);

			for (y = 1; y < cur_hgt - 1; y++)
			{
				for (x = 1; x < cur_wid - 1; x++)
				{
					if (alloc_stairs_aux(y, x, walls))
					{
						pick--;

						/* Is this a picked one? */
						if (!pick) break;
					}
				}

				if (!pick) break;
			}

			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Clear possible garbage of hidden trap */
			c_ptr->mimic = 0;

			/* Clear previous contents, add stairs */
			c_ptr->feat = (i < shaft_num) ? feat_state(feat, FF_SHAFT) : feat;

			/* No longer "FLOOR" */
			c_ptr->info &= ~(CAVE_FLOOR);

			/* Success */
			break;
		}
	}
	return TRUE;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
static void alloc_object(int set, int typ, int num)
{
	int y = 0, x = 0, k;
	int dummy = 0;
	cave_type *c_ptr;

	/* A small level has few objects. */
	num = num * cur_hgt * cur_wid / (MAX_HGT*MAX_WID) +1;

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

			/* Avoid player location */
			if (player_bold(y, x)) continue;

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
				place_object(y, x, 0L);
				break;
			}
		}
	}
}

/*!
 * @brief 隣接4マスに存在する通路の数を返す / Count the number of "corridor" grids adjacent to the given grid.
 * @param y1 基準となるマスのY座標
 * @param x1 基準となるマスのX座標
 * @return 通路の数
 * @note Assumes "in_bounds(y1, x1)"
 * @details
 * XXX XXX This routine currently only counts actual "empty floor"\n
 * grids which are not in rooms.  We might want to also count stairs,\n
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
		if (cave_have_flag_grid(c_ptr, FF_WALL)) continue;

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


/*!
 * @brief ドアを設置可能な地形かを返す / Determine if the given location is "between" two walls, and "next to" two corridor spaces.
 * @param y 判定を行いたいマスのY座標
 * @param x 判定を行いたいマスのX座標
 * @return ドアを設置可能ならばTRUEを返す
 * @note Assumes "in_bounds(y1, x1)"
 * @details
 * XXX XXX XXX\n
 * Assumes "in_bounds(y, x)"\n
 */
static bool possible_doorway(int y, int x)
{
	/* Count the adjacent corridors */
	if (next_to_corr(y, x) >= 2)
	{
		/* Check Vertical */
		if (cave_have_flag_bold(y - 1, x, FF_WALL) &&
		    cave_have_flag_bold(y + 1, x, FF_WALL))
		{
			return (TRUE);
		}

		/* Check Horizontal */
		if (cave_have_flag_bold(y, x - 1, FF_WALL) &&
		    cave_have_flag_bold(y, x + 1, FF_WALL))
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}

/*!
 * @brief ドアの設置を試みる / Places door at y, x position if at least 2 walls found
 * @param y 設置を行いたいマスのY座標
 * @param x 設置を行いたいマスのX座標
 * @return なし
 */
static void try_door(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x)) return;

	/* Ignore walls */
	if (cave_have_flag_bold(y, x, FF_WALL)) return;

	/* Ignore room grids */
	if (cave[y][x].info & (CAVE_ROOM)) return;

	/* Occasional door (if allowed) */
	if ((randint0(100) < dun_tun_jct) && possible_doorway(y, x) && !(d_info[dungeon_type].flags1 & DF1_NO_DOORS))
	{
		/* Place a door */
		place_random_door(y, x, FALSE);
	}
}


/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(void)
{
	int i;

	/* Handle the quest monster placements */
	for (i = 0; i < max_quests; i++)
	{
		monster_race *r_ptr;
		u32b mode;
		int j;

		if (quest[i].status != QUEST_STATUS_TAKEN ||
		    (quest[i].type != QUEST_TYPE_KILL_LEVEL &&
		     quest[i].type != QUEST_TYPE_RANDOM) ||
		    quest[i].level != dun_level ||
		    dungeon_type != quest[i].dungeon ||
		    (quest[i].flags & QUEST_FLAG_PRESET))
		{
			/* Ignore it */
			continue;
		}

		r_ptr = &r_info[quest[i].r_idx];

		/* Hack -- "unique" monsters must be "unique" */
		if ((r_ptr->flags1 & RF1_UNIQUE) &&
		    (r_ptr->cur_num >= r_ptr->max_num)) continue;

		mode = (PM_NO_KAGE | PM_NO_PET);

		if (!(r_ptr->flags1 & RF1_FRIENDS))
			mode |= PM_ALLOW_GROUP;

		for (j = 0; j < (quest[i].max_num - quest[i].cur_num); j++)
		{
			int k;

			for (k = 0; k < SAFE_MAX_ATTEMPTS; k++)
			{
				int x, y;
				int l;

				/* Find an empty grid */
				for (l = SAFE_MAX_ATTEMPTS; l > 0; l--)
				{
					cave_type    *c_ptr;
					feature_type *f_ptr;

					y = randint0(cur_hgt);
					x = randint0(cur_wid);

					c_ptr = &cave[y][x];
					f_ptr = &f_info[c_ptr->feat];

					if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) continue;
					if (!monster_can_enter(y, x, r_ptr, 0)) continue;
					if (distance(y, x, py, px) < 10) continue;
					if (c_ptr->info & CAVE_ICKY) continue;
					else break;
				}

				/* Failed to place */
				if (!l) return FALSE;

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

			/* Failed to place */
			if (k == SAFE_MAX_ATTEMPTS) return FALSE;
		}
	}

	return TRUE;
}


/*!
 * @brief マスにフロア端用の永久壁を配置する / Set boundary mimic and add "solid" perma-wall
 * @param c_ptr 永久壁を廃止したいマス構造体の参照ポインタ
 * @return なし
 */
static void set_bound_perm_wall(cave_type *c_ptr)
{
	if (bound_walls_perm)
	{
		/* Clear boundary mimic */
		c_ptr->mimic = 0;
	}
	else
	{
		feature_type *f_ptr = &f_info[c_ptr->feat];

		/* Hack -- Decline boundary walls with known treasure  */
		if ((have_flag(f_ptr->flags, FF_HAS_GOLD) || have_flag(f_ptr->flags, FF_HAS_ITEM)) &&
		    !have_flag(f_ptr->flags, FF_SECRET))
			c_ptr->feat = feat_state(c_ptr->feat, FF_ENSECRET);

		/* Set boundary mimic */
		c_ptr->mimic = c_ptr->feat;
	}

	/* Add "solid" perma-wall */
	place_solid_perm_grid(c_ptr);
}

/*!
 * @brief フロアに洞窟や湖を配置する / Generate various caverns and lakes
 * @details There were moved from cave_gen().
 * @return なし
 */
static void gen_caverns_and_lakes(void)
{
#ifdef ALLOW_CAVERNS_AND_LAKES
	/* Possible "destroyed" level */
	if ((dun_level > 30) && one_in_(DUN_DEST*2) && (small_levels) && (d_info[dungeon_type].flags1 & DF1_DESTROY))
	{
		dun->destroyed = TRUE;

		/* extra rubble around the place looks cool */
		build_lake(one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
	}

	/* Make a lake some of the time */
	if (one_in_(LAKE_LEVEL) && !dun->empty_level && !dun->destroyed &&
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
			if ((dun_level > 80) && (randint0(count) < 2)) dun->laketype = LAKE_T_LAVA;
			count -= 2;

			/* Lake of Lava2 */
			if (!dun->laketype && (dun_level > 80) && one_in_(count)) dun->laketype = LAKE_T_FIRE_VAULT;
			count--;
		}

		if ((d_info[dungeon_type].flags1 & DF1_LAKE_WATER) && !dun->laketype)
		{
			/* Lake of Water */
			if ((dun_level > 50) && randint0(count) < 2) dun->laketype = LAKE_T_WATER;
			count -= 2;

			/* Lake of Water2 */
			if (!dun->laketype && (dun_level > 50) && one_in_(count)) dun->laketype = LAKE_T_WATER_VAULT;
			count--;
		}

		if ((d_info[dungeon_type].flags1 & DF1_LAKE_RUBBLE) && !dun->laketype)
		{
			/* Lake of rubble */
			if ((dun_level > 35) && (randint0(count) < 2)) dun->laketype = LAKE_T_CAVE;
			count -= 2;

			/* Lake of rubble2 */
			if (!dun->laketype && (dun_level > 35) && one_in_(count)) dun->laketype = LAKE_T_EARTH_VAULT;
			count--;
		}

		/* Lake of tree */
		if ((dun_level > 5) && (d_info[dungeon_type].flags1 & DF1_LAKE_TREE) && !dun->laketype) dun->laketype = LAKE_T_AIR_VAULT;

		if (dun->laketype)
		{
			if (cheat_room)
#ifdef JP
				msg_print("湖を生成。");
#else
				msg_print("Lake on the level.");
#endif

			build_lake(dun->laketype);
		}
	}

	if ((dun_level > DUN_CAVERN) && !dun->empty_level &&
	    (d_info[dungeon_type].flags1 & DF1_CAVERN) &&
	    !dun->laketype && !dun->destroyed && (randint1(1000) < dun_level))
	{
		dun->cavern = TRUE;

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
	if (quest_number(dun_level)) dun->destroyed = FALSE;
}


/*!
 * @brief ダンジョン生成のメインルーチン / Generate a new dungeon level
 * @details Note that "dun_body" adds about 4000 bytes of memory to the stack.
 * @return ダンジョン生成が全て無事に成功したらTRUEを返す。
 */
static bool cave_gen(void)
{
	int i, k, y, x;

	dun_data dun_body;

	/* Global data */
	dun = &dun_body;

	dun->destroyed = FALSE;
	dun->empty_level = FALSE;
	dun->cavern = FALSE;
	dun->laketype = 0;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(dungeon_type);

	/* Prepare allocation table */
	get_mon_num_prep(get_monster_hook(), NULL);

	/* Randomize the dungeon creation values */
	dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
	dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
	dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
	dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
	dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);

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

	/* No rooms yet */
	dun->cent_n = 0;

	/* Empty arena levels */
	if (ironman_empty_levels || ((d_info[dungeon_type].flags1 & DF1_ARENA) && (empty_levels && one_in_(EMPTY_LEVEL))))
	{
		dun->empty_level = TRUE;

		if (cheat_room)
#ifdef JP
			msg_print("アリーナレベル");
#else
			msg_print("Arena level.");
#endif
	}

	if (dun->empty_level)
	{
		/* Start with floors */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				place_floor_bold(y, x);
			}
		}

		/* Special boundary walls -- Top and bottom */
		for (x = 0; x < cur_wid; x++)
		{
			place_extra_bold(0, x);
			place_extra_bold(cur_hgt - 1, x);
		}

		/* Special boundary walls -- Left and right */
		for (y = 1; y < (cur_hgt - 1); y++)
		{
			place_extra_bold(y, 0);
			place_extra_bold(y, cur_wid - 1);
		}
	}
	else
	{
		/* Start with walls */
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				place_extra_bold(y, x);
			}
		}
	}


	/* Generate various caverns and lakes */
	gen_caverns_and_lakes();


	/* Build maze */
	if (d_info[dungeon_type].flags1 & DF1_MAZE)
	{
		build_maze_vault(cur_wid/2-1, cur_hgt/2-1, cur_wid-4, cur_hgt-4, FALSE);

		/* Place 3 or 4 down stairs near some walls */
		if (!alloc_stairs(feat_down_stair, rand_range(2, 3), 3)) return FALSE;

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(feat_up_stair, 1, 3)) return FALSE;
	}

	/* Build some rooms */
	else
	{
		int tunnel_fail_count = 0;

		/*
		 * Build each type of room in turn until we cannot build any more.
		 */
		if (!generate_rooms()) return FALSE;


		/* Make a hole in the dungeon roof sometimes at level 1 */
		if (dun_level == 1)
		{
			while (one_in_(DUN_MOS_DEN))
			{
				place_trees(randint1(cur_wid - 2), randint1(cur_hgt - 2));
			}
		}

		/* Destroy the level if necessary */
		if (dun->destroyed) destroy_level();

		/* Hack -- Add some rivers */
		if (one_in_(3) && (randint1(dun_level) > 5))
		{
			int feat1 = 0, feat2 = 0;

			/* Choose water or lava */
			if ((randint1(MAX_DEPTH * 2) - 1 > dun_level) && (d_info[dungeon_type].flags1 & DF1_WATER_RIVER))
			{
				feat1 = feat_deep_water;
				feat2 = feat_shallow_water;
			}
			else if  (d_info[dungeon_type].flags1 & DF1_LAVA_RIVER)
			{
				feat1 = feat_deep_lava;
				feat2 = feat_shallow_lava;
			}
			else feat1 = 0;

			if (feat1)
			{
				feature_type *f_ptr = &f_info[feat1];

				/* Only add river if matches lake type or if have no lake at all */
				if (((dun->laketype == LAKE_T_LAVA) && have_flag(f_ptr->flags, FF_LAVA)) ||
				    ((dun->laketype == LAKE_T_WATER) && have_flag(f_ptr->flags, FF_WATER)) ||
				     !dun->laketype)
				{
					add_river(feat1, feat2);
				}
			}
		}

		/* Hack -- Scramble the room order */
		for (i = 0; i < dun->cent_n; i++)
		{
			int ty, tx;
			int pick = rand_range(0, i);

			ty = dun->cent[i].y;
			tx = dun->cent[i].x;
			dun->cent[i].y = dun->cent[pick].y;
			dun->cent[i].x = dun->cent[pick].x;
			dun->cent[pick].y = ty;
			dun->cent[pick].x = tx;
		}

		/* Start with no tunnel doors */
		dun->door_n = 0;

		/* Hack -- connect the first room to the last room */
		y = dun->cent[dun->cent_n-1].y;
		x = dun->cent[dun->cent_n-1].x;

		/* Connect all the rooms together */
		for (i = 0; i < dun->cent_n; i++)
		{
			int j;

			/* Reset the arrays */
			dun->tunn_n = 0;
			dun->wall_n = 0;

			/* Connect the room to the previous room */
			if (randint1(dun_level) > d_info[dungeon_type].tunnel_percent)
			{
				/* make cave-like tunnel */
				(void)build_tunnel2(dun->cent[i].x, dun->cent[i].y, x, y, 2, 2);
			}
			else
			{
				/* make normal tunnel */
				if (!build_tunnel(dun->cent[i].y, dun->cent[i].x, y, x)) tunnel_fail_count++;
			}

			if (tunnel_fail_count >= 2) return FALSE;

			/* Turn the tunnel into corridor */
			for (j = 0; j < dun->tunn_n; j++)
			{
				cave_type *c_ptr;
				feature_type *f_ptr;

				/* Access the grid */
				y = dun->tunn[j].y;
				x = dun->tunn[j].x;

				/* Access the grid */
				c_ptr = &cave[y][x];
				f_ptr = &f_info[c_ptr->feat];

				/* Clear previous contents (if not a lake), add a floor */
				if (!have_flag(f_ptr->flags, FF_MOVE) || (!have_flag(f_ptr->flags, FF_WATER) && !have_flag(f_ptr->flags, FF_LAVA)))
				{
					/* Clear mimic type */
					c_ptr->mimic = 0;

					place_floor_grid(c_ptr);
				}
			}

			/* Apply the piercings that we found */
			for (j = 0; j < dun->wall_n; j++)
			{
				cave_type *c_ptr;

				/* Access the grid */
				y = dun->wall[j].y;
				x = dun->wall[j].x;

				/* Access the grid */
				c_ptr = &cave[y][x];

				/* Clear mimic type */
				c_ptr->mimic = 0;

				/* Clear previous contents, add up floor */
				place_floor_grid(c_ptr);

				/* Occasional doorway */
				if ((randint0(100) < dun_tun_pen) && !(d_info[dungeon_type].flags1 & DF1_NO_DOORS))
				{
					/* Place a random door */
					place_random_door(y, x, TRUE);
				}
			}

			/* Remember the "previous" room */
			y = dun->cent[i].y;
			x = dun->cent[i].x;
		}

		/* Place intersection doors */
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
		if (!alloc_stairs(feat_down_stair, rand_range(3, 4), 3)) return FALSE;

		/* Place 1 or 2 up stairs near some walls */
		if (!alloc_stairs(feat_up_stair, rand_range(1, 2), 3)) return FALSE;
	}

	if (!dun->laketype)
	{
		if (d_info[dungeon_type].stream2)
		{
			/* Hack -- Add some quartz streamers */
			for (i = 0; i < DUN_STR_QUA; i++)
			{
				build_streamer(d_info[dungeon_type].stream2, DUN_STR_QC);
			}
		}

		if (d_info[dungeon_type].stream1)
		{
			/* Hack -- Add some magma streamers */
			for (i = 0; i < DUN_STR_MAG; i++)
			{
				build_streamer(d_info[dungeon_type].stream1, DUN_STR_MC);
			}
		}
	}

	/* Special boundary walls -- Top and bottom */
	for (x = 0; x < cur_wid; x++)
	{
		set_bound_perm_wall(&cave[0][x]);
		set_bound_perm_wall(&cave[cur_hgt - 1][x]);
	}

	/* Special boundary walls -- Left and right */
	for (y = 1; y < (cur_hgt - 1); y++)
	{
		set_bound_perm_wall(&cave[y][0]);
		set_bound_perm_wall(&cave[y][cur_wid - 1]);
	}

	/* Determine the character location */
	if (!new_player_spot()) return FALSE;

	if (!place_quest_monsters()) return FALSE;

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
		(void)alloc_monster(0, PM_ALLOW_SLEEP);
	}

	/* Place some traps in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

	/* Put some rubble in corridors (except NO_CAVE dungeon (Castle)) */
	if (!(d_info[dungeon_type].flags1 & DF1_NO_CAVE)) alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

	/* Mega Hack -- No object at first level of deeper dungeon */
	if (p_ptr->enter_dungeon && dun_level > 1)
	{
		/* No stair scum! */
		object_level = 1;
	}

	/* Put some objects in rooms */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));

	/* Set back to default */
	object_level = base_level;

	/* Put the Guardian */
	if (!alloc_guardian(TRUE)) return FALSE;

	if (dun->empty_level && (!one_in_(DARK_EMPTY) || (randint1(100) > dun_level)) && !(d_info[dungeon_type].flags1 & DF1_DARKNESS))
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

	return TRUE;
}

/*!
 * @brief 闘技場用のアリーナ地形を作成する / Builds the arena after it is entered -KMW-
 * @return なし
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
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 5; i--)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	place_extra_perm_bold(y_height+6, x_left+18);
	cave[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-6, x_left+18);
	cave[y_depth-6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_height+6, x_right-18);
	cave[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-6, x_right-18);
	cave[y_depth-6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	i = y_height + 5;
	j = xval;
	cave[i][j].feat = f_tag_to_index("ARENA_GATE");
	cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i, j);
}

/*!
 * @brief 闘技場への入場処理 / Town logic flow for generation of arena -KMW-
 * @return なし
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
			place_solid_perm_bold(y, x);

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
			cave[y][x].feat = feat_floor;
		}
	}

	build_arena();

	if(!place_monster_aux(0, py + 5, px, arena_info[p_ptr->arena_number].r_idx, (PM_NO_KAGE | PM_NO_PET)))
	{
		p_ptr->exit_bldg = TRUE;
		p_ptr->arena_number++;
#ifdef JP
		msg_print("相手は欠場した。あなたの不戦勝だ。");
#else
		msg_print("The enemy is unable appear. You won by default.");
#endif
	}

}

/*!
 * @brief モンスター闘技場のフロア生成 / Builds the arena after it is entered -KMW-
 * @return なし
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
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (i = y_depth; i >= y_depth - 3; i--)
		for (j = x_left; j <= x_right; j++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_left; j <= x_left + 17; j++)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}
	for (j = x_right; j >= x_right - 17; j--)
		for (i = y_height; i <= y_depth; i++)
		{
			place_extra_perm_bold(i, j);
			cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
		}

	place_extra_perm_bold(y_height+6, x_left+18);
	cave[y_height+6][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-4, x_left+18);
	cave[y_depth-4][x_left+18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_height+6, x_right-18);
	cave[y_height+6][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);
	place_extra_perm_bold(y_depth-4, x_right-18);
	cave[y_depth-4][x_right-18].info |= (CAVE_GLOW | CAVE_MARK);

	for (i = y_height + 1; i <= y_height + 5; i++)
		for (j = x_left + 20 + 2 * (y_height + 5 - i); j <= x_right - 20 - 2 * (y_height + 5 - i); j++)
		{
			cave[i][j].feat = feat_permanent_glass_wall;
		}

	i = y_height + 1;
	j = xval;
	cave[i][j].feat = f_tag_to_index("BUILDING_3");
	cave[i][j].info |= (CAVE_GLOW | CAVE_MARK);
	player_place(i, j);
}

/*!
 * @brief モンスター闘技場への導入処理 / Town logic flow for generation of arena -KMW-
 * @return なし
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
			place_solid_perm_bold(y, x);

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
			cave[y][x].feat = feat_floor;
		}
	}

	build_battle();

	for(i=0;i<4;i++)
	{
		place_monster_aux(0, py + 8 + (i/2)*4, px - 2 + (i%2)*4, battle_mon[i],
				  (PM_NO_KAGE | PM_NO_PET));
		set_friendly(&m_list[cave[py+8+(i/2)*4][px-2+(i%2)*4].m_idx]);
	}
	for(i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		if (!m_ptr->r_idx) continue;

		/* Hack -- Detect monster */
		m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}
}

/*!
 * @brief 固定マップクエストのフロア生成 / Generate a quest level
 * @return なし
 */
static void quest_gen(void)
{
	int x, y;


	/* Start with perm walls */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			place_solid_perm_bold(y, x);
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

	init_flags = INIT_CREATE_DUNGEON;

	process_dungeon_file("q_info.txt", 0, 0, MAX_HGT, MAX_WID);
}

/*!
 * @brief ダンジョン時のランダムフロア生成 / Make a real level
 * @return フロアの生成に成功したらTRUE
 */
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
		  msg_format("X:%d, Y:%d.", cur_wid, cur_hgt);
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

/*!
 * @brief フロアに存在する全マスの記憶状態を初期化する / Wipe all unnecessary flags after cave generation
 * @return なし
 */
void wipe_generate_cave_flags(void)
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

/*!
 * @brief フロアの全情報を初期化する / Clear and empty the cave
 * @return なし
 */
void clear_cave(void)
{
	int x, y, i;

	/* Very simplified version of wipe_o_list() */
	(void)C_WIPE(o_list, o_max, object_type);
	o_max = 1;
	o_cnt = 0;

	/* Very simplified version of wipe_m_list() */
	for (i = 1; i < max_r_idx; i++)
		r_info[i].cur_num = 0;
	(void)C_WIPE(m_list, m_max, monster_type);
	m_max = 1;
	m_cnt = 0;
	for (i = 0; i < MAX_MTIMED; i++) mproc_max[i] = 0;

	/* Pre-calc cur_num of pets in party_mon[] */
	precalc_cur_num_of_pet();


	/* Start with a blank cave */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			/* No flags */
			c_ptr->info = 0;

			/* No features */
			c_ptr->feat = 0;

			/* No objects */
			c_ptr->o_idx = 0;

			/* No monsters */
			c_ptr->m_idx = 0;

			/* No special */
			c_ptr->special = 0;

			/* No mimic */
			c_ptr->mimic = 0;

			/* No flow */
			c_ptr->cost = 0;
			c_ptr->dist = 0;
			c_ptr->when = 0;
		}
	}

	/* Mega-Hack -- no player yet */
	px = py = 0;

	/* Set the base level */
	base_level = dun_level;

	/* Reset the monster generation level */
	monster_level = base_level;

	/* Reset the object generation level */
	object_level = base_level;
}


/*!
 * ダンジョンのランダムフロアを生成する / Generates a random dungeon level -RAK-
 * @return なし
 * @note Hack -- regenerate any "overflow" levels
 */
void generate_cave(void)
{
	int num;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(dungeon_type);

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;

		cptr why = NULL;

		/* Clear and empty the cave */
		clear_cave();

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

	/* Glow deep lava and building entrances */
	glow_deep_lava_and_bldg();

	/* Reset flag */
	p_ptr->enter_dungeon = FALSE;

	wipe_generate_cave_flags();
}
