/*!
 * @file rooms.c
 * @brief ダンジョンフロアの部屋生成処理 / make rooms. Used by generate.c when creating dungeons.
 * @date 2014/01/06
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 * @details
 * Room building routines.\n
 *\n
 * Room types:\n
 *   1 -- normal\n
 *   2 -- overlapping\n
 *   3 -- cross shaped\n
 *   4 -- large room with features\n
 *   5 -- monster nests\n
 *   6 -- monster pits\n
 *   7 -- simple vaults\n
 *   8 -- greater vaults\n
 *   9 -- fractal caves\n
 *  10 -- random vaults\n
 *  11 -- circular rooms\n
 *  12 -- crypts\n
 *  13 -- trapped monster pits\n
 *  14 -- trapped room\n
 *  15 -- glass room\n
 *  16 -- underground arcade\n
 *\n
 * Some functions are used to determine if the given monster\n
 * is appropriate for inclusion in a monster nest or monster pit or\n
 * the given type.\n
 *\n
 * None of the pits/nests are allowed to include "unique" monsters.\n
 */

#include "angband.h"
#include "generate.h"
#include "grid.h"
#include "rooms.h"

#include "rooms-city.h"
#include "rooms-normal.h"
#include "rooms-pitnest.h"
#include "rooms-trap.h"
#include "rooms-vault.h"


/*!
 * 各部屋タイプの生成比定義
 *[from SAngband (originally from OAngband)]\n
 *\n
 * Table of values that control how many times each type of room will\n
 * appear.  Each type of room has its own row, and each column\n
 * corresponds to dungeon levels 0, 10, 20, and so on.  The final\n
 * value is the minimum depth the room can appear at.  -LM-\n
 *\n
 * Level 101 and below use the values for level 100.\n
 *\n
 * Rooms with lots of monsters or loot may not be generated if the\n
 * object or monster lists are already nearly full.  Rooms will not\n
 * appear above their minimum depth.  Tiny levels will not have space\n
 * for all the rooms you ask for.\n
 */
static room_info_type room_info_normal[ROOM_T_MAX] =
{
	/* Depth */
	/*  0  10  20  30  40  50  60  70  80  90 100  min limit */

	{{999,900,800,700,600,500,400,300,200,100,  0},  0}, /*NORMAL   */
	{{  1, 10, 20, 30, 40, 50, 60, 70, 80, 90,100},  1}, /*OVERLAP  */
	{{  1, 10, 20, 30, 40, 50, 60, 70, 80, 90,100},  3}, /*CROSS    */
	{{  1, 10, 20, 30, 40, 50, 60, 70, 80, 90,100},  3}, /*INNER_F  */
	{{  0,  1,  1,  1,  2,  3,  5,  6,  8, 10, 13}, 10}, /*NEST     */
	{{  0,  1,  1,  2,  3,  4,  6,  8, 10, 13, 16}, 10}, /*PIT      */
	{{  0,  1,  1,  1,  2,  2,  3,  5,  6,  8, 10}, 10}, /*LESSER_V */
	{{  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  4}, 20}, /*GREATER_V*/
	{{  0,100,200,300,400,500,600,700,800,900,999}, 10}, /*FRACAVE  */
	{{  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2}, 10}, /*RANDOM_V */
	{{  0,  4,  8, 12, 16, 20, 24, 28, 32, 36, 40},  3}, /*OVAL     */
	{{  1,  6, 12, 18, 24, 30, 36, 42, 48, 54, 60}, 10}, /*CRYPT    */
	{{  0,  0,  1,  1,  1,  2,  3,  4,  5,  6,  8}, 20}, /*TRAP_PIT */
	{{  0,  0,  1,  1,  1,  2,  3,  4,  5,  6,  8}, 20}, /*TRAP     */
	{{  0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  2}, 40}, /*GLASS    */
	{{  1,  1,  1,  1,  1,  1,  1,  2,  2,  3,  3},  1}, /*ARCADE   */
};


/*! 部屋の生成処理順 / Build rooms in descending order of difficulty. */
static byte room_build_order[ROOM_T_MAX] = {
	ROOM_T_GREATER_VAULT,
	ROOM_T_ARCADE,
	ROOM_T_RANDOM_VAULT,
	ROOM_T_LESSER_VAULT,
	ROOM_T_TRAP_PIT,
	ROOM_T_PIT,
	ROOM_T_NEST,
	ROOM_T_TRAP,
	ROOM_T_GLASS,
	ROOM_T_INNER_FEAT,
	ROOM_T_OVAL,
	ROOM_T_CRYPT,
	ROOM_T_OVERLAP,
	ROOM_T_CROSS,
	ROOM_T_FRACAVE,
	ROOM_T_NORMAL,
};

/*!
 * @brief 鍵のかかったドアを配置する
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return なし
 */
void place_locked_door(int y, int x)
{
	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		set_cave_feat(y, x, feat_locked_door_random((d_info[dungeon_type].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
		cave[y][x].info &= ~(CAVE_FLOOR);
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
void place_secret_door(int y, int x, int type)
{
	if (d_info[dungeon_type].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		cave_type *c_ptr = &cave[y][x];

		if (type == DOOR_DEFAULT)
		{
			type = ((d_info[dungeon_type].flags1 & DF1_CURTAIN) &&
			        one_in_((d_info[dungeon_type].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
			        ((d_info[dungeon_type].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
		}

		/* Create secret door */
		place_closed_door(y, x, type);

		if (type != DOOR_CURTAIN)
		{
			/* Hide by inner wall because this is used in rooms only */
			c_ptr->mimic = feat_wall_inner;

			/* Floor type terrain cannot hide a door */
			if (feat_supports_los(c_ptr->mimic) && !feat_supports_los(c_ptr->feat))
			{
				if (have_flag(f_info[c_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[c_ptr->mimic].flags, FF_CAN_FLY))
				{
					c_ptr->feat = one_in_(2) ? c_ptr->mimic : floor_type[randint0(100)];
				}
				c_ptr->mimic = 0;
			}
		}

		c_ptr->info &= ~(CAVE_FLOOR);
		delete_monster(y, x);
	}
}

/*!
 * @brief 1マスだけの部屋を作成し、上下左右いずれか一つに隠しドアを配置する。
 * @param y0 配置したい中心のY座標
 * @param x0 配置したい中心のX座標
 * @details
 * This funtion makes a very small room centred at (x0, y0)
 * This is used in crypts, and random elemental vaults.
 *
 * Note - this should be used only on allocated regions
 * within another room.
 */
void build_small_room(int x0, int y0)
{
	int x, y;

	for (y = y0 - 1; y <= y0 + 1; y++)
	{
		place_inner_bold(y, x0 - 1);
		place_inner_bold(y, x0 + 1);
	}

	for (x = x0 - 1; x <= x0 + 1; x++)
	{
		place_inner_bold(y0 - 1, x);
		place_inner_bold(y0 + 1, x);
	}

	/* Place a secret door on one side */
	switch (randint0(4))
	{
		case 0: place_secret_door(y0, x0 - 1, DOOR_DEFAULT); break;
		case 1: place_secret_door(y0, x0 + 1, DOOR_DEFAULT); break;
		case 2: place_secret_door(y0 - 1, x0, DOOR_DEFAULT); break;
		case 3: place_secret_door(y0 + 1, x0, DOOR_DEFAULT); break;
	}

	/* Clear mimic type */
	cave[y0][x0].mimic = 0;

	/* Add inner open space */
	place_floor_bold(y0, x0);
}

/*!
 * @brief
 * 指定範囲に通路が通っていることを確認した上で床で埋める
 * This function tunnels around a room if it will cut off part of a cave system.
 * @param x1 範囲の左端
 * @param y1 範囲の上端
 * @param x2 範囲の右端
 * @param y2 範囲の下端
 * @return なし
 */
static void check_room_boundary(int x1, int y1, int x2, int y2)
{
	int count, x, y;
	bool old_is_floor, new_is_floor;


	/* Initialize */
	count = 0;

	old_is_floor = get_is_floor(x1 - 1, y1);

	/*
	 * Count the number of floor-wall boundaries around the room
	 * Note: diagonal squares are ignored since the player can move diagonally
	 * to bypass these if needed.
	 */

	/* Above the top boundary */
	for (x = x1; x <= x2; x++)
	{
		new_is_floor = get_is_floor(x, y1 - 1);

		/* Increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Right boundary */
	for (y = y1; y <= y2; y++)
	{
		new_is_floor = get_is_floor(x2 + 1, y);

		/* increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Bottom boundary */
	for (x = x2; x >= x1; x--)
	{
		new_is_floor = get_is_floor(x, y2 + 1);

		/* increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* Left boundary */
	for (y = y2; y >= y1; y--)
	{
		new_is_floor = get_is_floor(x1 - 1, y);

		/* increment counter if they are different */
		if (new_is_floor != old_is_floor) count++;

		old_is_floor = new_is_floor;
	}

	/* If all the same, or only one connection exit. */
	if (count <= 2) return;


	/* Tunnel around the room so to prevent problems with caves */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			set_floor(x, y);
		}
	}
}


/*!
 * @brief
 * find_space()の予備処理として部屋の生成が可能かを判定する /
 * Helper function for find_space(). Is this a good location?
 * @param blocks_high 範囲の高さ
 * @param blocks_wide 範囲の幅
 * @param block_y 範囲の上端
 * @param block_x 範囲の左端
 * @return なし
 */
static bool find_space_aux(int blocks_high, int blocks_wide, int block_y, int block_x)
{
	int by1, bx1, by2, bx2, by, bx;

	/* Itty-bitty rooms must shift about within their rectangle */
	if (blocks_wide < 3)
	{
		if ((blocks_wide == 2) && (block_x % 3) == 2)
			return FALSE;
	}

	/* Rooms with width divisible by 3 must be fitted to a rectangle. */
	else if ((blocks_wide % 3) == 0)
	{
		/* Must be aligned to the left edge of a 11x33 rectangle. */
		if ((block_x % 3) != 0)
			return FALSE;
	}

	/*
	 * Big rooms that do not have a width divisible by 3 must be
	 * aligned towards the edge of the dungeon closest to them.
	 */
	else
	{
		/* Shift towards left edge of dungeon. */
		if (block_x + (blocks_wide / 2) <= dun->col_rooms / 2)
		{
			if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
				return FALSE;
			if ((block_x % 3) == 1)
				return FALSE;
		}

		/* Shift toward right edge of dungeon. */
		else
		{
			if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
				return FALSE;
			if ((block_x % 3) == 1)
				return FALSE;
		}
	}

	/* Extract blocks */
	by1 = block_y + 0;
	bx1 = block_x + 0;
	by2 = block_y + blocks_high;
	bx2 = block_x + blocks_wide;

	/* Never run off the screen */
	if ((by1 < 0) || (by2 > dun->row_rooms)) return FALSE;
	if ((bx1 < 0) || (bx2 > dun->col_rooms)) return FALSE;
	
	/* Verify available space */
	for (by = by1; by < by2; by++)
	{
		for (bx = bx1; bx < bx2; bx++)
		{
			if (dun->room_map[by][bx])
			{
				return FALSE;
			}
		}
	}

	/* This location is okay */
	return TRUE;
}


/*!
 * @brief 部屋生成が可能なスペースを確保する / Find a good spot for the next room.  -LM-
 * @param y 部屋の生成が可能な中心Y座標を返す参照ポインタ
 * @param x 部屋の生成が可能な中心X座標を返す参照ポインタ
 * @param height 確保したい領域の高さ
 * @param width 確保したい領域の幅
 * @return 所定の範囲が確保できた場合TRUEを返す
 * @details
 * Find and allocate a free space in the dungeon large enough to hold\n
 * the room calling this function.\n
 *\n
 * We allocate space in 11x11 blocks, but want to make sure that rooms\n
 * align neatly on the standard screen.  Therefore, we make them use\n
 * blocks in few 11x33 rectangles as possible.\n
 *\n
 * Be careful to include the edges of the room in height and width!\n
 *\n
 * Return TRUE and values for the center of the room if all went well.\n
 * Otherwise, return FALSE.\n
 */
bool find_space(POSITION *y, POSITION *x, POSITION height, POSITION width)
{
	int candidates, pick;
	int by, bx, by1, bx1, by2, bx2;
	int block_y = 0, block_x = 0;


	/* Find out how many blocks we need. */
	int blocks_high = 1 + ((height - 1) / BLOCK_HGT);
	int blocks_wide = 1 + ((width - 1) / BLOCK_WID);

	/* There are no way to allocate such huge space */
	if (dun->row_rooms < blocks_high) return FALSE;
	if (dun->col_rooms < blocks_wide) return FALSE;

	/* Initiallize */
	candidates = 0;

	/* Count the number of valid places */
	for (block_y = dun->row_rooms - blocks_high; block_y >= 0; block_y--)
	{
		for (block_x = dun->col_rooms - blocks_wide; block_x >= 0; block_x--)
		{
			if (find_space_aux(blocks_high, blocks_wide, block_y, block_x))
			{
				/* Find a valid place */
				candidates++;
			}
		}
	}

	/* No place! */
	if (!candidates)
	{
		return FALSE;
	}

	/* Normal dungeon */
	if (!(d_info[dungeon_type].flags1 & DF1_NO_CAVE))
	{
		/* Choose a random one */
		pick = randint1(candidates);
	}

	/* NO_CAVE dungeon (Castle) */
	else
	{
		/* Always choose the center one */
		pick = candidates/2 + 1;
	}

	/* Pick up the choosen location */
	for (block_y = dun->row_rooms - blocks_high; block_y >= 0; block_y--)
	{
		for (block_x = dun->col_rooms - blocks_wide; block_x >= 0; block_x--)
		{
			if (find_space_aux(blocks_high, blocks_wide, block_y, block_x))
			{
				pick--;

				/* This one is picked? */
				if (!pick) break;
			}
		}

		if (!pick) break;
	}

	/* Extract blocks */
	by1 = block_y + 0;
	bx1 = block_x + 0;
	by2 = block_y + blocks_high;
	bx2 = block_x + blocks_wide;

	/*
	 * It is *extremely* important that the following calculation
	 * be *exactly* correct to prevent memory errors
	 */

	/* Acquire the location of the room */
	(*y) = ((by1 + by2) * BLOCK_HGT) / 2;
	(*x) = ((bx1 + bx2) * BLOCK_WID) / 2;

	/* Save the room location */
	if (dun->cent_n < CENT_MAX)
	{
		dun->cent[dun->cent_n].y = (byte_hack)*y;
		dun->cent[dun->cent_n].x = (byte_hack)*x;
		dun->cent_n++;
	}

	/* Reserve some blocks. */
	for (by = by1; by < by2; by++)
	{
		for (bx = bx1; bx < bx2; bx++)
		{
			dun->room_map[by][bx] = TRUE;
		}
	}


	/*
	 * Hack- See if room will cut off a cavern.
	 *
	 * If so, fix by tunneling outside the room in such a
	 * way as to connect the caves.
	 */
	check_room_boundary(*x - width / 2 - 1, *y - height / 2 - 1, *x + (width - 1) / 2 + 1, *y + (height - 1) / 2 + 1);


	/* Success. */
	return TRUE;
}




/*
 * Structure to hold all "fill" data
 */

typedef struct fill_data_type fill_data_type;

struct fill_data_type
{
	/* area size */
	int xmin;
	int ymin;
	int xmax;
	int ymax;

	/* cutoffs */
	int c1;
	int c2;
	int c3;

	/* features to fill with */
	int feat1;
	int feat2;
	int feat3;

	int info1;
	int info2;
	int info3;

	/* number of filled squares */
	int amount;
};

static fill_data_type fill_data;


/* Store routine for the fractal cave generator */
/* this routine probably should be an inline function or a macro. */
static void store_height(int x, int y, int val)
{
	/* if on boundary set val > cutoff so walls are not as square */
	if (((x == fill_data.xmin) || (y == fill_data.ymin) ||
	     (x == fill_data.xmax) || (y == fill_data.ymax)) &&
	    (val <= fill_data.c1)) val = fill_data.c1 + 1;

	/* store the value in height-map format */
	cave[y][x].feat = (s16b)val;

	return;
}


/*
* Explanation of the plasma fractal algorithm:
*
* A grid of points is created with the properties of a 'height-map'
* This is done by making the corners of the grid have a random value.
* The grid is then subdivided into one with twice the resolution.
* The new points midway between two 'known' points can be calculated
* by taking the average value of the 'known' ones and randomly adding
* or subtracting an amount proportional to the distance between those
* points.  The final 'middle' points of the grid are then calculated
* by averaging all four of the originally 'known' corner points.  An
* random amount is added or subtracted from this to get a value of the
* height at that point.  The scaling factor here is adjusted to the
* slightly larger distance diagonally as compared to orthogonally.
*
* This is then repeated recursively to fill an entire 'height-map'
* A rectangular map is done the same way, except there are different
* scaling factors along the x and y directions.
*
* A hack to change the amount of correlation between points is done using
* the grd variable.  If the current step size is greater than grd then
* the point will be random, otherwise it will be calculated by the
* above algorithm.  This makes a maximum distance at which two points on
* the height map can affect each other.
*
* How fractal caves are made:
*
* When the map is complete, a cut-off value is used to create a cave.
* Heights below this value are "floor", and heights above are "wall".
* This also can be used to create lakes, by adding more height levels
* representing shallow and deep water/ lava etc.
*
* The grd variable affects the width of passages.
* The roug variable affects the roughness of those passages
*
* The tricky part is making sure the created cave is connected.  This
* is done by 'filling' from the inside and only keeping the 'filled'
* floor.  Walls bounding the 'filled' floor are also kept.  Everything
* else is converted to the normal _extra_.
 */


/*
 *  Note that this uses the cave.feat array in a very hackish way
 *  the values are first set to zero, and then each array location
 *  is used as a "heightmap"
 *  The heightmap then needs to be converted back into the "feat" format.
 *
 *  grd=level at which fractal turns on.  smaller gives more mazelike caves
 *  roug=roughness level.  16=normal.  higher values make things more convoluted
 *    small values are good for smooth walls.
 *  size=length of the side of the square cave system.
 */
static void generate_hmap(int y0, int x0, int xsiz, int ysiz, int grd, int roug, int cutoff)
{
	int xhsize, yhsize, xsize, ysize, maxsize;

	/*
	 * fixed point variables- these are stored as 256 x normal value
	 * this gives 8 binary places of fractional part + 8 places of normal part
	 */

	u16b xstep, xhstep, ystep, yhstep;
	u16b xstep2, xhstep2, ystep2, yhstep2;
	u16b i, j, ii, jj, diagsize, xxsize, yysize;
	
	/* Cache for speed */
	u16b xm, xp, ym, yp;

	/* redefine size so can change the value if out of range */
	xsize = xsiz;
	ysize = ysiz;

	/* Paranoia about size of the system of caves */
	if (xsize > 254) xsize = 254;
	if (xsize < 4) xsize = 4;
	if (ysize > 254) ysize = 254;
	if (ysize < 4) ysize = 4;

	/* get offsets to middle of array */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	/* fix rounding problem */
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	/* get limits of region */
	fill_data.xmin = x0 - xhsize;
	fill_data.ymin = y0 - yhsize;
	fill_data.xmax = x0 + xhsize;
	fill_data.ymax = y0 + yhsize;

	/* Store cutoff in global for quick access */
	fill_data.c1 = cutoff;

	/*
	* Scale factor for middle points:
	* About sqrt(2) * 256 - correct for a square lattice
	* approximately correct for everything else.
	 */
	diagsize = 362;

	/* maximum of xsize and ysize */
	maxsize = (xsize > ysize) ? xsize : ysize;

	/* Clear the section */
	for (i = 0; i <= xsize; i++)
	{
		for (j = 0; j <= ysize; j++)
		{
			/* -1 is a flag for "not done yet" */
			cave[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].feat = -1;
			/* Clear icky flag because may be redoing the cave */
			cave[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].info &= ~(CAVE_ICKY);
		}
	}

	/* Boundaries are walls */
	cave[fill_data.ymin][fill_data.xmin].feat = (s16b)maxsize;
	cave[fill_data.ymax][fill_data.xmin].feat = (s16b)maxsize;
	cave[fill_data.ymin][fill_data.xmax].feat = (s16b)maxsize;
	cave[fill_data.ymax][fill_data.xmax].feat = (s16b)maxsize;

	/* Set the middle square to be an open area. */
	cave[y0][x0].feat = 0;

	/* Initialize the step sizes */
	xstep = xhstep = xsize * 256;
	ystep = yhstep = ysize * 256;
	xxsize = xsize * 256;
	yysize = ysize * 256;

	/*
	 * Fill in the rectangle with fractal height data -
	 * like the 'plasma fractal' in fractint.
	 */
	while ((xhstep > 256) || (yhstep > 256))
	{
		/* Halve the step sizes */
		xstep = xhstep;
		xhstep /= 2;
		ystep = yhstep;
		yhstep /= 2;

		/* cache well used values */
		xstep2 = xstep / 256;
		ystep2 = ystep / 256;

		xhstep2 = xhstep / 256;
		yhstep2 = yhstep / 256;

		/* middle top to bottom. */
		for (i = xhstep; i <= xxsize - xhstep; i += xstep)
		{
			for (j = 0; j <= yysize; j += ystep)
			{
				/* cache often used values */
				ii = i / 256 + fill_data.xmin;
				jj = j / 256 + fill_data.ymin;

				/* Test square */
				if (cave[jj][ii].feat == -1)
				{
					if (xhstep2 > grd)
					{
						/* If greater than 'grid' level then is random */
						store_height(ii, jj, randint1(maxsize));
					}
					else
					{
						/* Average of left and right points +random bit */
						store_height(ii, jj,
							(cave[jj][fill_data.xmin + (i - xhstep) / 256].feat
							 + cave[jj][fill_data.xmin + (i + xhstep) / 256].feat) / 2
							 + (randint1(xstep2) - xhstep2) * roug / 16);
					}
				}
			}
		}


		/* middle left to right. */
		for (j = yhstep; j <= yysize - yhstep; j += ystep)
		{
			for (i = 0; i <= xxsize; i += xstep)
			{
				/* cache often used values */
				ii = i / 256 + fill_data.xmin;
				jj = j / 256 + fill_data.ymin;

				/* Test square */
				if (cave[jj][ii].feat == -1)
				{
					if (xhstep2 > grd)
					{
						/* If greater than 'grid' level then is random */
						store_height(ii, jj, randint1(maxsize));
					}
					else
					{
						/* Average of up and down points +random bit */
						store_height(ii, jj,
							(cave[fill_data.ymin + (j - yhstep) / 256][ii].feat
							+ cave[fill_data.ymin + (j + yhstep) / 256][ii].feat) / 2
							+ (randint1(ystep2) - yhstep2) * roug / 16);
					}
				}
			}
		}

		/* center. */
		for (i = xhstep; i <= xxsize - xhstep; i += xstep)
		{
			for (j = yhstep; j <= yysize - yhstep; j += ystep)
			{
				/* cache often used values */
				ii = i / 256 + fill_data.xmin;
				jj = j / 256 + fill_data.ymin;

				/* Test square */
				if (cave[jj][ii].feat == -1)
				{
					if (xhstep2 > grd)
					{
						/* If greater than 'grid' level then is random */
						store_height(ii, jj, randint1(maxsize));
					}
					else
					{
						/* Cache reused values. */
						xm = fill_data.xmin + (i - xhstep) / 256;
						xp = fill_data.xmin + (i + xhstep) / 256;
						ym = fill_data.ymin + (j - yhstep) / 256;
						yp = fill_data.ymin + (j + yhstep) / 256;

						/* 
						 * Average over all four corners + scale by diagsize to
						 * reduce the effect of the square grid on the shape of the fractal
						 */
						store_height(ii, jj,
							(cave[ym][xm].feat + cave[yp][xm].feat
							+ cave[ym][xp].feat + cave[yp][xp].feat) / 4
							+ (randint1(xstep2) - xhstep2) * (diagsize / 16) / 256 * roug);
					}
				}
			}
		}
	}
}


static bool hack_isnt_wall(int y, int x, int c1, int c2, int c3, int feat1, int feat2, int feat3, int info1, int info2, int info3)
{
	/*
	 * function used to convert from height-map back to the
	 *  normal angband cave format
	 */
	if (cave[y][x].info & CAVE_ICKY)
	{
		/* already done */
		return FALSE;
	}
	else
	{
		/* Show that have looked at this square */
		cave[y][x].info|= (CAVE_ICKY);

		/* Use cutoffs c1-c3 to allocate regions of floor /water/ lava etc. */
		if (cave[y][x].feat <= c1)
		{
			/* 25% of the time use the other tile : it looks better this way */
			if (randint1(100) < 75)
			{
				cave[y][x].feat = (s16b)feat1;
				cave[y][x].info &= ~(CAVE_MASK);
				cave[y][x].info |= info1;
				return TRUE;
			}
			else
			{
				cave[y][x].feat = (s16b)feat2;
				cave[y][x].info &= ~(CAVE_MASK);
				cave[y][x].info |= info2;
				return TRUE;
			}
		}
		else if (cave[y][x].feat <= c2)
		{
			/* 25% of the time use the other tile : it looks better this way */
			if (randint1(100) < 75)
			{
				cave[y][x].feat = (s16b)feat2;
				cave[y][x].info &= ~(CAVE_MASK);
				cave[y][x].info |= info2;
				return TRUE;
			}
			else
			{
				cave[y][x].feat = (s16b)feat1;
				cave[y][x].info &= ~(CAVE_MASK);
				cave[y][x].info |= info1;
				return TRUE;
			}
		}
		else if (cave[y][x].feat <= c3)
		{
			cave[y][x].feat = (s16b)feat3;
			cave[y][x].info &= ~(CAVE_MASK);
			cave[y][x].info |= info3;
			return TRUE;
		}
		/* if greater than cutoff then is a wall */
		else
		{
			place_outer_bold(y, x);
			return FALSE;
		}
	}
}




/*
 * Quick and nasty fill routine used to find the connected region
 * of floor in the middle of the cave
 */
static void cave_fill(POSITION y, POSITION x)
{
	int i, j, d;
	int ty, tx;

	int flow_tail = 1;
	int flow_head = 0;


	/*** Start Grid ***/

	/* Enqueue that entry */
	temp_y[0] = y;
	temp_x[0] = x;


	/* Now process the queue */
	while (flow_head != flow_tail)
	{
		/* Extract the next entry */
		ty = temp_y[flow_head];
		tx = temp_x[flow_head];

		/* Forget that entry */
		if (++flow_head == TEMP_MAX) flow_head = 0;

		/* Add the "children" */
		for (d = 0; d < 8; d++)
		{
			int old_head = flow_tail;

			/* Child location */
			j = ty + ddy_ddd[d];
			i = tx + ddx_ddd[d];

			/* Paranoia Don't leave the cave */
			if (!in_bounds(j, i))
			{
				/* affect boundary */
				cave[j][i].info |= CAVE_ICKY;
/*				return; */
			}

			/* If within bounds */
			else if ((i > fill_data.xmin) && (i < fill_data.xmax)
				&& (j > fill_data.ymin) && (j < fill_data.ymax))
			{
				/* If not a wall or floor done before */
				if (hack_isnt_wall(j, i,
					fill_data.c1, fill_data.c2, fill_data.c3,
					fill_data.feat1, fill_data.feat2, fill_data.feat3,
					fill_data.info1, fill_data.info2, fill_data.info3))
				{
					/* Enqueue that entry */
					temp_y[flow_tail] = (byte_hack)j;
					temp_x[flow_tail] = (byte_hack)i;

					/* Advance the queue */
					if (++flow_tail == TEMP_MAX) flow_tail = 0;

					/* Hack -- Overflow by forgetting new entry */
					if (flow_tail == flow_head)
					{
						flow_tail = old_head;
					}
					else
					{
						/* keep tally of size of cave system */
						(fill_data.amount)++;
					}
				}
			}
			else
			{
				/* affect boundary */
				cave[j][i].info |= CAVE_ICKY;
			}
		}
	}
}


static bool generate_fracave(int y0, int x0, int xsize, int ysize, int cutoff, bool light, bool room)
{
	int x, y, i, xhsize, yhsize;

	/* offsets to middle from corner */
	xhsize = xsize / 2;
	yhsize = ysize / 2;


	/*
	 * select region connected to center of cave system
	 * this gets rid of alot of isolated one-sqaures that
	 * can make teleport traps instadeaths...
	 */

	/* cutoffs */
	fill_data.c1 = cutoff;
	fill_data.c2 = 0;
	fill_data.c3 = 0;

	/* features to fill with */
	fill_data.feat1 = floor_type[randint0(100)];
	fill_data.feat2 = floor_type[randint0(100)];
	fill_data.feat3 = floor_type[randint0(100)];

	fill_data.info1 = CAVE_FLOOR;
	fill_data.info2 = CAVE_FLOOR;
	fill_data.info3 = CAVE_FLOOR;

	/* number of filled squares */
	fill_data.amount = 0;

	cave_fill((byte)y0, (byte)x0);

	/* if tally too small, try again */
	if (fill_data.amount < 10)
	{
		/* too small - clear area and try again later */
		for (x = 0; x <= xsize; ++x)
		{
			for (y = 0; y <= ysize; ++y)
			{
				place_extra_bold(y0 + y - yhsize, x0 + x - xhsize);
				cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
			}
		}
		return FALSE;
	}

	/*
	 * Do boundarys-check to see if they are next to a filled region
	 * If not then they are set to normal granite
	 * If so then they are marked as room walls.
	 */
	for (i = 0; i <= xsize; ++i)
	{
		/* top boundary */
		if ((cave[0 + y0 - yhsize][i + x0 - xhsize].info & CAVE_ICKY) && (room))
		{
			/* Next to a 'filled' region? - set to be room walls */
			place_outer_bold(y0 + 0 - yhsize, x0 + i - xhsize);
			if (light) cave[y0 + 0 - yhsize][x0 + i - xhsize].info |= (CAVE_GLOW);
			cave[y0 + 0 - yhsize][x0 + i - xhsize].info |= (CAVE_ROOM);
			place_outer_bold(y0 + 0 - yhsize, x0 + i - xhsize);
		}
		else
		{
			/* set to be normal granite */
			place_extra_bold(y0 + 0 - yhsize, x0 + i - xhsize);
		}

		/* bottom boundary */
		if ((cave[ysize + y0 - yhsize][i + x0 - xhsize].info & CAVE_ICKY) && (room))
		{
			/* Next to a 'filled' region? - set to be room walls */
			place_outer_bold(y0 + ysize - yhsize, x0 + i - xhsize);
			if (light) cave[y0 + ysize - yhsize][x0 + i - xhsize].info|=(CAVE_GLOW);
			cave[y0 + ysize - yhsize][x0 + i - xhsize].info|=(CAVE_ROOM);
			place_outer_bold(y0 + ysize - yhsize, x0 + i - xhsize);
		}
		else
		{
			/* set to be normal granite */
			place_extra_bold(y0 + ysize - yhsize, x0 + i - xhsize);
		}

		/* clear the icky flag-don't need it any more */
		cave[y0 + 0 - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
		cave[y0 + ysize - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
	}

	/* Do the left and right boundaries minus the corners (done above) */
	for (i = 1; i < ysize; ++i)
	{
		/* left boundary */
		if ((cave[i + y0 - yhsize][0 + x0 - xhsize].info & CAVE_ICKY) && room)
		{
			/* room boundary */
			place_outer_bold(y0 + i - yhsize, x0 + 0 - xhsize);
			if (light) cave[y0 + i - yhsize][x0 + 0 - xhsize].info |= (CAVE_GLOW);
			cave[y0 + i - yhsize][x0 + 0 - xhsize].info |= (CAVE_ROOM);
			place_outer_bold(y0 + i - yhsize, x0 + 0 - xhsize);
		}
		else
		{
			/* outside room */
			place_extra_bold(y0 + i - yhsize, x0 + 0 - xhsize);
		}
		/* right boundary */
		if ((cave[i + y0 - yhsize][xsize + x0 - xhsize].info & CAVE_ICKY) && room)
		{
			/* room boundary */
			place_outer_bold(y0 + i - yhsize, x0 + xsize - xhsize);
			if (light) cave[y0 + i - yhsize][x0 + xsize - xhsize].info |= (CAVE_GLOW);
			cave[y0 + i - yhsize][x0 + xsize - xhsize].info |= (CAVE_ROOM);
			place_outer_bold(y0 + i - yhsize, x0 + xsize - xhsize);
		}
		else
		{
			/* outside room */
			place_extra_bold(y0 + i - yhsize, x0 + xsize - xhsize);
		}

		/* clear icky flag -done with it */
		cave[y0 + i - yhsize][x0 + 0 - xhsize].info &= ~(CAVE_ICKY);
		cave[y0 + i - yhsize][x0 + xsize - xhsize].info &= ~(CAVE_ICKY);
	}


	/* Do the rest: convert back to the normal format */
	for (x = 1; x < xsize; ++x)
	{
		for (y = 1; y < ysize; ++y)
		{
			if (is_floor_bold(y0 + y - yhsize, x0 + x - xhsize) &&
			    (cave[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY))
			{
				/* Clear the icky flag in the filled region */
				cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~CAVE_ICKY;

				/* Set appropriate flags */
				if (light) cave[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_GLOW);
				if (room) cave[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_ROOM);
			}
			else if (is_outer_bold(y0 + y - yhsize, x0 + x - xhsize) &&
				 (cave[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY))
			{
				/* Walls */
				cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY);
				if (light) cave[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_GLOW);
				if (room)
				{
					cave[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_ROOM);
				}
				else
				{

					place_extra_bold(y0 + y - yhsize, x0 + x - xhsize);
					cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ROOM);
				}
			}
			else
			{
				/* Clear the unconnected regions */
				place_extra_bold(y0 + y - yhsize, x0 + x - xhsize);
				cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
			}
		}
	}

	/*
	 * XXX XXX XXX There is a slight problem when tunnels pierce the caves:
	 * Extra doors appear inside the system.  (Its not very noticeable though.)
	 * This can be removed by "filling" from the outside in.  This allows a separation
	 * from _outer_ with _inner_.  (Internal walls are  _outer_ instead.)
	 * The extra effort for what seems to be only a minor thing (even non-existant if you
	 * think of the caves not as normal rooms, but as holes in the dungeon), doesn't seem
	 * worth it.
	 */

	return TRUE;
}


/*!
 * @brief タイプ9の部屋…フラクタルカーブによる洞窟生成 / Type 9 -- Driver routine to create fractal cave system
 * @return なし
 */
static bool build_type9(void)
{
	int grd, roug, cutoff;
	POSITION xsize, ysize, y0, x0;

	bool done, light, room;

	/* get size: note 'Evenness'*/
	xsize = randint1(22) * 2 + 6;
	ysize = randint1(15) * 2 + 6;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, ysize + 1, xsize + 1))
	{
		/* Limit to the minimum room size, and retry */
		xsize = 8;
		ysize = 8;

		/* Find and reserve some space in the dungeon.  Get center of room. */
		if (!find_space(&y0, &x0, ysize + 1, xsize + 1))
		{
			/*
			 * Still no space?!
			 * Try normal room
			 */
			return build_type1();
		}
	}

	light = done = FALSE;
	room = TRUE;

	if ((dun_level <= randint1(25)) && !(d_info[dungeon_type].flags1 & DF1_DARKNESS)) light = TRUE;

	while (!done)
	{
		/* Note: size must be even or there are rounding problems
		* This causes the tunnels not to connect properly to the room */

		/* testing values for these parameters feel free to adjust */
		grd = 1 << (randint0(4));

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* about size/2 */
		cutoff = randint1(xsize / 4) + randint1(ysize / 4) +
			 randint1(xsize / 4) + randint1(ysize / 4);

		/* make it */
		generate_hmap(y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format + clean up */
		done = generate_fracave(y0, x0, xsize, ysize, cutoff, light, room);
	}

	return TRUE;
}

#ifdef ALLOW_CAVERNS_AND_LAKES
/*
 * Builds a cave system in the center of the dungeon.
 */
void build_cavern(void)
{
	int grd, roug, cutoff, xsize, ysize, x0, y0;
	bool done, light;

	light = done = FALSE;
	if ((dun_level <= randint1(50)) && !(d_info[dungeon_type].flags1 & DF1_DARKNESS)) light = TRUE;

	/* Make a cave the size of the dungeon */
	xsize = cur_wid - 1;
	ysize = cur_hgt - 1;
	x0 = xsize / 2;
	y0 = ysize / 2;

	/* Paranoia: make size even */
	xsize = x0 * 2;
	ysize = y0 * 2;

	while (!done)
	{
		/* testing values for these parameters: feel free to adjust */
		grd = randint1(4) + 4;

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* about size/2 */
		cutoff = xsize / 2;

		 /* make it */
		generate_hmap(y0 + 1, x0 + 1, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format+ clean up */
		done = generate_fracave(y0 + 1, x0 + 1, xsize, ysize, cutoff, light, FALSE);
	}
}

static bool generate_lake(int y0, int x0, int xsize, int ysize, int c1, int c2, int c3, int type)
{
	int x, y, i, xhsize, yhsize;
	int feat1, feat2, feat3;

	/* offsets to middle from corner */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	/* Get features based on type */
	switch (type)
	{
	case LAKE_T_LAVA: /* Lava */
		feat1 = feat_deep_lava;
		feat2 = feat_shallow_lava;
		feat3 = floor_type[randint0(100)];
		break;
	case LAKE_T_WATER: /* Water */
		feat1 = feat_deep_water;
		feat2 = feat_shallow_water;
		feat3 = floor_type[randint0(100)];
		break;
	case LAKE_T_CAVE: /* Collapsed cave */
		feat1 = floor_type[randint0(100)];
		feat2 = floor_type[randint0(100)];
		feat3 = feat_rubble;
		break;
	case LAKE_T_EARTH_VAULT: /* Earth vault */
		feat1 = feat_rubble;
		feat2 = floor_type[randint0(100)];
		feat3 = feat_rubble;
		break;
	case LAKE_T_AIR_VAULT: /* Air vault */
		feat1 = feat_grass;
		feat2 = feat_tree;
		feat3 = feat_grass;
		break;
	case LAKE_T_WATER_VAULT: /* Water vault */
		feat1 = feat_shallow_water;
		feat2 = feat_deep_water;
		feat3 = feat_shallow_water;
		break;
	case LAKE_T_FIRE_VAULT: /* Fire Vault */
		feat1 = feat_shallow_lava;
		feat2 = feat_deep_lava;
		feat3 = feat_shallow_lava;
		break;

	/* Paranoia */
	default: return FALSE;
	}

	/*
	 * select region connected to center of cave system
	 * this gets rid of alot of isolated one-sqaures that
	 * can make teleport traps instadeaths...
	 */

	/* cutoffs */
	fill_data.c1 = c1;
	fill_data.c2 = c2;
	fill_data.c3 = c3;

	/* features to fill with */
	fill_data.feat1 = feat1;
	fill_data.feat2 = feat2;
	fill_data.feat3 = feat3;

	fill_data.info1 = 0;
	fill_data.info2 = 0;
	fill_data.info3 = 0;

	/* number of filled squares */
	fill_data.amount = 0;

	/* select region connected to center of cave system
	* this gets rid of alot of isolated one-sqaures that
	* can make teleport traps instadeaths... */
	cave_fill((byte)y0, (byte)x0);

	/* if tally too small, try again */
	if (fill_data.amount < 10)
	{
		/* too small -clear area and try again later */
		for (x = 0; x <= xsize; ++x)
		{
			for (y = 0; y <= ysize; ++y)
			{
				place_floor_bold(y0 + y - yhsize, x0 + x - xhsize);
				cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY);
			}
		}
		return FALSE;
	}

	/* Do boundarys- set to normal granite */
	for (i = 0; i <= xsize; ++i)
	{
		place_extra_bold(y0 + 0 - yhsize, x0 + i - xhsize);
		place_extra_bold(y0 + ysize - yhsize, x0 + i - xhsize);

		/* clear the icky flag-don't need it any more */
		cave[y0 + 0 - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
		cave[y0 + ysize - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
	}

	/* Do the left and right boundaries minus the corners (done above) */

	for (i = 1; i < ysize; ++i)
	{
		place_extra_bold(y0 + i - yhsize, x0 + 0 - xhsize);
		place_extra_bold(y0 + i - yhsize, x0 + xsize - xhsize);

		/* clear icky flag -done with it */
		cave[y0 + i - yhsize][x0 + 0 - xhsize].info &= ~(CAVE_ICKY);
		cave[y0 + i - yhsize][x0 + xsize - xhsize].info &= ~(CAVE_ICKY);
	}


	/* Do the rest: convert back to the normal format */
	for (x = 1; x < xsize; ++x)
	{
		for (y = 1; y < ysize; ++y)
		{
			/* Fill unconnected regions with granite */
			if ((!(cave[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY)) ||
				is_outer_bold(y0 + y - yhsize, x0 + x - xhsize))
				place_extra_bold(y0 + y - yhsize, x0 + x - xhsize);

			/* turn off icky flag (no longer needed.) */
			cave[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);

			/* Light lava */
			if (cave_have_flag_bold(y0 + y - yhsize, x0 + x - xhsize, FF_LAVA))
			{
				if (!(d_info[dungeon_type].flags1 & DF1_DARKNESS)) cave[y0 + y - yhsize][x0 + x - xhsize].info |= CAVE_GLOW;
			}
		}
	}

	return TRUE;
}


/*
 * makes a lake/collapsed cave system in the center of the dungeon
 */
void build_lake(int type)
{
	int grd, roug, xsize, ysize, x0, y0;
	bool done = FALSE;
	int c1, c2, c3;

	/* paranoia - exit if lake type out of range. */
	if ((type < LAKE_T_LAVA) || (type > LAKE_T_FIRE_VAULT))
	{
		msg_format("Invalid lake type (%d)", type);
		return;
	}

	/* Make the size of the dungeon */
	xsize = cur_wid - 1;
	ysize = cur_hgt - 1;
	x0 = xsize / 2;
	y0 = ysize / 2;

	/* Paranoia: make size even */
	xsize = x0 * 2;
	ysize = y0 * 2;

	while (!done)
	{
		/* testing values for these parameters: feel free to adjust */
		grd = randint1(3) + 4;

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* Make up size of various componants */
		/* Floor */
		c3 = 3 * xsize / 4;

		/* Deep water/lava */
		c1 = randint0(c3 / 2) + randint0(c3 / 2) - 5;

		/* Shallow boundary */
		c2 = (c1 + c3) / 2;

		/* make it */
		generate_hmap(y0 + 1, x0 + 1, xsize, ysize, grd, roug, c3);

		/* Convert to normal format+ clean up */
		done = generate_lake(y0 + 1, x0 + 1, xsize, ysize, c1, c2, c3, type);
	}
}
#endif /* ALLOW_CAVERNS_AND_LAKES */


/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
static void add_door(int x, int y)
{
	/* Need to have a wall in the center square */
	if (!is_outer_bold(y, x)) return;

	/* look at:
	 *  x#x
	 *  .#.
	 *  x#x
	 *
	 *  where x=don't care
	 *  .=floor, #=wall
	 */

	if (is_floor_bold(y-1,x) && is_floor_bold(y+1,x) &&
	    (is_outer_bold(y, x - 1) && is_outer_bold(y, x + 1)))
	{
		/* secret door */
		place_secret_door(y, x, DOOR_DEFAULT);

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
	if (is_outer_bold(y - 1, x) && is_outer_bold(y + 1, x) &&
	    is_floor_bold(y,x-1) && is_floor_bold(y,x+1))
	{
		/* secret door */
		place_secret_door(y, x, DOOR_DEFAULT);

		/* set boundarys so don't get wide doors */
		place_solid_bold(y - 1, x);
		place_solid_bold(y + 1, x);
	}
}


/*
 * Routine that fills the empty areas of a room with treasure and monsters.
 */
static void fill_treasure(int x1, int x2, int y1, int y2, int difficulty)
{
	int x, y, cx, cy, size;
	s32b value;

	/* center of room:*/
	cx = (x1 + x2) / 2;
	cy = (y1 + y2) / 2;

	/* Rough measure of size of vault= sum of lengths of sides */
	size = abs(x2 - x1) + abs(y2 - y1);

	for (x = x1; x <= x2; x++)
	{
		for (y = y1; y <= y2; y++)
		{
			/* Thing added based on distance to center of vault
			 * Difficulty is 1-easy to 10-hard */
			value = ((((s32b)(distance(cx, cy, x, y))) * 100) / size) + randint1(10) - difficulty;

			/* hack- empty square part of the time */
			if ((randint1(100) - difficulty * 3) > 50) value = 20;

			 /* if floor, shallow water and lava */
			if (is_floor_bold(y, x) ||
			    (cave_have_flag_bold(y, x, FF_PLACE) && cave_have_flag_bold(y, x, FF_DROP)))
			{
				/* The smaller 'value' is, the better the stuff */
				if (value < 0)
				{
					/* Meanest monster + treasure */
					monster_level = base_level + 40;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					monster_level = base_level;
					object_level = base_level + 20;
					place_object(y, x, AM_GOOD);
					object_level = base_level;
				}
				else if (value < 5)
				{
					/* Mean monster +treasure */
					monster_level = base_level + 20;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					monster_level = base_level;
					object_level = base_level + 10;
					place_object(y, x, AM_GOOD);
					object_level = base_level;
				}
				else if (value < 10)
				{
					/* Monster */
					monster_level = base_level + 9;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					monster_level = base_level;
				}
				else if (value < 17)
				{
					/* Intentional Blank space */

					/*
					 * (Want some of the vault to be empty
					 * so have room for group monsters.
					 * This is used in the hack above to lower
					 * the density of stuff in the vault.)
					 */
				}
				else if (value < 23)
				{
					/* Object or trap */
					if (randint0(100) < 25)
					{
						place_object(y, x, 0L);
					}
					else
					{
						place_trap(y, x);
					}
				}
				else if (value < 30)
				{
					/* Monster and trap */
					monster_level = base_level + 5;
					place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					monster_level = base_level;
					place_trap(y, x);
				}
				else if (value < 40)
				{
					/* Monster or object */
					if (randint0(100) < 50)
					{
						monster_level = base_level + 3;
						place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
						monster_level = base_level;
					}
					if (randint0(100) < 50)
					{
						object_level = base_level + 7;
						place_object(y, x, 0L);
						object_level = base_level;
					}
				}
				else if (value < 50)
				{
					/* Trap */
					place_trap(y, x);
				}
				else
				{
					/* Various Stuff */

					/* 20% monster, 40% trap, 20% object, 20% blank space */
					if (randint0(100) < 20)
					{
						place_monster(y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
					}
					else if (randint0(100) < 50)
					{
						place_trap(y, x);
					}
					else if (randint0(100) < 50)
					{
						place_object(y, x, 0L);
					}
				}

			}
		}
	}
}


/*
 * This function creates a random vault that looks like a collection of bubbles.
 * It works by getting a set of coordinates that represent the center of each
 * bubble.  The entire room is made by seeing which bubble center is closest. If
 * two centers are equidistant then the square is a wall, otherwise it is a floor.
 * The only exception is for squares really near a center, these are always floor.
 * (It looks better than without this check.)
 *
 * Note: If two centers are on the same point then this algorithm will create a
 *       blank bubble filled with walls. - This is prevented from happening.
 */
static void build_bubble_vault(int x0, int y0, int xsize, int ysize)
{
	#define BUBBLENUM 10		/* number of bubbles */

	/* array of center points of bubbles */
	coord center[BUBBLENUM];

	int i, j, x, y;
	u16b min1, min2, temp;
	bool done;

	/* Offset from center to top left hand corner */
	int xhsize = xsize / 2;
	int yhsize = ysize / 2;

	msg_print_wizard(CHEAT_DUNGEON, _("泡型ランダムVaultを生成しました。", "Room Vault."));

	/* Allocate center of bubbles */
	center[0].x = (byte)randint1(xsize - 3) + 1;
	center[0].y = (byte)randint1(ysize - 3) + 1;

	for (i = 1; i < BUBBLENUM; i++)
	{
		done = FALSE;

		/* get center and check to see if it is unique */
		while (!done)
		{
			done = TRUE;

			x = randint1(xsize - 3) + 1;
			y = randint1(ysize - 3) + 1;

			for (j = 0; j < i; j++)
			{
				/* rough test to see if there is an overlap */
				if ((x == center[j].x) && (y == center[j].y)) done = FALSE;
			}
		}

		center[i].x = (byte_hack)x;
		center[i].y = (byte_hack)y;
	}


	/* Top and bottom boundaries */
	for (i = 0; i < xsize; i++)
	{
		int side_x = x0 - xhsize + i;

		place_outer_noperm_bold(y0 - yhsize + 0, side_x);
		cave[y0 - yhsize + 0][side_x].info |= (CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(y0 - yhsize + ysize - 1, side_x);
		cave[y0 - yhsize + ysize - 1][side_x].info |= (CAVE_ROOM | CAVE_ICKY);
	}

	/* Left and right boundaries */
	for (i = 1; i < ysize - 1; i++)
	{
		int side_y = y0 - yhsize + i;

		place_outer_noperm_bold(side_y, x0 - xhsize + 0);
		cave[side_y][x0 - xhsize + 0].info |= (CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(side_y, x0 - xhsize + xsize - 1);
		cave[side_y][x0 - xhsize + xsize - 1].info |= (CAVE_ROOM | CAVE_ICKY);
	}

	/* Fill in middle with bubbles */
	for (x = 1; x < xsize - 1; x++)
	{
		for (y = 1; y < ysize - 1; y++)
		{
			/* Get distances to two closest centers */

			/* initialize */
			min1 = (u16b)distance(x, y, center[0].x, center[0].y);
			min2 = (u16b)distance(x, y, center[1].x, center[1].y);

			if (min1 > min2)
			{
				/* swap if in wrong order */
				temp = min1;
				min1 = min2;
				min2 = temp;
			}

			/* Scan the rest */
			for (i = 2; i < BUBBLENUM; i++)
			{
				temp = (u16b)distance(x, y, center[i].x, center[i].y);

				if (temp < min1)
				{
					/* smallest */
					min2 = min1;
					min1 = temp;
				}
				else if (temp < min2)
				{
					/* second smallest */
					min2 = temp;
				}
			}
			if (((min2 - min1) <= 2) && (!(min1 < 3)))
			{
				/* Boundary at midpoint+ not at inner region of bubble */
				place_outer_noperm_bold(y0 - yhsize + y, x0 - xhsize + x);
			}
			else
			{
				/* middle of a bubble */
				place_floor_bold(y0 - yhsize + y, x0 - xhsize + x);
			}

			/* clean up rest of flags */
			cave[y0 - yhsize + y][x0 - xhsize + x].info |= (CAVE_ROOM | CAVE_ICKY);
		}
	}

	/* Try to add some random doors */
	for (i = 0; i < 500; i++)
	{
		x = randint1(xsize - 3) - xhsize + x0 + 1;
		y = randint1(ysize - 3) - yhsize + y0 + 1;
		add_door(x, y);
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 2, y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint1(5));
}


/*
 * Overlay a rectangular room given its bounds
 * This routine is used by build_room_vault
 * The area inside the walls is not touched:
 * only granite is removed- normal walls stay
 */
static void build_room(int x1, int x2, int y1, int y2)
{
	int x, y, i, xsize, ysize, temp;

	/* Check if rectangle has no width */
	if ((x1 == x2) || (y1 == y2)) return;

	/* initialize */
	if (x1 > x2)
	{
		/* Swap boundaries if in wrong order */
		temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if (y1 > y2)
	{
		/* Swap boundaries if in wrong order */
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	/* get total widths */
	xsize = x2 - x1;
	ysize = y2 - y1;


	/* Top and bottom boundaries */
	for (i = 0; i <= xsize; i++)
	{
		place_outer_noperm_bold(y1, x1 + i);
		cave[y1][x1 + i].info |= (CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(y2, x1 + i);
		cave[y2][x1 + i].info |= (CAVE_ROOM | CAVE_ICKY);
	}

	/* Left and right boundaries */
	for (i = 1; i < ysize; i++)
	{
		place_outer_noperm_bold(y1 + i, x1);
		cave[y1 + i][x1].info|=(CAVE_ROOM | CAVE_ICKY);
		place_outer_noperm_bold(y1 + i, x2);
		cave[y1 + i][x2].info|=(CAVE_ROOM | CAVE_ICKY);
	}

	/* Middle */
	for (x = 1; x < xsize; x++)
	{
		for (y = 1; y < ysize; y++)
		{
			if (is_extra_bold(y1+y, x1+x))
			{
				/* clear the untouched region */
				place_floor_bold(y1 + y, x1 + x);
				cave[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
			}
			else
			{
				/* make it a room- but don't touch */
				cave[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
			}
		}
	}
}


/* Create a random vault that looks like a collection of overlapping rooms */

static void build_room_vault(int x0, int y0, int xsize, int ysize)
{
	int i, x1, x2, y1, y2, xhsize, yhsize;

	/* get offset from center */
	xhsize = xsize / 2;
	yhsize = ysize / 2;

	msg_print_wizard(CHEAT_DUNGEON, _("部屋型ランダムVaultを生成しました。", "Room Vault."));

	/* fill area so don't get problems with arena levels */
	for (x1 = 0; x1 < xsize; x1++)
	{
		int x = x0 - xhsize + x1;

		for (y1 = 0; y1 < ysize; y1++)
		{
			int y = y0 - yhsize + y1;

			place_extra_bold(y, x);
			cave[y][x].info &= (~CAVE_ICKY);
		}
	}

	/* add ten random rooms */
	for (i = 0; i < 10; i++)
	{
		x1 = randint1(xhsize) * 2 + x0 - xhsize;
		x2 = randint1(xhsize) * 2 + x0 - xhsize;
		y1 = randint1(yhsize) * 2 + y0 - yhsize;
		y2 = randint1(yhsize) * 2 + y0 - yhsize;
		build_room(x1, x2, y1, y2);
	}

	/* Add some random doors */
	for (i = 0; i < 500; i++)
	{
		x1 = randint1(xsize - 3) - xhsize + x0 + 1;
		y1 = randint1(ysize - 3) - yhsize + y0 + 1;
		add_door(x1, y1);
	}

	/* Fill with monsters and treasure, high difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 2, y0 - yhsize + 1, y0 - yhsize + ysize - 2, randint1(5) + 5);
}


/* Create a random vault out of a fractal cave */
static void build_cave_vault(int x0, int y0, int xsiz, int ysiz)
{
	int grd, roug, cutoff, xhsize, yhsize, xsize, ysize, x, y;
	bool done, light, room;

	/* round to make sizes even */
	xhsize = xsiz / 2;
	yhsize = ysiz / 2;
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	msg_print_wizard(CHEAT_DUNGEON, _("洞穴ランダムVaultを生成しました。", "Cave Vault."));

	light = done = FALSE;
	room = TRUE;

	while (!done)
	{
		/* testing values for these parameters feel free to adjust */
		grd = 1 << randint0(4);

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* about size/2 */
		cutoff = randint1(xsize / 4) + randint1(ysize / 4) +
			 randint1(xsize / 4) + randint1(ysize / 4);

		/* make it */
		generate_hmap(y0, x0, xsize, ysize, grd, roug, cutoff);

		/* Convert to normal format+ clean up */
		done = generate_fracave(y0, x0, xsize, ysize, cutoff, light, room);
	}

	/* Set icky flag because is a vault */
	for (x = 0; x <= xsize; x++)
	{
		for (y = 0; y <= ysize; y++)
		{
			cave[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;
		}
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 1, y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}

/*
 * maze vault -- rectangular labyrinthine rooms
 *
 * maze vault uses two routines:
 *    r_visit - a recursive routine that builds the labyrinth
 *    build_maze_vault - a driver routine that calls r_visit and adds
 *                   monsters, traps and treasure
 *
 * The labyrinth is built by creating a spanning tree of a graph.
 * The graph vertices are at
 *    (x, y) = (2j + x1, 2k + y1)   j = 0,...,m-1    k = 0,...,n-1
 * and the edges are the vertical and horizontal nearest neighbors.
 *
 * The spanning tree is created by performing a suitably randomized
 * depth-first traversal of the graph. The only adjustable parameter
 * is the randint0(3) below; it governs the relative density of
 * twists and turns in the labyrinth: smaller number, more twists.
 */
static void r_visit(int y1, int x1, int y2, int x2,
		    int node, int dir, int *visited)
{
	int i, j, m, n, temp, x, y, adj[4];

	/* dimensions of vertex array */
	m = (x2 - x1) / 2 + 1;
	n = (y2 - y1) / 2 + 1;

	/* mark node visited and set it to a floor */
	visited[node] = 1;
	x = 2 * (node % m) + x1;
	y = 2 * (node / m) + y1;
	place_floor_bold(y, x);

	/* setup order of adjacent node visits */
	if (one_in_(3))
	{
		/* pick a random ordering */
		for (i = 0; i < 4; i++)
			adj[i] = i;
		for (i = 0; i < 4; i++)
		{
			j = randint0(4);
			temp = adj[i];
			adj[i] = adj[j];
			adj[j] = temp;
		}
		dir = adj[0];
	}
	else
	{
		/* pick a random ordering with dir first */
		adj[0] = dir;
		for (i = 1; i < 4; i++)
			adj[i] = i;
		for (i = 1; i < 4; i++)
		{
			j = 1 + randint0(3);
			temp = adj[i];
			adj[i] = adj[j];
			adj[j] = temp;
		}
	}

	for (i = 0; i < 4; i++)
	{
		switch (adj[i])
		{
			case 0:
				/* (0,+) - check for bottom boundary */
				if ((node / m < n - 1) && (visited[node + m] == 0))
				{
					place_floor_bold(y + 1, x);
					r_visit(y1, x1, y2, x2, node + m, dir, visited);
				}
				break;
			case 1:
				/* (0,-) - check for top boundary */
				if ((node / m > 0) && (visited[node - m] == 0))
				{
					place_floor_bold(y - 1, x);
					r_visit(y1, x1, y2, x2, node - m, dir, visited);
				}
				break;
			case 2:
				/* (+,0) - check for right boundary */
				if ((node % m < m - 1) && (visited[node + 1] == 0))
				{
					place_floor_bold(y, x + 1);
					r_visit(y1, x1, y2, x2, node + 1, dir, visited);
				}
				break;
			case 3:
				/* (-,0) - check for left boundary */
				if ((node % m > 0) && (visited[node - 1] == 0))
				{
					place_floor_bold(y, x - 1);
					r_visit(y1, x1, y2, x2, node - 1, dir, visited);
				}
		} /* end switch */
	}
}


void build_maze_vault(int x0, int y0, int xsize, int ysize, bool is_vault)
{
	int y, x, dy, dx;
	int y1, x1, y2, x2;
	int m, n, num_vertices, *visited;
	bool light;
	cave_type *c_ptr;

	msg_print_wizard(CHEAT_DUNGEON, _("迷路ランダムVaultを生成しました。", "Maze Vault."));

	/* Choose lite or dark */
	light = ((dun_level <= randint1(25)) && is_vault && !(d_info[dungeon_type].flags1 & DF1_DARKNESS));

	/* Pick a random room size - randomized by calling routine */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;

	/* generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			c_ptr = &cave[y][x];
			c_ptr->info |= CAVE_ROOM;
			if (is_vault) c_ptr->info |= CAVE_ICKY;
			if ((x == x1 - 1) || (x == x2 + 1) || (y == y1 - 1) || (y == y2 + 1))
			{
				place_outer_grid(c_ptr);
			}
			else if (!is_vault)
			{
				place_extra_grid(c_ptr);
			}
			else
			{
				place_inner_grid(c_ptr);
			}
			if (light) c_ptr->info |= (CAVE_GLOW);
		}
	}

	/* dimensions of vertex array */
	m = dx + 1;
	n = dy + 1;
	num_vertices = m * n;

	/* initialize array of visited vertices */
	C_MAKE(visited, num_vertices, int);

	/* traverse the graph to create a spaning tree, pick a random root */
	r_visit(y1, x1, y2, x2, randint0(num_vertices), 0, visited);

	/* Fill with monsters and treasure, low difficulty */
	if (is_vault) fill_treasure(x1, x2, y1, y2, randint1(5));

	C_KILL(visited, num_vertices, int);
}


/* Build a "mini" checkerboard vault
 *
 * This is done by making a permanent wall maze and setting
 * the diagonal sqaures of the checker board to be granite.
 * The vault has two entrances on opposite sides to guarantee
 * a way to get in even if the vault abuts a side of the dungeon.
 */
static void build_mini_c_vault(int x0, int y0, int xsize, int ysize)
{
	int dy, dx;
	int y1, x1, y2, x2, y, x, total;
	int m, n, num_vertices;
	int *visited;

	msg_print_wizard(CHEAT_DUNGEON, _("小型チェッカーランダムVaultを生成しました。", "Mini Checker Board Vault."));

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;


	/* generate the room */
	for (x = x1 - 2; x <= x2 + 2; x++)
	{
		if (!in_bounds(y1-2,x)) break;

		cave[y1-2][x].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y1-2, x);
	}

	for (x = x1 - 2; x <= x2 + 2; x++)
	{
		if (!in_bounds(y2+2,x)) break;

		cave[y2+2][x].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y2+2, x);
	}

	for (y = y1 - 2; y <= y2 + 2; y++)
	{
		if (!in_bounds(y,x1-2)) break;

		cave[y][x1-2].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y, x1-2);
	}

	for (y = y1 - 2; y <= y2 + 2; y++)
	{
		if (!in_bounds(y,x2+2)) break;

		cave[y][x2+2].info |= (CAVE_ROOM | CAVE_ICKY);

		place_outer_noperm_bold(y, x2+2);
	}

	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			c_ptr->info |= (CAVE_ROOM | CAVE_ICKY);

			/* Permanent walls */
			place_inner_perm_grid(c_ptr);
		}
	}


	/* dimensions of vertex array */
	m = dx + 1;
	n = dy + 1;
	num_vertices = m * n;

	/* initialize array of visited vertices */
	C_MAKE(visited, num_vertices, int);

	/* traverse the graph to create a spannng tree, pick a random root */
	r_visit(y1, x1, y2, x2, randint0(num_vertices), 0, visited);

	/* Make it look like a checker board vault */
	for (x = x1; x <= x2; x++)
	{
		for (y = y1; y <= y2; y++)
		{
			total = x - x1 + y - y1;
			/* If total is odd- and is a floor then make a wall */
			if ((total % 2 == 1) && is_floor_bold(y, x))
			{
				place_inner_bold(y, x);
			}
		}
	}

	/* Make a couple of entrances */
	if (one_in_(2))
	{
		/* left and right */
		y = randint1(dy) + dy / 2;
		place_inner_bold(y1 + y, x1 - 1);
		place_inner_bold(y1 + y, x2 + 1);
	}
	else
	{
		/* top and bottom */
		x = randint1(dx) + dx / 2;
		place_inner_bold(y1 - 1, x1 + x);
		place_inner_bold(y2 + 1, x1 + x);
	}

	/* Fill with monsters and treasure, highest difficulty */
	fill_treasure(x1, x2, y1, y2, 10);

	C_KILL(visited, num_vertices, int);
}


/* Build a town/ castle by using a recursive algorithm.
 * Basically divide each region in a probalistic way to create
 * smaller regions.  When the regions get too small stop.
 *
 * The power variable is a measure of how well defended a region is.
 * This alters the possible choices.
 */
static void build_recursive_room(int x1, int y1, int x2, int y2, int power)
{
	int xsize, ysize;
	int x, y;
	int choice;

	/* Temp variables */
	int t1, t2, t3, t4;

	xsize = x2 - x1;
	ysize = y2 - y1;

	if ((power < 3) && (xsize > 12) && (ysize > 12))
	{
		/* Need outside wall +keep */
		choice = 1;
	}
	else
	{
		if (power < 10)
		{
			/* Make rooms + subdivide */
			if ((randint1(10) > 2) && (xsize < 8) && (ysize < 8))
			{
				choice = 4;
			}
			else
			{
				choice = randint1(2) + 1;
			}
		}
		else
		{
			/* Mostly subdivide */
			choice = randint1(3) + 1;
		}
	}

	/* Based on the choice made above, do something */

	switch (choice)
	{
		case 1:
		{
			/* Outer walls */

			/* top and bottom */
			for (x = x1; x <= x2; x++)
			{
				place_outer_bold(y1, x);
				place_outer_bold(y2, x);
			}

			/* left and right */
			for (y = y1 + 1; y < y2; y++)
			{
				place_outer_bold(y, x1);
				place_outer_bold(y, x2);
			}

			/* Make a couple of entrances */
			if (one_in_(2))
			{
				/* left and right */
				y = randint1(ysize) + y1;
				place_floor_bold(y, x1);
				place_floor_bold(y, x2);
			}
			else
			{
				/* top and bottom */
				x = randint1(xsize) + x1;
				place_floor_bold(y1, x);
				place_floor_bold(y2, x);
			}

			/* Select size of keep */
			t1 = randint1(ysize / 3) + y1;
			t2 = y2 - randint1(ysize / 3);
			t3 = randint1(xsize / 3) + x1;
			t4 = x2 - randint1(xsize / 3);

			/* Do outside areas */

			/* Above and below keep */
			build_recursive_room(x1 + 1, y1 + 1, x2 - 1, t1, power + 1);
			build_recursive_room(x1 + 1, t2, x2 - 1, y2, power + 1);

			/* Left and right of keep */
			build_recursive_room(x1 + 1, t1 + 1, t3, t2 - 1, power + 3);
			build_recursive_room(t4, t1 + 1, x2 - 1, t2 - 1, power + 3);

			/* Make the keep itself: */
			x1 = t3;
			x2 = t4;
			y1 = t1;
			y2 = t2;
			xsize = x2 - x1;
			ysize = y2 - y1;
			power += 2;

			/* Fall through */
		}
		case 4:
		{
			/* Try to build a room */
			if ((xsize < 3) || (ysize < 3))
			{
				for (y = y1; y < y2; y++)
				{
					for (x = x1; x < x2; x++)
					{
						place_inner_bold(y, x);
					}
				}

				/* Too small */
				return;
			}

			/* Make outside walls */
			/* top and bottom */
			for (x = x1 + 1; x <= x2 - 1; x++)
			{
				place_inner_bold(y1 + 1, x);
				place_inner_bold(y2 - 1, x);
			}

			/* left and right */
			for (y = y1 + 1; y <= y2 - 1; y++)
			{
				place_inner_bold(y, x1 + 1);
				place_inner_bold(y, x2 - 1);
			}

			/* Make a door */
			y = randint1(ysize - 3) + y1 + 1;

			if (one_in_(2))
			{
				/* left */
				place_floor_bold(y, x1 + 1);
			}
			else
			{
				/* right */
				place_floor_bold(y, x2 - 1);
			}

			/* Build the room */
			build_recursive_room(x1 + 2, y1 + 2, x2 - 2, y2 - 2, power + 3);
			break;
		}
		case 2:
		{
			/* Try and divide vertically */
			if (xsize < 3)
			{
				/* Too small */
				for (y = y1; y < y2; y++)
				{
					for (x = x1; x < x2; x++)
					{
						place_inner_bold(y, x);
					}
				}
				return;
			}

			t1 = randint1(xsize - 2) + x1 + 1;
			build_recursive_room(x1, y1, t1, y2, power - 2);
			build_recursive_room(t1 + 1, y1, x2, y2, power - 2);
			break;
		}
		case 3:
		{
			/* Try and divide horizontally */
			if (ysize < 3)
			{
				/* Too small */
				for (y = y1; y < y2; y++)
				{
					for (x = x1; x < x2; x++)
					{
						place_inner_bold(y, x);
					}
				}
				return;
			}

			t1 = randint1(ysize - 2) + y1 + 1;
			build_recursive_room(x1, y1, x2, t1, power - 2);
			build_recursive_room(x1, t1 + 1, x2, y2, power - 2);
			break;
		}
	}
}


/* Build a castle */

/* Driver routine: clear the region and call the recursive
* room routine.
*
*This makes a vault that looks like a castle/ city in the dungeon.
*/
static void build_castle_vault(int x0, int y0, int xsize, int ysize)
{
	int dy, dx;
	int y1, x1, y2, x2;
	int y, x;

	/* Pick a random room size */
	dy = ysize / 2 - 1;
	dx = xsize / 2 - 1;

	y1 = y0 - dy;
	x1 = x0 - dx;
	y2 = y0 + dy;
	x2 = x0 + dx;

	msg_print_wizard(CHEAT_DUNGEON, _("城型ランダムVaultを生成しました。", "Castle Vault"));

	/* generate the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave[y][x].info |= (CAVE_ROOM | CAVE_ICKY);
			/* Make everything a floor */
			place_floor_bold(y, x);
		}
	}

	/* Make the castle */
	build_recursive_room(x1, y1, x2, y2, randint1(5));

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x1, x2, y1, y2, randint1(3));
}


/*
 * Add outer wall to a floored region
 * Note: no range checking is done so must be inside dungeon
 * This routine also stomps on doors
 */
void add_outer_wall(int x, int y, int light, int x1, int y1, int x2, int y2)
{
	cave_type *c_ptr;
	feature_type *f_ptr;
	int i, j;

	if (!in_bounds(y, x)) return;

	c_ptr = &cave[y][x];

	/* hack- check to see if square has been visited before
	* if so, then exit (use room flag to do this) */
	if (c_ptr->info & CAVE_ROOM) return;

	/* set room flag */
	c_ptr->info |= CAVE_ROOM;

	f_ptr = &f_info[c_ptr->feat];

	if (is_floor_bold(y, x))
	{
		for (i = -1; i <= 1; i++)
		{
			for (j = -1; j <= 1; j++)
			{
				if ((x + i >= x1) && (x + i <= x2) &&
					 (y + j >= y1) && (y + j <= y2))
				{
					add_outer_wall(x + i, y + j, light, x1, y1, x2, y2);
					if (light) c_ptr->info |= CAVE_GLOW;
				}
			}
		}
	}
	else if (is_extra_bold(y, x))
	{
		/* Set bounding walls */
		place_outer_bold(y, x);
		if (light) c_ptr->info |= CAVE_GLOW;
	}
	else if (permanent_wall(f_ptr))
	{
		/* Set bounding walls */
		if (light) c_ptr->info |= CAVE_GLOW;
	}
}


/*
 * Hacked distance formula - gives the 'wrong' answer.
 * Used to build crypts
 */
int dist2(int x1, int y1, int x2, int y2, int h1, int h2, int h3, int h4)
{
	int dx, dy;
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);

	/* Basically this works by taking the normal pythagorean formula
	 * and using an expansion to express this in a way without the
	 * square root.  This approximate formula is then perturbed to give
	 * the distorted results.  (I found this by making a mistake when I was
	 * trying to fix the circular rooms.)
	 */

	/* h1-h4 are constants that describe the metric */
	if (dx >= 2 * dy) return (dx + (dy * h1) / h2);
	if (dy >= 2 * dx) return (dy + (dx * h1) / h2);
	return (((dx + dy) * 128) / 181 +
		(dx * dx / (dy * h3) + dy * dy / (dx * h3)) * h4);
	/* 128/181 is approx. 1/sqrt(2) */
}


/*
 * Build target vault.
 * This is made by two concentric "crypts" with perpendicular
 * walls creating the cross-hairs.
 */
static void build_target_vault(int x0, int y0, int xsize, int ysize)
{
	int rad, x, y;

	/* Make a random metric */
	int h1, h2, h3, h4;
	h1 = randint1(32) - 16;
	h2 = randint1(16);
	h3 = randint1(32);
	h4 = randint1(32) - 16;

	msg_print_wizard(CHEAT_DUNGEON, _("対称形ランダムVaultを生成しました。", "Elemental Vault"));

	/* work out outer radius */
	if (xsize > ysize)
	{
		rad = ysize / 2;
	}
	else
	{
		rad = xsize / 2;
	}

	/* Make floor */
	for (x = x0 - rad; x <= x0 + rad; x++)
	{
		for (y = y0 - rad; y <= y0 + rad; y++)
		{
			/* clear room flag */
			cave[y][x].info &= ~(CAVE_ROOM);

			/* Vault - so is "icky" */
			cave[y][x].info |= CAVE_ICKY;

			if (dist2(y0, x0, y, x, h1, h2, h3, h4) <= rad - 1)
			{
				/* inside- so is floor */
				place_floor_bold(y, x);
			}
			else
			{
				/* make granite outside so arena works */
				place_extra_bold(y, x);
			}

			/* proper boundary for arena */
			if (((y + rad) == y0) || ((y - rad) == y0) ||
			    ((x + rad) == x0) || ((x - rad) == x0))
			{
				place_extra_bold(y, x);
			}
		}
	}

	/* Find visible outer walls and set to be FEAT_OUTER */
	add_outer_wall(x0, y0, FALSE, x0 - rad - 1, y0 - rad - 1,
		       x0 + rad + 1, y0 + rad + 1);

	/* Add inner wall */
	for (x = x0 - rad / 2; x <= x0 + rad / 2; x++)
	{
		for (y = y0 - rad / 2; y <= y0 + rad / 2; y++)
		{
			if (dist2(y0, x0, y, x, h1, h2, h3, h4) == rad / 2)
			{
				/* Make an internal wall */
				place_inner_bold(y, x);
			}
		}
	}

	/* Add perpendicular walls */
	for (x = x0 - rad; x <= x0 + rad; x++)
	{
		place_inner_bold(y0, x);
	}

	for (y = y0 - rad; y <= y0 + rad; y++)
	{
		place_inner_bold(y, x0);
	}

	/* Make inner vault */
	for (y = y0 - 1; y <= y0 + 1; y++)
	{
		place_inner_bold(y, x0 - 1);
		place_inner_bold(y, x0 + 1);
	}
	for (x = x0 - 1; x <= x0 + 1; x++)
	{
		place_inner_bold(y0 - 1, x);
		place_inner_bold(y0 + 1, x);
	}

	place_floor_bold(y0, x0);


	/* Add doors to vault */
	/* get two distances so can place doors relative to centre */
	x = (rad - 2) / 4 + 1;
	y = rad / 2 + x;

	add_door(x0 + x, y0);
	add_door(x0 + y, y0);
	add_door(x0 - x, y0);
	add_door(x0 - y, y0);
	add_door(x0, y0 + x);
	add_door(x0, y0 + y);
	add_door(x0, y0 - x);
	add_door(x0, y0 - y);

	/* Fill with stuff - medium difficulty */
	fill_treasure(x0 - rad, x0 + rad, y0 - rad, y0 + rad, randint1(3) + 3);
}


#ifdef ALLOW_CAVERNS_AND_LAKES
/*
 * This routine uses a modified version of the lake code to make a
 * distribution of some terrain type over the vault.  This type
 * depends on the dungeon depth.
 *
 * Miniture rooms are then scattered across the vault.
 */
static void build_elemental_vault(int x0, int y0, int xsiz, int ysiz)
{
	int grd, roug;
	int c1, c2, c3;
	bool done = FALSE;
	int xsize, ysize, xhsize, yhsize, x, y, i;
	int type;

	msg_print_wizard(CHEAT_DUNGEON, _("精霊界ランダムVaultを生成しました。", "Elemental Vault"));

	/* round to make sizes even */
	xhsize = xsiz / 2;
	yhsize = ysiz / 2;
	xsize = xhsize * 2;
	ysize = yhsize * 2;

	if (dun_level < 25)
	{
		/* Earth vault  (Rubble) */
		type = LAKE_T_EARTH_VAULT;
	}
	else if (dun_level < 50)
	{
		/* Air vault (Trees) */
		type = LAKE_T_AIR_VAULT;
	}
	else if (dun_level < 75)
	{
		/* Water vault (shallow water) */
		type = LAKE_T_WATER_VAULT;
	}
	else
	{
		/* Fire vault (shallow lava) */
		type = LAKE_T_FIRE_VAULT;
	}

	while (!done)
	{
		/* testing values for these parameters: feel free to adjust */
		grd = 1 << (randint0(3));

		/* want average of about 16 */
		roug = randint1(8) * randint1(4);

		/* Make up size of various componants */
		/* Floor */
		c3 = 2 * xsize / 3;

		/* Deep water/lava */
		c1 = randint0(c3 / 2) + randint0(c3 / 2) - 5;

		/* Shallow boundary */
		c2 = (c1 + c3) / 2;

		/* make it */
		generate_hmap(y0, x0, xsize, ysize, grd, roug, c3);

		/* Convert to normal format+ clean up */
		done = generate_lake(y0, x0, xsize, ysize, c1, c2, c3, type);
	}

	/* Set icky flag because is a vault */
	for (x = 0; x <= xsize; x++)
	{
		for (y = 0; y <= ysize; y++)
		{
			cave[y0 - yhsize + y][x0 - xhsize + x].info |= CAVE_ICKY;
		}
	}

	/* make a few rooms in the vault */
	for (i = 1; i <= (xsize * ysize) / 50; i++)
	{
		build_small_room(x0 + randint0(xsize - 4) - xsize / 2 + 2,
				 y0 + randint0(ysize - 4) - ysize / 2 + 2);
	}

	/* Fill with monsters and treasure, low difficulty */
	fill_treasure(x0 - xhsize + 1, x0 - xhsize + xsize - 1,
		      y0 - yhsize + 1, y0 - yhsize + ysize - 1, randint1(5));
}
#endif /* ALLOW_CAVERNS_AND_LAKES */


/*!
 * @brief タイプ10の部屋…ランダム生成vault / Type 10 -- Random vaults
 * @return なし
 */
static bool build_type10(void)
{
	POSITION y0, x0, xsize, ysize, vtype;

	/* Get size */
	/* big enough to look good, small enough to be fairly common. */
	xsize = randint1(22) + 22;
	ysize = randint1(11) + 11;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, ysize + 1, xsize + 1)) return FALSE;

	/* Select type of vault */
#ifdef ALLOW_CAVERNS_AND_LAKES
	do
	{
		vtype = randint1(15);
	}
	while ((d_info[dungeon_type].flags1 & DF1_NO_CAVE) &&
		((vtype == 1) || (vtype == 3) || (vtype == 8) || (vtype == 9) || (vtype == 11)));
#else /* ALLOW_CAVERNS_AND_LAKES */
	do
	{
		vtype = randint1(7);
	}
	while ((d_info[dungeon_type].flags1 & DF1_NO_CAVE) &&
		((vtype == 1) || (vtype == 3)));
#endif /* ALLOW_CAVERNS_AND_LAKES */

	switch (vtype)
	{
		/* Build an appropriate room */
		case 1: case  9: build_bubble_vault(x0, y0, xsize, ysize); break;
		case 2: case 10: build_room_vault(x0, y0, xsize, ysize); break;
		case 3: case 11: build_cave_vault(x0, y0, xsize, ysize); break;
		case 4: case 12: build_maze_vault(x0, y0, xsize, ysize, TRUE); break;
		case 5: case 13: build_mini_c_vault(x0, y0, xsize, ysize); break;
		case 6: case 14: build_castle_vault(x0, y0, xsize, ysize); break;
		case 7: case 15: build_target_vault(x0, y0, xsize, ysize); break;
#ifdef ALLOW_CAVERNS_AND_LAKES
		case 8: build_elemental_vault(x0, y0, xsize, ysize); break;
#endif /* ALLOW_CAVERNS_AND_LAKES */
		/* I know how to add a few more... give me some time. */

		/* Paranoia */
		default: return FALSE;
	}

	return TRUE;
}



/*
 * Helper function for "glass room"
 */
static bool vault_aux_lite(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Validate the monster */
	if (!vault_monster_okay(r_idx)) return FALSE;

	/* Require lite attack */
	if (!(r_ptr->flags4 & RF4_BR_LITE) && !(r_ptr->a_ability_flags1 & RF5_BA_LITE)) return FALSE;

	/* No wall passing monsters */
	if (r_ptr->flags2 & (RF2_PASS_WALL | RF2_KILL_WALL)) return FALSE;

	/* No disintegrating monsters */
	if (r_ptr->flags4 & RF4_BR_DISI) return FALSE;

	return TRUE;
}

/*
 * Helper function for "glass room"
 */
static bool vault_aux_shards(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Validate the monster */
	if (!vault_monster_okay(r_idx)) return FALSE;

	/* Require shards breath attack */
	if (!(r_ptr->flags4 & RF4_BR_SHAR)) return FALSE;

	return TRUE;
}

/*
 * Hack -- determine if a template is potion
 */
static bool kind_is_potion(KIND_OBJECT_IDX k_idx)
{
	return k_info[k_idx].tval == TV_POTION;
}

/*!
 * @brief タイプ15の部屋…ガラス部屋の生成 / Type 15 -- glass rooms
 * @return なし
 */
static bool build_type15(void)
{
	POSITION y, x, y2, x2, yval, xval;
	POSITION y1, x1, xsize, ysize;
	bool light;

	cave_type *c_ptr;

	/* Pick a room size */
	xsize = rand_range(9, 13);
	ysize = rand_range(9, 13);

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&yval, &xval, ysize + 2, xsize + 2)) return FALSE;

	/* Choose lite or dark */
	light = ((dun_level <= randint1(25)) && !(d_info[dungeon_type].flags1 & DF1_DARKNESS));

	/* Get corner values */
	y1 = yval - ysize / 2;
	x1 = xval - xsize / 2;
	y2 = yval + (ysize - 1) / 2;
	x2 = xval + (xsize - 1) / 2;

	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			c_ptr = &cave[y][x];
			place_floor_grid(c_ptr);
			c_ptr->feat = feat_glass_floor;
			c_ptr->info |= (CAVE_ROOM);
			if (light) c_ptr->info |= (CAVE_GLOW);
		}
	}

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		c_ptr = &cave[y][x1 - 1];
		place_outer_grid(c_ptr);
		c_ptr->feat = feat_glass_wall;
		c_ptr = &cave[y][x2 + 1];
		place_outer_grid(c_ptr);
		c_ptr->feat = feat_glass_wall;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		c_ptr = &cave[y1 - 1][x];
		place_outer_grid(c_ptr);
		c_ptr->feat = feat_glass_wall;
		c_ptr = &cave[y2 + 1][x];
		place_outer_grid(c_ptr);
		c_ptr->feat = feat_glass_wall;
	}

	switch (randint1(3))
	{
	case 1: /* 4 lite breathers + potion */
		{
			int dir1, dir2;

			/* Prepare allocation table */
			get_mon_num_prep(vault_aux_lite, NULL);

			/* Place fixed lite berathers */
			for (dir1 = 4; dir1 < 8; dir1++)
			{
				MONRACE_IDX r_idx = get_mon_num(dun_level);

				y = yval + 2 * ddy_ddd[dir1];
				x = xval + 2 * ddx_ddd[dir1];
				if (r_idx) place_monster_aux(0, y, x, r_idx, PM_ALLOW_SLEEP);

				/* Walls around the breather */
				for (dir2 = 0; dir2 < 8; dir2++)
				{
					c_ptr = &cave[y + ddy_ddd[dir2]][x + ddx_ddd[dir2]];
					place_inner_grid(c_ptr);
					c_ptr->feat = feat_glass_wall;
				}
			}

			/* Walls around the potion */
			for (dir1 = 0; dir1 < 4; dir1++)
			{
				y = yval + 2 * ddy_ddd[dir1];
				x = xval + 2 * ddx_ddd[dir1];
				c_ptr = &cave[y][x];
				place_inner_perm_grid(c_ptr);
				c_ptr->feat = feat_permanent_glass_wall;
				cave[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]].info |= (CAVE_ICKY);
			}

			/* Glass door */
			dir1 = randint0(4);
			y = yval + 2 * ddy_ddd[dir1];
			x = xval + 2 * ddx_ddd[dir1];
			place_secret_door(y, x, DOOR_GLASS_DOOR);
			c_ptr = &cave[y][x];
			if (is_closed_door(c_ptr->feat)) c_ptr->mimic = feat_glass_wall;

			/* Place a potion */
			get_obj_num_hook = kind_is_potion;
			place_object(yval, xval, AM_NO_FIXED_ART);
			cave[yval][xval].info |= (CAVE_ICKY);
		}
		break;

	case 2: /* 1 lite breather + random object */
		{
			MONRACE_IDX r_idx;
			DIRECTION dir1;

			/* Pillars */
			c_ptr = &cave[y1 + 1][x1 + 1];
			place_inner_grid(c_ptr);
			c_ptr->feat = feat_glass_wall;

			c_ptr = &cave[y1 + 1][x2 - 1];
			place_inner_grid(c_ptr);
			c_ptr->feat = feat_glass_wall;

			c_ptr = &cave[y2 - 1][x1 + 1];
			place_inner_grid(c_ptr);
			c_ptr->feat = feat_glass_wall;

			c_ptr = &cave[y2 - 1][x2 - 1];
			place_inner_grid(c_ptr);
			c_ptr->feat = feat_glass_wall;

			/* Prepare allocation table */
			get_mon_num_prep(vault_aux_lite, NULL);

			r_idx = get_mon_num(dun_level);
			if (r_idx) place_monster_aux(0, yval, xval, r_idx, 0L);

			/* Walls around the breather */
			for (dir1 = 0; dir1 < 8; dir1++)
			{
				c_ptr = &cave[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
			}

			/* Curtains around the breather */
			for (y = yval - 1; y <= yval + 1; y++)
			{
				place_closed_door(y, xval - 2, DOOR_CURTAIN);
				place_closed_door(y, xval + 2, DOOR_CURTAIN);
			}
			for (x = xval - 1; x <= xval + 1; x++)
			{
				place_closed_door(yval - 2, x, DOOR_CURTAIN);
				place_closed_door(yval + 2, x, DOOR_CURTAIN);
			}

			/* Place an object */
			place_object(yval, xval, AM_NO_FIXED_ART);
			cave[yval][xval].info |= (CAVE_ICKY);
		}
		break;

	case 3: /* 4 shards breathers + 2 potions */
		{
			int dir1;

			/* Walls around the potion */
			for (y = yval - 2; y <= yval + 2; y++)
			{
				c_ptr = &cave[y][xval - 3];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
				c_ptr = &cave[y][xval + 3];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
			}
			for (x = xval - 2; x <= xval + 2; x++)
			{
				c_ptr = &cave[yval - 3][x];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
				c_ptr = &cave[yval + 3][x];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
			}
			for (dir1 = 4; dir1 < 8; dir1++)
			{
				c_ptr = &cave[yval + 2 * ddy_ddd[dir1]][xval + 2 * ddx_ddd[dir1]];
				place_inner_grid(c_ptr);
				c_ptr->feat = feat_glass_wall;
			}

			/* Prepare allocation table */
			get_mon_num_prep(vault_aux_shards, NULL);

			/* Place shard berathers */
			for (dir1 = 4; dir1 < 8; dir1++)
			{
				MONRACE_IDX r_idx = get_mon_num(dun_level);

				y = yval + ddy_ddd[dir1];
				x = xval + ddx_ddd[dir1];
				if (r_idx) place_monster_aux(0, y, x, r_idx, 0L);
			}

			/* Place two potions */
			if (one_in_(2))
			{
				get_obj_num_hook = kind_is_potion;
				place_object(yval, xval - 1, AM_NO_FIXED_ART);
				get_obj_num_hook = kind_is_potion;
				place_object(yval, xval + 1, AM_NO_FIXED_ART);
			}
			else
			{
				get_obj_num_hook = kind_is_potion;
				place_object(yval - 1, xval, AM_NO_FIXED_ART);
				get_obj_num_hook = kind_is_potion;
				place_object(yval + 1, xval, AM_NO_FIXED_ART);
			}

			for (y = yval - 2; y <= yval + 2; y++)
				for (x = xval - 2; x <= xval + 2; x++)
					cave[y][x].info |= (CAVE_ICKY);

		}
		break;
	}

	msg_print_wizard(CHEAT_DUNGEON, _("ガラスの部屋が生成されました。", "Glass room was generated."));

	return TRUE;
}


/* Create a new floor room with optional light */
void generate_room_floor(int y1, int x1, int y2, int x2, int light)
{
	int y, x;
	
	cave_type *c_ptr;

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			/* Point to grid */
			c_ptr = &cave[y][x];
			place_floor_grid(c_ptr);
			c_ptr->info |= (CAVE_ROOM);
			if (light) c_ptr->info |= (CAVE_GLOW);
		}
	}
}

void generate_fill_perm_bold(int y1, int x1, int y2, int x2)
{
	int y, x;

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			/* Point to grid */
			place_inner_perm_bold(y, x);
		}
	}
}


/*!
 * @brief 与えられた部屋型IDに応じて部屋の生成処理分岐を行い結果を返す / Attempt to build a room of the given type at the given block
 * @param type 部屋型ID
 * @note that we restrict the number of "crowded" rooms to reduce the chance of overflowing the monster list during level creation.
 * @return 部屋の精製に成功した場合 TRUE を返す。
 */
static bool room_build(int typ)
{
	/* Build a room */
	switch (typ)
	{
	/* Build an appropriate room */
	case ROOM_T_NORMAL:        return build_type1();
	case ROOM_T_OVERLAP:       return build_type2();
	case ROOM_T_CROSS:         return build_type3();
	case ROOM_T_INNER_FEAT:    return build_type4();
	case ROOM_T_NEST:          return build_type5();
	case ROOM_T_PIT:           return build_type6();
	case ROOM_T_LESSER_VAULT:  return build_type7();
	case ROOM_T_GREATER_VAULT: return build_type8();
	case ROOM_T_FRACAVE:       return build_type9();
	case ROOM_T_RANDOM_VAULT:  return build_type10();
	case ROOM_T_OVAL:          return build_type11();
	case ROOM_T_CRYPT:         return build_type12();
	case ROOM_T_TRAP_PIT:      return build_type13();
	case ROOM_T_TRAP:          return build_type14();
	case ROOM_T_GLASS:         return build_type15();
	case ROOM_T_ARCADE:        return build_type16();
	}

	/* Paranoia */
	return FALSE;
}

/*!
 * @brief 指定した部屋の生成確率を別の部屋に加算し、指定した部屋の生成率を0にする
 * @param dst 確率を移す先の部屋種ID
 * @param src 確率を与える元の部屋種ID
 */
#define MOVE_PLIST(dst, src) (prob_list[dst] += prob_list[src], prob_list[src] = 0) 

/*!
 * @brief 部屋生成処理のメインルーチン(Sangbandを経由してOangbandからの実装を引用) / Generate rooms in dungeon.  Build bigger rooms at first.　[from SAngband (originally from OAngband)]
 * @return 部屋生成に成功した場合 TRUE を返す。
 */
bool generate_rooms(void)
{
	int i;
	bool remain;
	int crowded = 0;
	int total_prob;
	int prob_list[ROOM_T_MAX];
	int rooms_built = 0;
	int area_size = 100 * (cur_hgt*cur_wid) / (MAX_HGT*MAX_WID);
	int level_index = MIN(10, div_round(dun_level, 10));

	/* Number of each type of room on this level */
	s16b room_num[ROOM_T_MAX];

	/* Limit number of rooms */
	int dun_rooms = DUN_ROOMS_MAX * area_size / 100;

	/* Assume normal cave */
	room_info_type *room_info_ptr = room_info_normal;

	/*
	 * Initialize probability list.
	 */
	for (i = 0; i < ROOM_T_MAX; i++)
	{
		/* No rooms allowed above their minimum depth. */
		if (dun_level < room_info_ptr[i].min_level)
		{
			prob_list[i] = 0;
		}
		else
		{
			prob_list[i] = room_info_ptr[i].prob[level_index];
		}
	}

	/*
	 * XXX -- Various dungeon types and options.
	 */

	/*! @details ダンジョンにBEGINNER、CHAMELEON、SMALLESTいずれのフラグもなく、かつ「常に通常でない部屋を生成する」フラグがONならば、GRATER_VAULTのみを生成対象とする。 / Ironman sees only Greater Vaults */
	if (ironman_rooms && !((d_info[dungeon_type].flags1 & (DF1_BEGINNER | DF1_CHAMELEON | DF1_SMALLEST))))
	{
		for (i = 0; i < ROOM_T_MAX; i++)
		{
			if (i == ROOM_T_GREATER_VAULT) prob_list[i] = 1;
			else prob_list[i] = 0;
		}
	}

	/*! @details ダンジョンにNO_VAULTフラグがあるならば、LESSER_VAULT / GREATER_VAULT/ RANDOM_VAULTを除外 / Forbidden vaults */
	else if (d_info[dungeon_type].flags1 & DF1_NO_VAULT)
	{
		prob_list[ROOM_T_LESSER_VAULT] = 0;
		prob_list[ROOM_T_GREATER_VAULT] = 0;
		prob_list[ROOM_T_RANDOM_VAULT] = 0;
	}

	/*! @details ダンジョンにNO_CAVEフラグがある場合、FRACAVEの生成枠がNORMALに与えられる。CRIPT、OVALの生成枠がINNER_Fに与えられる。/ NO_CAVE dungeon (Castle)*/
	if (d_info[dungeon_type].flags1 & DF1_NO_CAVE)
	{
		MOVE_PLIST(ROOM_T_NORMAL, ROOM_T_FRACAVE);
		MOVE_PLIST(ROOM_T_INNER_FEAT, ROOM_T_CRYPT);
		MOVE_PLIST(ROOM_T_INNER_FEAT, ROOM_T_OVAL);
	}

	/*! @details ダンジョンにCAVEフラグがある場合、NORMALの生成枠がFRACAVEに与えられる。/ CAVE dungeon (Orc cave etc.) */
	else if (d_info[dungeon_type].flags1 & DF1_CAVE)
	{
		MOVE_PLIST(ROOM_T_FRACAVE, ROOM_T_NORMAL);
	}

	/*! @details ダンジョンの基本地形が最初から渓谷かアリーナ型の場合 FRACAVE は生成から除外。 /  No caves when a (random) cavern exists: they look bad */
	else if (dun->cavern || dun->empty_level)
	{
		prob_list[ROOM_T_FRACAVE] = 0;
	}

	/*! @details ダンジョンに最初からGLASS_ROOMフラグがある場合、GLASS を生成から除外。/ Forbidden glass rooms */
	if (!(d_info[dungeon_type].flags1 & DF1_GLASS_ROOM))
	{
		prob_list[ROOM_T_GLASS] = 0;
	}

	/*! @details ARCADEは同フラグがダンジョンにないと生成されない。 / Forbidden glass rooms */
	if (!(d_info[dungeon_type].flags1 & DF1_ARCADE))
	{
		prob_list[ROOM_T_ARCADE] = 0;
	}

	/*
	 * Initialize number of rooms,
	 * And calcurate total probability.
	 */
	for (total_prob = 0, i = 0; i < ROOM_T_MAX; i++)
	{
		room_num[i] = 0;
		total_prob += prob_list[i];
	}

	/*
	 * Prepare the number of rooms, of all types, we should build
	 * on this level.
	 */
	for (i = dun_rooms; i > 0; i--)
	{
		int room_type;
		int rand = randint0(total_prob);

		/* Get room_type randomly */
		for (room_type = 0; room_type < ROOM_T_MAX; room_type++)
		{
			if (rand < prob_list[room_type]) break;
			else rand -= prob_list[room_type];
		}

		/* Paranoia */
		if (room_type >= ROOM_T_MAX) room_type = ROOM_T_NORMAL;

		/* Increase the number of rooms of that type we should build. */
		room_num[room_type]++;

		switch (room_type)
		{
		case ROOM_T_NEST:
		case ROOM_T_PIT:
		case ROOM_T_LESSER_VAULT:
		case ROOM_T_TRAP_PIT:
		case ROOM_T_GLASS:
		case ROOM_T_ARCADE:

			/* Large room */
			i -= 2;
			break;

		case ROOM_T_GREATER_VAULT:
		case ROOM_T_RANDOM_VAULT:

			/* Largest room */
			i -= 3;
			break;
		}
	}

	/*
	 * Build each type of room one by one until we cannot build any more.
	 * [from SAngband (originally from OAngband)]
	 */
	while (TRUE)
	{
		/* Assume no remaining rooms */
		remain = FALSE;

		for (i = 0; i < ROOM_T_MAX; i++)
		{
			/* What type of room are we building now? */
			int room_type = room_build_order[i];

			/* Go next if none available */
			if (!room_num[room_type]) continue;

			/* Use up one unit */
			room_num[room_type]--;

			/* Build the room. */
			if (room_build(room_type))
			{
				/* Increase the room built count. */
				rooms_built++;

				/* Mark as there was some remaining rooms */
				remain = TRUE;

				switch (room_type)
				{
				case ROOM_T_PIT:
				case ROOM_T_NEST:
				case ROOM_T_TRAP_PIT:

					/* Avoid too many monsters */
					if (++crowded >= 2)
					{
						room_num[ROOM_T_PIT] = 0;
						room_num[ROOM_T_NEST] = 0;
						room_num[ROOM_T_TRAP_PIT] = 0;
					}
					break;

				case ROOM_T_ARCADE:

					/* Avoid double-town */
					room_num[ROOM_T_ARCADE] = 0;
					break;
				}
			}
		}

		/* End loop if no room remain */
		if (!remain) break;
	}

	/*! @details 部屋生成数が2未満の場合生成失敗を返す */
	if (rooms_built < 2)
	{
		msg_format_wizard(CHEAT_DUNGEON, _("部屋数が2未満でした。生成を再試行します。", "Number of rooms was under 2. Retry."), rooms_built);
		return FALSE;
	}

	msg_format_wizard(CHEAT_DUNGEON, _("このダンジョンの部屋数は %d です。", "Number of Rooms: %d"), rooms_built);

	return TRUE;
}
