/*!
 * @file wild.c
 * @brief 荒野マップの生成とルール管理 / Wilderness generation
 * @date 2014/02/13
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "util.h"

#include "dungeon.h"
#include "floor.h"
#include "wild.h"
#include "world.h"
#include "monster.h"
#include "realm-hex.h"
#include "player-status.h"
#include "player-effects.h"
#include "grid.h"
#include "monster-status.h"
#include "quest.h"
#include "dungeon-file.h"
#include "files.h"
#include "feature.h"

 /*
  * Wilderness
  */
wilderness_type **wilderness;


/*!
 * @brief 地形生成確率を決める要素100の配列を確率テーブルから作成する
 * @param feat_type 非一様確率を再現するための要素数100の配列
 * @param prob 元の確率テーブル
 * @return なし
 */
static void set_floor_and_wall_aux(s16b feat_type[100], feat_prob prob[DUNGEON_FEAT_PROB_NUM])
{
	int lim[DUNGEON_FEAT_PROB_NUM], cur = 0, i;

	lim[0] = prob[0].percent;
	for (i = 1; i < DUNGEON_FEAT_PROB_NUM; i++) lim[i] = lim[i - 1] + prob[i].percent;
	if (lim[DUNGEON_FEAT_PROB_NUM - 1] < 100) lim[DUNGEON_FEAT_PROB_NUM - 1] = 100;

	for (i = 0; i < 100; i++)
	{
		while (i == lim[cur]) cur++;
		feat_type[i] = prob[cur].feat;
	}
}

/*!
 * @brief ダンジョンの地形を指定確率に応じて各マスへランダムに敷き詰める
 * / Fill the arrays of floors and walls in the good proportions
 * @param type ダンジョンID
 * @return なし
 */
void set_floor_and_wall(DUNGEON_IDX type)
{
	DUNGEON_IDX cur_type = 255;
	dungeon_type *d_ptr;

	/* Already filled */
	if (cur_type == type) return;

	cur_type = type;
	d_ptr = &d_info[type];

	set_floor_and_wall_aux(feat_ground_type, d_ptr->floor);
	set_floor_and_wall_aux(feat_wall_type, d_ptr->fill);

	feat_wall_outer = d_ptr->outer_wall;
	feat_wall_inner = d_ptr->inner_wall;
	feat_wall_solid = d_ptr->outer_wall;
}


/*!
 * @brief プラズマフラクタル的地形生成の再帰中間処理
 * / Helper for plasma generation.
 * @param x1 左上端の深み
 * @param x2 右上端の深み
 * @param x3 左下端の深み
 * @param x4 右下端の深み
 * @param xmid 中央座標X
 * @param ymid 中央座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 * @return なし
 */
static void perturb_point_mid(FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, FEAT_IDX x4, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
	/*
	 * Average the four corners & perturb it a bit.
	 * tmp is a random int +/- rough
	 */
	FEAT_IDX tmp2 = rough*2 + 1;
	FEAT_IDX tmp = randint1(tmp2) - (rough + 1);

	FEAT_IDX avg = ((x1 + x2 + x3 + x4) / 4) + tmp;

	/* Division always rounds down, so we round up again */
	if (((x1 + x2 + x3 + x4) % 4) > 1)
		avg++;

	/* Normalize */
	if (avg < 0) avg = 0;
	if (avg > depth_max) avg = depth_max;

	/* Set the new value. */
	current_floor_ptr->grid_array[ymid][xmid].feat = (FEAT_IDX)avg;
}


/*!
 * @brief プラズマフラクタル的地形生成の再帰末端処理
 * / Helper for plasma generation.
 * @param x1 中間末端部1の重み
 * @param x2 中間末端部2の重み
 * @param x3 中間末端部3の重み
 * @param xmid 最終末端部座標X
 * @param ymid 最終末端部座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 * @return なし
 */
static void perturb_point_end(FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
	/*
	 * Average the three corners & perturb it a bit.
	 * tmp is a random int +/- rough
	 */
	FEAT_IDX tmp2 = rough * 2 + 1;
	FEAT_IDX tmp = randint0(tmp2) - rough;

	FEAT_IDX avg = ((x1 + x2 + x3) / 3) + tmp;

	/* Division always rounds down, so we round up again */
	if ((x1 + x2 + x3) % 3) avg++;

	/* Normalize */
	if (avg < 0) avg = 0;
	if (avg > depth_max) avg = depth_max;

	/* Set the new value. */
	current_floor_ptr->grid_array[ymid][xmid].feat = (FEAT_IDX)avg;
}


/*!
 * @brief プラズマフラクタル的地形生成の開始処理
 * / Helper for plasma generation.
 * @param x1 処理範囲の左上X座標
 * @param y1 処理範囲の左上Y座標
 * @param x2 処理範囲の右下X座標
 * @param y2 処理範囲の右下Y座標
 * @param depth_max 深みの最大値
 * @param rough ランダム幅
 * @return なし
 * @details
 * <pre>
 * A generic function to generate the plasma fractal.
 * Note that it uses ``cave_feat'' as temporary storage.
 * The values in ``cave_feat'' after this function
 * are NOT actual features; They are raw heights which
 * need to be converted to features.
 * </pre>
 */
static void plasma_recursive(POSITION x1, POSITION y1, POSITION x2, POSITION y2, FEAT_IDX depth_max, FEAT_IDX rough)
{
	/* Find middle */
	POSITION xmid = (x2 - x1) / 2 + x1;
	POSITION ymid = (y2 - y1) / 2 + y1;

	/* Are we done? */
	if (x1 + 1 == x2) return;

	perturb_point_mid(current_floor_ptr->grid_array[y1][x1].feat, current_floor_ptr->grid_array[y2][x1].feat, current_floor_ptr->grid_array[y1][x2].feat,
		current_floor_ptr->grid_array[y2][x2].feat, xmid, ymid, rough, depth_max);

	perturb_point_end(current_floor_ptr->grid_array[y1][x1].feat, current_floor_ptr->grid_array[y1][x2].feat, current_floor_ptr->grid_array[ymid][xmid].feat,
		xmid, y1, rough, depth_max);

	perturb_point_end(current_floor_ptr->grid_array[y1][x2].feat, current_floor_ptr->grid_array[y2][x2].feat, current_floor_ptr->grid_array[ymid][xmid].feat,
		x2, ymid, rough, depth_max);

	perturb_point_end(current_floor_ptr->grid_array[y2][x2].feat, current_floor_ptr->grid_array[y2][x1].feat, current_floor_ptr->grid_array[ymid][xmid].feat,
		xmid, y2, rough, depth_max);

	perturb_point_end(current_floor_ptr->grid_array[y2][x1].feat, current_floor_ptr->grid_array[y1][x1].feat, current_floor_ptr->grid_array[ymid][xmid].feat,
		x1, ymid, rough, depth_max);


	/* Recurse the four quadrants */
	plasma_recursive(x1, y1, xmid, ymid, depth_max, rough);
	plasma_recursive(xmid, y1, x2, ymid, depth_max, rough);
	plasma_recursive(x1, ymid, xmid, y2, depth_max, rough);
	plasma_recursive(xmid, ymid, x2, y2, depth_max, rough);
}


#define MAX_FEAT_IN_TERRAIN 18

/*
 * The default table in terrain level generation.
 */
static s16b terrain_table[MAX_WILDERNESS][MAX_FEAT_IN_TERRAIN];

/*!
 * @brief 荒野フロア生成のサブルーチン
 * @param terrain 荒野地形ID
 * @param seed 乱数の固定シード
 * @param border 未使用
 * @param corner 広域マップの角部分としての生成ならばTRUE
 * @return なし
 */
static void generate_wilderness_area(int terrain, u32b seed, bool border, bool corner)
{
	POSITION x1, y1;
	int table_size = sizeof(terrain_table[0]) / sizeof(s16b);
	FEAT_IDX roughness = 1; /* The roughness of the level. */
	u32b state_backup[4];

	/* Unused */
	(void)border;

	/* The outer wall is easy */
	if (terrain == TERRAIN_EDGE)
	{
		/* Create level background */
		for (y1 = 0; y1 < MAX_HGT; y1++)
		{
			for (x1 = 0; x1 < MAX_WID; x1++)
			{
				current_floor_ptr->grid_array[y1][x1].feat = feat_permanent;
			}
		}

		/* We are done already */
		return;
	}


	/* Hack -- Backup the RNG state */
	Rand_state_backup(state_backup);

	/* Hack -- Induce consistant flavors */
	Rand_state_set(seed);

	if (!corner)
	{
		/* Create level background */
		for (y1 = 0; y1 < MAX_HGT; y1++)
		{
			for (x1 = 0; x1 < MAX_WID; x1++)
			{
				current_floor_ptr->grid_array[y1][x1].feat = table_size / 2;
			}
		}
	}

	/*
	 * Initialize the four corners
	 * ToDo: calculate the medium height of the adjacent
	 * terrains for every corner.
	 */
	current_floor_ptr->grid_array[1][1].feat = (s16b)randint0(table_size);
	current_floor_ptr->grid_array[MAX_HGT-2][1].feat = (s16b)randint0(table_size);
	current_floor_ptr->grid_array[1][MAX_WID-2].feat = (s16b)randint0(table_size);
	current_floor_ptr->grid_array[MAX_HGT-2][MAX_WID-2].feat = (s16b)randint0(table_size);

	if (!corner)
	{
		/* Hack -- preserve four corners */
		s16b north_west = current_floor_ptr->grid_array[1][1].feat;
		s16b south_west = current_floor_ptr->grid_array[MAX_HGT - 2][1].feat;
		s16b north_east = current_floor_ptr->grid_array[1][MAX_WID - 2].feat;
		s16b south_east = current_floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat;

		/* x1, y1, x2, y2, num_depths, roughness */
		plasma_recursive(1, 1, MAX_WID-2, MAX_HGT-2, table_size-1, roughness);

		/* Hack -- copyback four corners */
		current_floor_ptr->grid_array[1][1].feat = north_west;
		current_floor_ptr->grid_array[MAX_HGT - 2][1].feat = south_west;
		current_floor_ptr->grid_array[1][MAX_WID - 2].feat = north_east;
		current_floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat = south_east;

		for (y1 = 1; y1 < MAX_HGT - 1; y1++)
		{
			for (x1 = 1; x1 < MAX_WID - 1; x1++)
			{
				current_floor_ptr->grid_array[y1][x1].feat = terrain_table[terrain][current_floor_ptr->grid_array[y1][x1].feat];
			}
		}
	}
	else /* Hack -- only four corners */
	{
		current_floor_ptr->grid_array[1][1].feat = terrain_table[terrain][current_floor_ptr->grid_array[1][1].feat];
		current_floor_ptr->grid_array[MAX_HGT - 2][1].feat = terrain_table[terrain][current_floor_ptr->grid_array[MAX_HGT - 2][1].feat];
		current_floor_ptr->grid_array[1][MAX_WID - 2].feat = terrain_table[terrain][current_floor_ptr->grid_array[1][MAX_WID - 2].feat];
		current_floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat = terrain_table[terrain][current_floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat];
	}

	/* Hack -- Restore the RNG state */
	Rand_state_restore(state_backup);
}



/*!
 * @brief 荒野フロア生成のメインルーチン /
 * Load a town or generate a terrain level using "plasma" fractals.
 * @param y 広域Y座標
 * @param x 広域X座標
 * @param border 広域マップの辺部分としての生成ならばTRUE
 * @param corner 広域マップの角部分としての生成ならばTRUE
 * @return なし
 * @details
 * <pre>
 * x and y are the coordinates of the area in the wilderness.
 * Border and corner are optimization flags to speed up the
 * generation of the fractal terrain.
 * If border is set then only the border of the terrain should
 * be generated (for initializing the border structure).
 * If corner is set then only the corners of the area are needed.
 * </pre>
 */
static void generate_area(POSITION y, POSITION x, bool border, bool corner)
{
	POSITION x1, y1;

	/* Number of the town (if any) */
	p_ptr->town_num = wilderness[y][x].town;

	/* Set the base level */
	current_floor_ptr->base_level = wilderness[y][x].level;

	/* Set the dungeon level */
	current_floor_ptr->dun_level = 0;

	/* Set the monster generation level */
	current_floor_ptr->monster_level = current_floor_ptr->base_level;

	/* Set the object generation level */
	current_floor_ptr->object_level = current_floor_ptr->base_level;


	/* Create the town */
	if (p_ptr->town_num)
	{
		/* Reset the buildings */
		init_buildings();

		/* Initialize the town */
		if (border | corner)
			init_flags = INIT_CREATE_DUNGEON | INIT_ONLY_FEATURES;
		else
			init_flags = INIT_CREATE_DUNGEON;

		process_dungeon_file("t_info.txt", 0, 0, MAX_HGT, MAX_WID);

		if (!corner && !border) p_ptr->visit |= (1L << (p_ptr->town_num - 1));
	}
	else
	{
		int terrain = wilderness[y][x].terrain;
		u32b seed = wilderness[y][x].seed;

		generate_wilderness_area(terrain, seed, border, corner);
	}

	if (!corner && !wilderness[y][x].town)
	{
		/*
		 * Place roads in the wilderness
		 * ToDo: make the road a bit more interresting
		 */
		if (wilderness[y][x].road)
		{
			current_floor_ptr->grid_array[MAX_HGT/2][MAX_WID/2].feat = feat_floor;

			if (wilderness[y-1][x].road)
			{
				/* North road */
				for (y1 = 1; y1 < MAX_HGT/2; y1++)
				{
					x1 = MAX_WID/2;
					current_floor_ptr->grid_array[y1][x1].feat = feat_floor;
				}
			}

			if (wilderness[y+1][x].road)
			{
				/* North road */
				for (y1 = MAX_HGT/2; y1 < MAX_HGT - 1; y1++)
				{
					x1 = MAX_WID/2;
					current_floor_ptr->grid_array[y1][x1].feat = feat_floor;
				}
			}

			if (wilderness[y][x+1].road)
			{
				/* East road */
				for (x1 = MAX_WID/2; x1 < MAX_WID - 1; x1++)
				{
					y1 = MAX_HGT/2;
					current_floor_ptr->grid_array[y1][x1].feat = feat_floor;
				}
			}

			if (wilderness[y][x-1].road)
			{
				/* West road */
				for (x1 = 1; x1 < MAX_WID/2; x1++)
				{
					y1 = MAX_HGT/2;
					current_floor_ptr->grid_array[y1][x1].feat = feat_floor;
				}
			}
		}
	}

	if (wilderness[y][x].entrance && !wilderness[y][x].town && (p_ptr->total_winner || !(d_info[wilderness[y][x].entrance].flags1 & DF1_WINNER)))
	{
		int dy, dx;
		u32b state_backup[4];

		/* Hack -- Backup the RNG state */
		Rand_state_backup(state_backup);

		/* Hack -- Induce consistant flavors */
		Rand_state_set(wilderness[y][x].seed);

		dy = rand_range(6, current_floor_ptr->height - 6);
		dx = rand_range(6, current_floor_ptr->width - 6);

		current_floor_ptr->grid_array[dy][dx].feat = feat_entrance;
		current_floor_ptr->grid_array[dy][dx].special = wilderness[y][x].entrance;

		/* Hack -- Restore the RNG state */
		Rand_state_restore(state_backup);
	}
}


/*
 * Border of the wilderness area
 */
static border_type border;


/*!
 * @brief 広域マップの生成 /
 * Build the wilderness area outside of the town.
 * @return なし
 */
void wilderness_gen(void)
{
	int i, lim;
	POSITION y, x;
	grid_type *g_ptr;
	feature_type *f_ptr;

	/* Big town */
	current_floor_ptr->height = MAX_HGT;
	current_floor_ptr->width = MAX_WID;

	/* Assume illegal panel */
	panel_row_min = current_floor_ptr->height;
	panel_col_min = current_floor_ptr->width;

	/* Init the wilderness */

	process_dungeon_file("w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);

	x = p_ptr->wilderness_x;
	y = p_ptr->wilderness_y;
	get_mon_num_prep(get_monster_hook(), NULL);

	/* North border */
	generate_area(y - 1, x, TRUE, FALSE);

	for (i = 1; i < MAX_WID - 1; i++)
	{
		border.north[i] = current_floor_ptr->grid_array[MAX_HGT - 2][i].feat;
	}

	/* South border */
	generate_area(y + 1, x, TRUE, FALSE);

	for (i = 1; i < MAX_WID - 1; i++)
	{
		border.south[i] = current_floor_ptr->grid_array[1][i].feat;
	}

	/* West border */
	generate_area(y, x - 1, TRUE, FALSE);

	for (i = 1; i < MAX_HGT - 1; i++)
	{
		border.west[i] = current_floor_ptr->grid_array[i][MAX_WID - 2].feat;
	}

	/* East border */
	generate_area(y, x + 1, TRUE, FALSE);

	for (i = 1; i < MAX_HGT - 1; i++)
	{
		border.east[i] = current_floor_ptr->grid_array[i][1].feat;
	}

	/* North west corner */
	generate_area(y - 1, x - 1, FALSE, TRUE);
	border.north_west = current_floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat;

	/* North east corner */
	generate_area(y - 1, x + 1, FALSE, TRUE);
	border.north_east = current_floor_ptr->grid_array[MAX_HGT - 2][1].feat;

	/* South west corner */
	generate_area(y + 1, x - 1, FALSE, TRUE);
	border.south_west = current_floor_ptr->grid_array[1][MAX_WID - 2].feat;

	/* South east corner */
	generate_area(y + 1, x + 1, FALSE, TRUE);
	border.south_east = current_floor_ptr->grid_array[1][1].feat;


	/* Create terrain of the current area */
	generate_area(y, x, FALSE, FALSE);


	/* Special boundary walls -- North */
	for (i = 0; i < MAX_WID; i++)
	{
		current_floor_ptr->grid_array[0][i].feat = feat_permanent;
		current_floor_ptr->grid_array[0][i].mimic = border.north[i];
	}

	/* Special boundary walls -- South */
	for (i = 0; i < MAX_WID; i++)
	{
		current_floor_ptr->grid_array[MAX_HGT - 1][i].feat = feat_permanent;
		current_floor_ptr->grid_array[MAX_HGT - 1][i].mimic = border.south[i];
	}

	/* Special boundary walls -- West */
	for (i = 0; i < MAX_HGT; i++)
	{
		current_floor_ptr->grid_array[i][0].feat = feat_permanent;
		current_floor_ptr->grid_array[i][0].mimic = border.west[i];
	}

	/* Special boundary walls -- East */
	for (i = 0; i < MAX_HGT; i++)
	{
		current_floor_ptr->grid_array[i][MAX_WID - 1].feat = feat_permanent;
		current_floor_ptr->grid_array[i][MAX_WID - 1].mimic = border.east[i];
	}

	/* North west corner */
	current_floor_ptr->grid_array[0][0].mimic = border.north_west;

	/* North east corner */
	current_floor_ptr->grid_array[0][MAX_WID - 1].mimic = border.north_east;

	/* South west corner */
	current_floor_ptr->grid_array[MAX_HGT - 1][0].mimic = border.south_west;

	/* South east corner */
	current_floor_ptr->grid_array[MAX_HGT - 1][MAX_WID - 1].mimic = border.south_east;

	/* Light up or darken the area */
	for (y = 0; y < current_floor_ptr->height; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			g_ptr = &current_floor_ptr->grid_array[y][x];

			if (is_daytime())
			{
				/* Assume lit */
				g_ptr->info |= (CAVE_GLOW);

				/* Hack -- Memorize lit grids if allowed */
				if (view_perma_grids) g_ptr->info |= (CAVE_MARK);
			}
			else
			{
				/* Feature code (applying "mimic" field) */
				f_ptr = &f_info[get_feat_mimic(g_ptr)];

				if (!is_mirror_grid(g_ptr) && !have_flag(f_ptr->flags, FF_QUEST_ENTER) &&
				    !have_flag(f_ptr->flags, FF_ENTRANCE))
				{
					/* Assume dark */
					g_ptr->info &= ~(CAVE_GLOW);

					/* Darken "boring" features */
					if (!have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Forget the grid */
						g_ptr->info &= ~(CAVE_MARK);
					}
				}
				else if (have_flag(f_ptr->flags, FF_ENTRANCE))
				{
					/* Assume lit */
					g_ptr->info |= (CAVE_GLOW);

					/* Hack -- Memorize lit grids if allowed */
					if (view_perma_grids) g_ptr->info |= (CAVE_MARK);
				}
			}
		}
	}

	if (p_ptr->teleport_town)
	{
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				g_ptr = &current_floor_ptr->grid_array[y][x];

				/* Seeing true feature code (ignore mimic) */
				f_ptr = &f_info[g_ptr->feat];

				if (have_flag(f_ptr->flags, FF_BLDG))
				{
					if ((f_ptr->subtype == 4) || ((p_ptr->town_num == 1) && (f_ptr->subtype == 0)))
					{
						if (g_ptr->m_idx) delete_monster_idx(g_ptr->m_idx);
						p_ptr->oldpy = y;
						p_ptr->oldpx = x;
					}
				}
			}
		}
		p_ptr->teleport_town = FALSE;
	}

	else if (p_ptr->leaving_dungeon)
	{
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				g_ptr = &current_floor_ptr->grid_array[y][x];

				if (cave_have_flag_grid(g_ptr, FF_ENTRANCE))
				{
					if (g_ptr->m_idx) delete_monster_idx(g_ptr->m_idx);
					p_ptr->oldpy = y;
					p_ptr->oldpx = x;
				}
			}
		}
		p_ptr->teleport_town = FALSE;
	}

	player_place(p_ptr->oldpy, p_ptr->oldpx);
	/* p_ptr->leaving_dungeon = FALSE;*/

	lim = (generate_encounter == TRUE) ? 40 : MIN_M_ALLOC_TN;

	/* Make some residents */
	for (i = 0; i < lim; i++)
	{
		BIT_FLAGS mode = 0;

		if (!(generate_encounter || (one_in_(2) && (!p_ptr->town_num))))
			mode |= PM_ALLOW_SLEEP;

		/* Make a resident */
		(void)alloc_monster(generate_encounter ? 0 : 3, mode);
	}

	if(generate_encounter) p_ptr->ambush_flag = TRUE;
	generate_encounter = FALSE;

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(0);

	/* Set rewarded quests to finished */
	for (i = 0; i < max_q_idx; i++)
	{
		if (quest[i].status == QUEST_STATUS_REWARDED)
			quest[i].status = QUEST_STATUS_FINISHED;
	}
}


static s16b conv_terrain2feat[MAX_WILDERNESS];

/*!
 * @brief 広域マップの生成(簡易処理版) /
 * Build the wilderness area. -DG-
 * @return なし
 */
void wilderness_gen_small(void)
{
	int i, j;

	/* To prevent stupid things */
	for (i = 0; i < MAX_WID; i++)
	for (j = 0; j < MAX_HGT; j++)
	{
		current_floor_ptr->grid_array[j][i].feat = feat_permanent;
	}

	/* Init the wilderness */
	process_dungeon_file("w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);

	/* Fill the map */
	for (i = 0; i < current_world_ptr->max_wild_x; i++)
	for (j = 0; j < current_world_ptr->max_wild_y; j++)
	{
		if (wilderness[j][i].town && (wilderness[j][i].town != NO_TOWN))
		{
			current_floor_ptr->grid_array[j][i].feat = (s16b)feat_town;
			current_floor_ptr->grid_array[j][i].special = (s16b)wilderness[j][i].town;
		}
		else if (wilderness[j][i].road) current_floor_ptr->grid_array[j][i].feat = feat_floor;
		else if (wilderness[j][i].entrance && (p_ptr->total_winner || !(d_info[wilderness[j][i].entrance].flags1 & DF1_WINNER)))
		{
			current_floor_ptr->grid_array[j][i].feat = feat_entrance;
			current_floor_ptr->grid_array[j][i].special = (byte)wilderness[j][i].entrance;
		}
		else current_floor_ptr->grid_array[j][i].feat = conv_terrain2feat[wilderness[j][i].terrain];

		current_floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_MARK);
	}

	current_floor_ptr->height = (s16b) current_world_ptr->max_wild_y;
	current_floor_ptr->width = (s16b) current_world_ptr->max_wild_x;

	if (current_floor_ptr->height > MAX_HGT) current_floor_ptr->height = MAX_HGT;
	if (current_floor_ptr->width > MAX_WID) current_floor_ptr->width = MAX_WID;

	/* Assume illegal panel */
	panel_row_min = current_floor_ptr->height;
	panel_col_min = current_floor_ptr->width;

	/* Place the player */
	p_ptr->x = p_ptr->wilderness_x;
	p_ptr->y = p_ptr->wilderness_y;

	p_ptr->town_num = 0;
}


typedef struct wilderness_grid wilderness_grid;

struct wilderness_grid
{
	int		terrain;    /* Terrain type */
	TOWN_IDX town;   /* Town number */
	DEPTH level;     /* Level of the wilderness */
	byte	road;       /* Road */
	char	name[32];	/* Name of the town/wilderness */
};


static wilderness_grid w_letter[255];


/*!
 * @brief w_info.txtのデータ解析 /
 * Parse a sub-file of the "extra info"
 * @param buf 読み取ったデータ行のバッファ
 * @param ymin 未使用
 * @param xmin 広域地形マップを読み込みたいx座標の開始位置
 * @param ymax 未使用
 * @param xmax 広域地形マップを読み込みたいx座標の終了位置
 * @param y 広域マップの高さを返す参照ポインタ
 * @param x 広域マップの幅を返す参照ポインタ
 * @return なし
 */
errr parse_line_wilderness(char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
	int i, num;
	char *zz[33];

	/* Unused */
	(void)ymin;
	(void)ymax;
	if (!(buf[0] == 'W')) return (PARSE_ERROR_GENERIC);

	switch (buf[2])
	{
		/* Process "W:F:<letter>:<terrain>:<town>:<road>:<name> */
#ifdef JP
	case 'E':
		return 0;
	case 'F':
	case 'J':
#else
	case 'J':
		return 0;
	case 'F':
	case 'E':
#endif
	{
		if ((num = tokenize(buf+4, 6, zz, 0)) > 1)
		{
			int index = zz[0][0];
			
			if (num > 1)
				w_letter[index].terrain = atoi(zz[1]);
			else
				w_letter[index].terrain = 0;
			
			if (num > 2)
				w_letter[index].level = (s16b)atoi(zz[2]);
			else
				w_letter[index].level = 0;
			
			if (num > 3)
				w_letter[index].town = (TOWN_IDX)atoi(zz[3]);
			else
				w_letter[index].town = 0;
			
			if (num > 4)
				w_letter[index].road = (byte_hack)atoi(zz[4]);
			else
				w_letter[index].road = 0;
			
			if (num > 5)
				strcpy(w_letter[index].name, zz[5]);
			else
				w_letter[index].name[0] = 0;
		}
		else
		{
				/* Failure */
			return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
		}
		
		break;
	}
	
	/* Process "W:D:<layout> */
	/* Layout of the wilderness */
	case 'D':
	{
		/* Acquire the text */
		char *s = buf+4;
		
		/* Length of the text */
		int len = strlen(s);
		
		for (*x = xmin, i = 0; ((*x < xmax) && (i < len)); (*x)++, s++, i++)
		{
			int id = s[0];
			wilderness[*y][*x].terrain = w_letter[id].terrain;
			wilderness[*y][*x].level = w_letter[id].level;
			wilderness[*y][*x].town = w_letter[id].town;
			wilderness[*y][*x].road = w_letter[id].road;
			strcpy(town_info[w_letter[id].town].name, w_letter[id].name);
		}
		
		(*y)++;
		
		break;
	}
	
	/* Process "W:P:<x>:<y> - starting position in the wilderness */
	case 'P':
	{
		if ((p_ptr->wilderness_x == 0) &&
		    (p_ptr->wilderness_y == 0))
		{
			if (tokenize(buf+4, 2, zz, 0) == 2)
			{
				p_ptr->wilderness_y = atoi(zz[0]);
				p_ptr->wilderness_x = atoi(zz[1]);
				
				if ((p_ptr->wilderness_x < 1) ||
				    (p_ptr->wilderness_x > current_world_ptr->max_wild_x) ||
				    (p_ptr->wilderness_y < 1) ||
				    (p_ptr->wilderness_y > current_world_ptr->max_wild_y))
				{
					return (PARSE_ERROR_OUT_OF_BOUNDS);
				}
			}
			else
			{
				return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
			}
		}
		
		break;
	}
	
	default:
		/* Failure */
		return (PARSE_ERROR_UNDEFINED_DIRECTIVE);
	}
	
	for (i = 1; i < max_d_idx; i++)
	{
		if (!d_info[i].maxdepth) continue;
		wilderness[d_info[i].dy][d_info[i].dx].entrance = (byte_hack)i;
		if (!wilderness[d_info[i].dy][d_info[i].dx].town)
			wilderness[d_info[i].dy][d_info[i].dx].level = d_info[i].mindepth;
	}

	/* Success */
	return (0);
}



/*!
 * @brief ゲーム開始時に各荒野フロアの乱数シードを指定する /
 * Generate the random seeds for the wilderness
 * @return なし
 */
void seed_wilderness(void)
{
	POSITION x, y;

	/* Init wilderness seeds */
	for (x = 0; x < current_world_ptr->max_wild_x; x++)
	{
		for (y = 0; y < current_world_ptr->max_wild_y; y++)
		{
			wilderness[y][x].seed = randint0(0x10000000);
			wilderness[y][x].entrance = 0;
		}
	}
}


/*
 * Pointer to wilderness_type
 */
typedef wilderness_type *wilderness_type_ptr;


/*!
 * @brief ゲーム開始時の荒野初期化メインルーチン /
 * Initialize wilderness array
 * @return エラーコード
 */
errr init_wilderness(void)
{
	int i;

	/* Allocate the wilderness (two-dimension array) */
	C_MAKE(wilderness, current_world_ptr->max_wild_y, wilderness_type_ptr);
	C_MAKE(wilderness[0], current_world_ptr->max_wild_x * current_world_ptr->max_wild_y, wilderness_type);

	/* Init the other pointers */
	for (i = 1; i < current_world_ptr->max_wild_y; i++)
		wilderness[i] = wilderness[0] + i * current_world_ptr->max_wild_x;

	generate_encounter = FALSE;

	return 0;
}

/*!
 * @brief 荒野の地勢設定を初期化する /
 * Initialize wilderness array
 * @param terrain 初期化したい地勢ID
 * @param feat_global 基本的な地形ID
 * @param fmt 地勢内の地形数を参照するための独自フォーマット
 * @return なし
 */
static void init_terrain_table(int terrain, s16b feat_global, concptr fmt, ...)
{
	va_list vp;
	concptr    p;
	int     cur = 0;
	char    check = 'a';
	FEAT_IDX feat;
	int     num;

	/* Begin the varargs stuff */
	va_start(vp, fmt);

	/* Wilderness terrains on global map */
	conv_terrain2feat[terrain] = feat_global;

	/* Wilderness terrains on local map */
	for (p = fmt; *p; p++)
	{
		if (*p == check)
		{
			int lim;

			feat = (s16b)va_arg(vp, int);
			num = va_arg(vp, int);
			lim = cur + num;

			for (; (cur < lim) && (cur < MAX_FEAT_IN_TERRAIN); cur++)
				terrain_table[terrain][cur] = feat;
			if (cur >= MAX_FEAT_IN_TERRAIN) break;

			check++;
		}
		else
		{
			plog_fmt("Format error");
		}
	}
	if (cur < MAX_FEAT_IN_TERRAIN)
	{
		plog_fmt("Too few parameters");
	}

	/* End the varargs stuff */
	va_end(vp);
}


/*!
 * @brief 荒野の地勢設定全体を初期化するメインルーチン /
 * Initialize arrays for wilderness terrains
 * @return なし
 */
void init_wilderness_terrains(void)
{
	init_terrain_table(TERRAIN_EDGE, feat_permanent, "a",
		feat_permanent, MAX_FEAT_IN_TERRAIN);

	init_terrain_table(TERRAIN_TOWN, feat_town, "a",
		feat_floor, MAX_FEAT_IN_TERRAIN);

	init_terrain_table(TERRAIN_DEEP_WATER, feat_deep_water, "ab",
		feat_deep_water, 12,
		feat_shallow_water, MAX_FEAT_IN_TERRAIN - 12);

	init_terrain_table(TERRAIN_SHALLOW_WATER, feat_shallow_water, "abcde",
		feat_deep_water, 3,
		feat_shallow_water, 12,
		feat_floor, 1,
		feat_dirt, 1,
		feat_grass, MAX_FEAT_IN_TERRAIN - 17);

	init_terrain_table(TERRAIN_SWAMP, feat_swamp, "abcdef",
		feat_dirt, 2,
		feat_grass, 3,
		feat_tree, 1,
		feat_brake, 1,
		feat_shallow_water, 4,
		feat_swamp, MAX_FEAT_IN_TERRAIN - 11);

	init_terrain_table(TERRAIN_DIRT, feat_dirt, "abcdef",
		feat_floor, 3,
		feat_dirt, 10,
		feat_flower, 1,
		feat_brake, 1,
		feat_grass, 1,
		feat_tree, MAX_FEAT_IN_TERRAIN - 16);

	init_terrain_table(TERRAIN_GRASS, feat_grass, "abcdef",
		feat_floor, 2,
		feat_dirt, 2,
		feat_grass, 9,
		feat_flower, 1,
		feat_brake, 2,
		feat_tree, MAX_FEAT_IN_TERRAIN - 16);

	init_terrain_table(TERRAIN_TREES, feat_tree, "abcde",
		feat_floor, 2,
		feat_dirt, 1,
		feat_tree, 11,
		feat_brake, 2,
		feat_grass, MAX_FEAT_IN_TERRAIN - 16);

	init_terrain_table(TERRAIN_DESERT, feat_dirt, "abc",
		feat_floor, 2,
		feat_dirt, 13,
		feat_grass, MAX_FEAT_IN_TERRAIN - 15);

	init_terrain_table(TERRAIN_SHALLOW_LAVA, feat_shallow_lava, "abc",
		feat_shallow_lava, 14,
		feat_deep_lava, 3,
		feat_mountain, MAX_FEAT_IN_TERRAIN - 17);

	init_terrain_table(TERRAIN_DEEP_LAVA, feat_deep_lava, "abcd",
		feat_dirt, 3,
		feat_shallow_lava, 3,
		feat_deep_lava, 10,
		feat_mountain, MAX_FEAT_IN_TERRAIN - 16);

	init_terrain_table(TERRAIN_MOUNTAIN, feat_mountain, "abcdef",
		feat_floor, 1,
		feat_brake, 1,
		feat_grass, 2,
		feat_dirt, 2,
		feat_tree, 2,
		feat_mountain, MAX_FEAT_IN_TERRAIN - 8);
}

/*!
 * @brief 荒野から広域マップへの切り替え処理 /
 * Initialize arrays for wilderness terrains
 * @return 切り替えが行われた場合はTRUEを返す。
 */
bool change_wild_mode(void)
{
	int i;
	bool have_pet = FALSE;

	/* It is in the middle of changing map */
	if (p_ptr->leaving) return FALSE;


	if (lite_town || vanilla_town)
	{
		msg_print(_("荒野なんてない。", "No global map."));
		return FALSE;
	}

	if (p_ptr->wild_mode)
	{
		/* Save the location in the global map */
		p_ptr->wilderness_x = p_ptr->x;
		p_ptr->wilderness_y = p_ptr->y;

		/* Give first move to the player */
		p_ptr->energy_need = 0;

		/* Go back to the ordinary map */
		p_ptr->wild_mode = FALSE;
		p_ptr->leaving = TRUE;

		/* Succeed */
		return TRUE;
	}

	for (i = 1; i < current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;
		if (is_pet(m_ptr) && i != p_ptr->riding) have_pet = TRUE;
		if (MON_CSLEEP(m_ptr)) continue;
		if (m_ptr->cdis > MAX_SIGHT) continue;
		if (!is_hostile(m_ptr)) continue;
		msg_print(_("敵がすぐ近くにいるときは広域マップに入れない！",
			"You cannot enter global map, since there is some monsters nearby!"));
		free_turn(p_ptr);
		return FALSE;
	}

	if (have_pet)
	{
		concptr msg = _("ペットを置いて広域マップに入りますか？",
			"Do you leave your pets behind? ");

		if (!get_check_strict(msg, CHECK_OKAY_CANCEL))
		{
			free_turn(p_ptr);
			return FALSE;
		}
	}

	take_turn(p_ptr, 1000);

	/* Remember the position */
	p_ptr->oldpx = p_ptr->x;
	p_ptr->oldpy = p_ptr->y;

	/* Cancel hex spelling */
	if (hex_spelling_any()) stop_hex_spell_all();

	/* Cancel any special action */
	set_action(ACTION_NONE);

	/* Go into the global map */
	p_ptr->wild_mode = TRUE;
	p_ptr->leaving = TRUE;

	/* Succeed */
	return TRUE;
}
