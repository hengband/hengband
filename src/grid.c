
 /*!
  * @file grid.c
  * @brief グリッドの実装 / low level dungeon routines -BEN-
  * @date 2013/12/30
  * @author
  * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
  *\n
  * This software may be copied and distributed for educational, research,\n
  * and not for profit purposes provided that this copyright and statement\n
  * are included in all such copies.  Other copyrights may also apply.\n
  * \n
  * Support for Adam Bolt's tileset, lighting and transparency effects\n
  * by Robert Ruehlmann (rr9@angband.org)\n
  * \n
  * 2013 Deskull Doxygen向けのコメント整理\n
  */


#include "angband.h"
#include "util.h"
#include "term.h"

#include "floor.h"
#include "world.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "dungeon.h"
#include "floor-generate.h"
#include "grid.h"
#include "trap.h"
#include "rooms.h"
#include "monster.h"
#include "quest.h"
#include "feature.h"
#include "monster-status.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-class.h"
#include "spells.h"
#include "view-mainwindow.h"
#include "realm-song.h"

#define MONSTER_FLOW_DEPTH 32 /*!< 敵のプレイヤーに対する移動道のりの最大値(この値以上は処理を打ち切る) / OPTION: Maximum flow depth when using "MONSTER_FLOW" */

/*
 * Feature action flags
 */
#define FAF_DESTROY     0x01
#define FAF_NO_DROP     0x02
#define FAF_CRASH_GLASS 0x04

/*!
 * @brief 地形状態フラグテーブル /
 * The table of features' actions
 */
static const byte feature_action_flags[FF_FLAG_MAX] =
{
	0, /* LOS */
	0, /* PROJECT */
	0, /* MOVE */
	0, /* PLACE */
	0, /* DROP */
	0, /* SECRET */
	0, /* NOTICE */
	0, /* REMEMBER */
	0, /* OPEN */
	0, /* CLOSE */
	FAF_CRASH_GLASS, /* BASH */
	0, /* SPIKE */
	FAF_DESTROY, /* DISARM */
	0, /* STORE */
	FAF_DESTROY | FAF_CRASH_GLASS, /* TUNNEL */
	0, /* MAY_HAVE_GOLD */
	0, /* HAS_GOLD */
	0, /* HAS_ITEM */
	0, /* DOOR */
	0, /* TRAP */
	0, /* STAIRS */
	0, /* GLYPH */
	0, /* LESS */
	0, /* MORE */
	0, /* RUN */
	0, /* FLOOR */
	0, /* WALL */
	0, /* PERMANENT */
	0, /* INNER */
	0, /* OUTER */
	0, /* SOLID */
	0, /* HIT_TRAP */

	0, /* BRIDGE */
	0, /* RIVER */
	0, /* LAKE */
	0, /* BRIDGED */
	0, /* COVERED */
	0, /* GLOW */
	0, /* ENSECRET */
	0, /* WATER */
	0, /* LAVA */
	0, /* SHALLOW */
	0, /* DEEP */
	0, /* FILLED */
	FAF_DESTROY | FAF_CRASH_GLASS, /* HURT_ROCK */
	0, /* HURT_FIRE */
	0, /* HURT_COLD */
	0, /* HURT_ACID */
	0, /* ICE */
	0, /* ACID */
	0, /* OIL */
	0, /* XXX04 */
	0, /* CAN_CLIMB */
	0, /* CAN_FLY */
	0, /* CAN_SWIM */
	0, /* CAN_PASS */
	0, /* CAN_OOZE */
	0, /* CAN_DIG */
	0, /* HIDE_ITEM */
	0, /* HIDE_SNEAK */
	0, /* HIDE_SWIM */
	0, /* HIDE_DIG */
	0, /* KILL_HUGE */
	0, /* KILL_MOVE */

	0, /* PICK_TRAP */
	0, /* PICK_DOOR */
	0, /* ALLOC */
	0, /* CHEST */
	0, /* DROP_1D2 */
	0, /* DROP_2D2 */
	0, /* DROP_GOOD */
	0, /* DROP_GREAT */
	0, /* HURT_POIS */
	0, /* HURT_ELEC */
	0, /* HURT_WATER */
	0, /* HURT_BWATER */
	0, /* USE_FEAT */
	0, /* GET_FEAT */
	0, /* GROUND */
	0, /* OUTSIDE */
	0, /* EASY_HIDE */
	0, /* EASY_CLIMB */
	0, /* MUST_CLIMB */
	0, /* TREE */
	0, /* NEED_TREE */
	0, /* BLOOD */
	0, /* DUST */
	0, /* SLIME */
	0, /* PLANT */
	0, /* XXX2 */
	0, /* INSTANT */
	0, /* EXPLODE */
	0, /* TIMED */
	0, /* ERUPT */
	0, /* STRIKE */
	0, /* SPREAD */

	0, /* SPECIAL */
	FAF_DESTROY | FAF_NO_DROP | FAF_CRASH_GLASS, /* HURT_DISI */
	0, /* QUEST_ENTER */
	0, /* QUEST_EXIT */
	0, /* QUEST */
	0, /* SHAFT */
	0, /* MOUNTAIN */
	0, /* BLDG */
	0, /* MINOR_GLYPH */
	0, /* PATTERN */
	0, /* TOWN */
	0, /* ENTRANCE */
	0, /* MIRROR */
	0, /* UNPERM */
	0, /* TELEPORTABLE */
	0, /* CONVERT */
	0, /* GLASS */
};

/*!
 * @brief 新規フロアに入りたてのプレイヤーをランダムな場所に配置する / Returns random co-ordinates for player/monster/object
 * @param creature_ptr 配置したいクリーチャーの参照ポインタ
 * @return 配置に成功したらTRUEを返す
 */
bool new_player_spot(player_type *creature_ptr)
{
	POSITION y = 0, x = 0;
	int max_attempts = 10000;

	grid_type *g_ptr;
	feature_type *f_ptr;

	while (max_attempts--)
	{
		/* Pick a legal spot */
		y = (POSITION)rand_range(1, creature_ptr->current_floor_ptr->height - 2);
		x = (POSITION)rand_range(1, creature_ptr->current_floor_ptr->width - 2);

		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Must be a "naked" floor grid */
		if (g_ptr->m_idx) continue;
		if (creature_ptr->current_floor_ptr->dun_level)
		{
			f_ptr = &f_info[g_ptr->feat];

			if (max_attempts > 5000) /* Rule 1 */
			{
				if (!have_flag(f_ptr->flags, FF_FLOOR)) continue;
			}
			else /* Rule 2 */
			{
				if (!have_flag(f_ptr->flags, FF_MOVE)) continue;
				if (have_flag(f_ptr->flags, FF_HIT_TRAP)) continue;
			}

			/* Refuse to start on anti-teleport grids in dungeon */
			if (!have_flag(f_ptr->flags, FF_TELEPORTABLE)) continue;
		}
		if (!player_can_enter(g_ptr->feat, 0)) continue;
		if (!in_bounds(creature_ptr->current_floor_ptr, y, x)) continue;

		/* Refuse to start on anti-teleport grids */
		if (g_ptr->info & (CAVE_ICKY)) continue;

		break;
	}

	if (max_attempts < 1) /* Should be -1, actually if we failed... */
		return FALSE;

	/* Save the new player grid */
	creature_ptr->y = y;
	creature_ptr->x = x;

	return TRUE;
}

/*!
* @brief 隣接4マスに存在する通路の数を返す / Count the number of "corridor" grids adjacent to the given grid.
* @param y1 基準となるマスのY座標
* @param x1 基準となるマスのX座標
* @return 通路の数
* @note Assumes "in_bounds(p_ptr->current_floor_ptr, y1, x1)"
* @details
* XXX XXX This routine currently only counts actual "empty floor"\n
* grids which are not in rooms.  We might want to also count stairs,\n
* open doors, closed doors, etc.
*/
static int next_to_corr(POSITION y1, POSITION x1)
{
	int i, k = 0;
	POSITION y, x;

	grid_type *g_ptr;

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];
		g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

		/* Skip non floors */
		if (cave_have_flag_grid(g_ptr, FF_WALL)) continue;

		/* Skip non "empty floor" grids */
		if (!is_floor_grid(g_ptr))
			continue;

		/* Skip grids inside rooms */
		if (g_ptr->info & (CAVE_ROOM)) continue;

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
* @note Assumes "in_bounds(p_ptr->current_floor_ptr, y1, x1)"
* @details
* \n
* Assumes "in_bounds(p_ptr->current_floor_ptr, y, x)"\n
*/
static bool possible_doorway(POSITION y, POSITION x)
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
void try_door(POSITION y, POSITION x)
{	if (!in_bounds(p_ptr->current_floor_ptr, y, x)) return;

	/* Ignore walls */
	if (cave_have_flag_bold(y, x, FF_WALL)) return;

	/* Ignore room grids */
	if (p_ptr->current_floor_ptr->grid_array[y][x].info & (CAVE_ROOM)) return;

	/* Occasional door (if allowed) */
	if ((randint0(100) < dun_tun_jct) && possible_doorway(y, x) && !(d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_DOORS))
	{
		/* Place a door */
		place_random_door(p_ptr->current_floor_ptr, y, x, FALSE);
	}
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する / Create up to "num" objects near the given coordinates
 * @param y 配置したい中心マスのY座標
 * @param x 配置したい中心マスのX座標
 * @param num 配置したい数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_objects(POSITION y, POSITION x, int num)
{
	int dummy = 0;
	int i = 0, j = y, k = x;

	grid_type *g_ptr;


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
				if (!in_bounds(p_ptr->current_floor_ptr, j, k)) continue;
				break;
			}

			if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
			{
				msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
			}

			/* Require "clean" floor space */
			g_ptr = &p_ptr->current_floor_ptr->grid_array[j][k];
			if (!is_floor_grid(g_ptr) || g_ptr->o_idx) continue;

			if (randint0(100) < 75)
			{
				place_object(j, k, 0L);
			}
			else
			{
				place_gold(j, k);
			}

			/* Placement accomplished */
			break;
		}
	}
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(vault_trapのサブセット) / Place a trap with a given displacement of point
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_trap_aux(POSITION y, POSITION x, POSITION yd, POSITION xd)
{
	int count = 0, y1 = y, x1 = x;
	int dummy = 0;

	grid_type *g_ptr;

	/* Place traps */
	for (count = 0; count <= 5; count++)
	{
		/* Get a location */
		while (dummy < SAFE_MAX_ATTEMPTS)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			dummy++;
			if (!in_bounds(p_ptr->current_floor_ptr, y1, x1)) continue;
			break;
		}

		if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room)
		{
			msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
		}

		/* Require "naked" floor grids */
		g_ptr = &p_ptr->current_floor_ptr->grid_array[y1][x1];
		if (!is_floor_grid(g_ptr) || g_ptr->o_idx || g_ptr->m_idx) continue;

		/* Place the trap */
		place_trap(p_ptr->current_floor_ptr, y1, x1);

		break;
	}
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(メインルーチン) / Place some traps with a given displacement of given location
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @param num 配置したいトラップの数
 * @return なし
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_traps(POSITION y, POSITION x, POSITION yd, POSITION xd, int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		vault_trap_aux(y, x, yd, xd);
	}
}

/*!
 * @brief 指定のマスが床系地形であるかを返す / Function that sees if a square is a floor.  (Includes range checking.)
 * @param x チェックするマスのX座標
 * @param y チェックするマスのY座標
 * @return 床系地形ならばTRUE
 */
bool get_is_floor(POSITION x, POSITION y)
{
	if (!in_bounds(p_ptr->current_floor_ptr, y, x))
	{
		/* Out of bounds */
		return (FALSE);
	}

	/* Do the real check */
	if (is_floor_bold(p_ptr->current_floor_ptr, y, x)) return (TRUE);

	return (FALSE);
}

/*!
 * @brief 指定のマスを床地形に変える / Set a square to be floor.  (Includes range checking.)
 * @param x 地形を変えたいマスのX座標
 * @param y 地形を変えたいマスのY座標
 * @return なし
 */
void set_floor(POSITION x, POSITION y)
{
	if (!in_bounds(p_ptr->current_floor_ptr, y, x))
	{
		/* Out of bounds */
		return;
	}

	if (p_ptr->current_floor_ptr->grid_array[y][x].info & CAVE_ROOM)
	{
		/* A room border don't touch. */
		return;
	}

	/* Set to be floor if is a wall (don't touch lakes). */
	if (is_extra_bold(p_ptr->current_floor_ptr, y, x))
		place_floor_bold(p_ptr->current_floor_ptr, y, x);
}

/*!
 * @brief マスにフロア端用の永久壁を配置する / Set boundary mimic and add "solid" perma-wall
 * @param g_ptr 永久壁を配置したいマス構造体の参照ポインタ
 * @return なし
 */
void place_bound_perm_wall(grid_type *g_ptr)
{
	if (bound_walls_perm)
	{
		/* Clear boundary mimic */
		g_ptr->mimic = 0;
	}
	else
	{
		feature_type *f_ptr = &f_info[g_ptr->feat];

		/* Hack -- Decline boundary walls with known treasure  */
		if ((have_flag(f_ptr->flags, FF_HAS_GOLD) || have_flag(f_ptr->flags, FF_HAS_ITEM)) &&
			!have_flag(f_ptr->flags, FF_SECRET))
			g_ptr->feat = feat_state(g_ptr->feat, FF_ENSECRET);

		/* Set boundary mimic */
		g_ptr->mimic = g_ptr->feat;
	}

	/* Add "solid" perma-wall */
	place_solid_perm_grid(g_ptr);
}

/*!
 * @brief マスに看破済みの罠があるかの判定を行う。 / Return TRUE if the given grid is a known trap
 * @param g_ptr マス構造体の参照ポインタ
 * @return 看破済みの罠があるならTRUEを返す。
 */
bool is_known_trap(grid_type *g_ptr)
{
	if (!g_ptr->mimic && !cave_have_flag_grid(g_ptr, FF_SECRET) &&
		is_trap(g_ptr->feat)) return TRUE;
	else
		return FALSE;
}



/*!
 * @brief マスに隠されたドアがあるかの判定を行う。 / Return TRUE if the given grid is a hidden closed door
 * @param g_ptr マス構造体の参照ポインタ
 * @return 隠されたドアがあるならTRUEを返す。
 */
bool is_hidden_door(grid_type *g_ptr)
{
	if ((g_ptr->mimic || cave_have_flag_grid(g_ptr, FF_SECRET)) &&
		is_closed_door(g_ptr->feat))
		return TRUE;
	else
		return FALSE;
}

#define COMPLEX_WALL_ILLUMINATION /*!< 照明状態を壁により影響を受ける、より複雑な判定に切り替えるマクロ */


/*!
 * @brief 指定された座標のマスが現在照らされているかを返す。 / Check for "local" illumination
 * @param y y座標
 * @param x x座標
 * @return 指定された座標に照明がかかっているならTRUEを返す。。
 */
bool check_local_illumination(POSITION y, POSITION x)
{
	/* Hack -- move towards player */
	POSITION yy = (y < p_ptr->y) ? (y + 1) : (y > p_ptr->y) ? (y - 1) : y;
	POSITION xx = (x < p_ptr->x) ? (x + 1) : (x > p_ptr->x) ? (x - 1) : x;

	/* Check for "local" illumination */

#ifdef COMPLEX_WALL_ILLUMINATION /* COMPLEX_WALL_ILLUMINATION */

	/* Check for "complex" illumination */
	if ((feat_supports_los(get_feat_mimic(&p_ptr->current_floor_ptr->grid_array[yy][xx])) &&
		(p_ptr->current_floor_ptr->grid_array[yy][xx].info & CAVE_GLOW)) ||
		(feat_supports_los(get_feat_mimic(&p_ptr->current_floor_ptr->grid_array[y][xx])) &&
		(p_ptr->current_floor_ptr->grid_array[y][xx].info & CAVE_GLOW)) ||
			(feat_supports_los(get_feat_mimic(&p_ptr->current_floor_ptr->grid_array[yy][x])) &&
		(p_ptr->current_floor_ptr->grid_array[yy][x].info & CAVE_GLOW)))
	{
		return TRUE;
	}
	else return FALSE;

#else /* COMPLEX_WALL_ILLUMINATION */

	/* Check for "simple" illumination */
	return (p_ptr->current_floor_ptr->grid_array[yy][xx].info & CAVE_GLOW) ? TRUE : FALSE;

#endif /* COMPLEX_WALL_ILLUMINATION */
}


/*! 対象座標のマスの照明状態を更新する際の補助処理マクロ */
#define update_local_illumination_aux(C, Y, X) \
{ \
	if (player_has_los_bold((C), (Y), (X))) \
	{ \
		/* Update the monster */ \
		if ((C)->current_floor_ptr->grid_array[(Y)][(X)].m_idx) update_monster((C), (C)->current_floor_ptr->grid_array[(Y)][(X)].m_idx, FALSE); \
\
		/* Notice and redraw */ \
		note_spot((Y), (X)); \
		lite_spot((Y), (X)); \
	} \
}

/*!
 * @brief 指定された座標の照明状態を更新する / Update "local" illumination
 * @param creature_ptr 視界元のクリーチャー
 * @param y 視界先y座標
 * @param x 視界先x座標
 * @return なし
 */
void update_local_illumination(player_type * creature_ptr, POSITION y, POSITION x)
{
	int i;
	POSITION yy, xx;

	if (!in_bounds(p_ptr->current_floor_ptr, y, x)) return;

#ifdef COMPLEX_WALL_ILLUMINATION /* COMPLEX_WALL_ILLUMINATION */

	if ((y != creature_ptr->y) && (x != creature_ptr->x))
	{
		yy = (y < creature_ptr->y) ? (y - 1) : (y + 1);
		xx = (x < creature_ptr->x) ? (x - 1) : (x + 1);
		update_local_illumination_aux(creature_ptr, yy, xx);
		update_local_illumination_aux(creature_ptr, y, xx);
		update_local_illumination_aux(creature_ptr, yy, x);
	}
	else if (x != creature_ptr->x) /* y == creature_ptr->y */
	{
		xx = (x < creature_ptr->x) ? (x - 1) : (x + 1);
		for (i = -1; i <= 1; i++)
		{
			yy = y + i;
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
		yy = y - 1;
		update_local_illumination_aux(creature_ptr, yy, x);
		yy = y + 1;
		update_local_illumination_aux(creature_ptr, yy, x);
	}
	else if (y != creature_ptr->y) /* x == creature_ptr->x */
	{
		yy = (y < creature_ptr->y) ? (y - 1) : (y + 1);
		for (i = -1; i <= 1; i++)
		{
			xx = x + i;
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
		xx = x - 1;
		update_local_illumination_aux(creature_ptr, y, xx);
		xx = x + 1;
		update_local_illumination_aux(creature_ptr, y, xx);
	}
	else /* Player's grid */
	{
		for (i = 0; i < 8; i++)
		{
			yy = y + ddy_cdd[i];
			xx = x + ddx_cdd[i];
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
	}

#else /* COMPLEX_WALL_ILLUMINATION */

	if ((y != creature_ptr->y) && (x != creature_ptr->x))
	{
		yy = (y < creature_ptr->y) ? (y - 1) : (y + 1);
		xx = (x < creature_ptr->x) ? (x - 1) : (x + 1);
		update_local_illumination_aux(creature_ptr, yy, xx);
	}
	else if (x != creature_ptr->x) /* y == creature_ptr->y */
	{
		xx = (x < creature_ptr->x) ? (x - 1) : (x + 1);
		for (i = -1; i <= 1; i++)
		{
			yy = y + i;
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
	}
	else if (y != creature_ptr->y) /* x == creature_ptr->x */
	{
		yy = (y < creature_ptr->y) ? (y - 1) : (y + 1);
		for (i = -1; i <= 1; i++)
		{
			xx = x + i;
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
	}
	else /* Player's grid */
	{
		for (i = 0; i < 8; i++)
		{
			yy = y + ddy_cdd[i];
			xx = x + ddx_cdd[i];
			update_local_illumination_aux(creature_ptr, yy, xx);
		}
	}

#endif /* COMPLEX_WALL_ILLUMINATION */
}


/*!
 * @brief 指定された座標をプレイヤー収められていない状態かどうか / Returns true if the player's grid is dark
 * @return 視覚に収められていないならTRUEを返す
 * @details player_can_see_bold()関数の返り値の否定を返している。
 */
bool no_lite(void)
{
	return (!player_can_see_bold(p_ptr->y, p_ptr->x));
}

/*
 * Place an attr/char pair at the given map coordinate, if legal.
 */
void print_rel(SYMBOL_CODE c, TERM_COLOR a, TERM_LEN y, TERM_LEN x)
{
	/* Only do "legal" locations */
	if (panel_contains(y, x))
	{
		/* Hack -- fake monochrome */
		if (!use_graphics)
		{
			if (current_world_ptr->timewalk_m_idx) a = TERM_DARK;
			else if (IS_INVULN() || p_ptr->timewalk) a = TERM_WHITE;
			else if (p_ptr->wraith_form) a = TERM_L_DARK;
		}

		/* Draw the char using the attr */
		Term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, 0, 0);
	}
}


/*
 * Memorize interesting viewable object/features in the given grid
 *
 * This function should only be called on "legal" grids.
 *
 * This function will memorize the object and/or feature in the given
 * grid, if they are (1) viewable and (2) interesting.  Note that all
 * objects are interesting, all terrain features except floors (and
 * invisible traps) are interesting, and floors (and invisible traps)
 * are interesting sometimes (depending on various options involving
 * the illumination of floor grids).
 *
 * The automatic memorization of all objects and non-floor terrain
 * features as soon as they are displayed allows incredible amounts
 * of optimization in various places, especially "map_info()".
 *
 * Note that the memorization of objects is completely separate from
 * the memorization of terrain features, preventing annoying floor
 * memorization when a detected object is picked up from a dark floor,
 * and object memorization when an object is dropped into a floor grid
 * which is memorized but out-of-sight.
 *
 * This function should be called every time the "memorization" of
 * a grid (or the object in a grid) is called into question, such
 * as when an object is created in a grid, when a terrain feature
 * "changes" from "floor" to "non-floor", when any grid becomes
 * "illuminated" or "viewable", and when a "floor" grid becomes
 * "torch-lit".
 *
 * Note the relatively efficient use of this function by the various
 * "update_view()" and "update_lite()" calls, to allow objects and
 * terrain features to be memorized (and drawn) whenever they become
 * viewable or illuminated in any way, but not when they "maintain"
 * or "lose" their previous viewability or illumination.
 *
 * Note the butchered "internal" version of "player_can_see_bold()",
 * optimized primarily for the most common cases, that is, for the
 * non-marked floor grids.
 */
void note_spot(POSITION y, POSITION x)
{
	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	/* Blind players see nothing */
	if (p_ptr->blind) return;

	/* Analyze non-torch-lit grids */
	if (!(g_ptr->info & (CAVE_LITE | CAVE_MNLT)))
	{
		/* Require line of sight to the grid */
		if (!(g_ptr->info & (CAVE_VIEW))) return;

		/* Require "perma-lite" of the grid */
		if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW)
		{
			/* Not Ninja */
			if (!p_ptr->see_nocto) return;
		}
	}


	/* Hack -- memorize objects */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr = &p_ptr->current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Memorize objects */
		o_ptr->marked |= OM_FOUND;
	}


	/* Hack -- memorize grids */
	if (!(g_ptr->info & (CAVE_MARK)))
	{
		/* Feature code (applying "mimic" field) */
		feature_type *f_ptr = &f_info[get_feat_mimic(g_ptr)];

		/* Memorize some "boring" grids */
		if (!have_flag(f_ptr->flags, FF_REMEMBER))
		{
			/* Option -- memorize all torch-lit floors */
			if (view_torch_grids &&
				((g_ptr->info & (CAVE_LITE | CAVE_MNLT)) || p_ptr->see_nocto))
			{
				g_ptr->info |= (CAVE_MARK);
			}

			/* Option -- memorize all perma-lit floors */
			else if (view_perma_grids && ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW))
			{
				g_ptr->info |= (CAVE_MARK);
			}
		}

		/* Memorize normal grids */
		else if (have_flag(f_ptr->flags, FF_LOS))
		{
			g_ptr->info |= (CAVE_MARK);
		}

		/* Memorize torch-lit walls */
		else if (g_ptr->info & (CAVE_LITE | CAVE_MNLT))
		{
			g_ptr->info |= (CAVE_MARK);
		}

		/* Memorize walls seen by noctovision of Ninja */
		else if (p_ptr->see_nocto)
		{
			g_ptr->info |= (CAVE_MARK);
		}

		/* Memorize certain non-torch-lit wall grids */
		else if (check_local_illumination(y, x))
		{
			g_ptr->info |= (CAVE_MARK);
		}
	}

	/* Memorize terrain of the grid */
	g_ptr->info |= (CAVE_KNOWN);
}

/*
 * Redraw (on the screen) a given MAP location
 *
 * This function should only be called on "legal" grids
 */
void lite_spot(POSITION y, POSITION x)
{
	/* Redraw if on screen */
	if (panel_contains(y, x) && in_bounds2(p_ptr->current_floor_ptr, y, x))
	{
		TERM_COLOR a;
		SYMBOL_CODE c;
		TERM_COLOR ta;
		SYMBOL_CODE tc;

		map_info(y, x, &a, &c, &ta, &tc);

		/* Hack -- fake monochrome */
		if (!use_graphics)
		{
			if (current_world_ptr->timewalk_m_idx) a = TERM_DARK;
			else if (IS_INVULN() || p_ptr->timewalk) a = TERM_WHITE;
			else if (p_ptr->wraith_form) a = TERM_L_DARK;
		}

		/* Hack -- Queue it */
		Term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);

		/* Update sub-windows */
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}
}

/*
 * Some comments on the grid flags.  -BEN-
 *
 *
 * One of the major bottlenecks in previous versions of Angband was in
 * the calculation of "line of sight" from the player to various grids,
 * such as monsters.  This was such a nasty bottleneck that a lot of
 * silly things were done to reduce the dependancy on "line of sight",
 * for example, you could not "see" any grids in a lit room until you
 * actually entered the room, and there were all kinds of bizarre grid
 * flags to enable this behavior.  This is also why the "call light"
 * spells always lit an entire room.
 *
 * The code below provides functions to calculate the "field of view"
 * for the player, which, once calculated, provides extremely fast
 * calculation of "line of sight from the player", and to calculate
 * the "field of torch lite", which, again, once calculated, provides
 * extremely fast calculation of "which grids are lit by the player's
 * lite source".  In addition to marking grids as "GRID_VIEW" and/or
 * "GRID_LITE", as appropriate, these functions maintain an array for
 * each of these two flags, each array containing the locations of all
 * of the grids marked with the appropriate flag, which can be used to
 * very quickly scan through all of the grids in a given set.
 *
 * To allow more "semantically valid" field of view semantics, whenever
 * the field of view (or the set of torch lit grids) changes, all of the
 * grids in the field of view (or the set of torch lit grids) are "drawn"
 * so that changes in the world will become apparent as soon as possible.
 * This has been optimized so that only grids which actually "change" are
 * redrawn, using the "temp" array and the "GRID_TEMP" flag to keep track
 * of the grids which are entering or leaving the relevent set of grids.
 *
 * These new methods are so efficient that the old nasty code was removed.
 *
 * Note that there is no reason to "update" the "viewable space" unless
 * the player "moves", or walls/doors are created/destroyed, and there
 * is no reason to "update" the "torch lit grids" unless the field of
 * view changes, or the "light radius" changes.  This means that when
 * the player is resting, or digging, or doing anything that does not
 * involve movement or changing the state of the dungeon, there is no
 * need to update the "view" or the "lite" regions, which is nice.
 *
 * Note that the calls to the nasty "los()" function have been reduced
 * to a bare minimum by the use of the new "field of view" calculations.
 *
 * I wouldn't be surprised if slight modifications to the "update_view()"
 * function would allow us to determine "reverse line-of-sight" as well
 * as "normal line-of-sight", which would allow monsters to use a more
 * "correct" calculation to determine if they can "see" the player.  For
 * now, monsters simply "cheat" somewhat and assume that if the player
 * has "line of sight" to the monster, then the monster can "pretend"
 * that it has "line of sight" to the player.
 *
 *
 * The "update_lite()" function maintains the "CAVE_LITE" flag for each
 * grid and maintains an array of all "CAVE_LITE" grids.
 *
 * This set of grids is the complete set of all grids which are lit by
 * the players light source, which allows the "player_can_see_bold()"
 * function to work very quickly.
 *
 * Note that every "CAVE_LITE" grid is also a "CAVE_VIEW" grid, and in
 * fact, the player (unless blind) can always "see" all grids which are
 * marked as "CAVE_LITE", unless they are "off screen".
 *
 *
 * The "update_view()" function maintains the "CAVE_VIEW" flag for each
 * grid and maintains an array of all "CAVE_VIEW" grids.
 *
 * This set of grids is the complete set of all grids within line of sight
 * of the player, allowing the "player_has_los_bold(p_ptr, )" macro to work very
 * quickly.
 *
 *
 * The current "update_view()" algorithm uses the "CAVE_XTRA" flag as a
 * temporary internal flag to mark those grids which are not only in view,
 * but which are also "easily" in line of sight of the player.  This flag
 * is always cleared when we are done.
 *
 *
 * The current "update_lite()" and "update_view()" algorithms use the
 * "CAVE_TEMP" flag, and the array of grids which are marked as "CAVE_TEMP",
 * to keep track of which grids were previously marked as "CAVE_LITE" or
 * "CAVE_VIEW", which allows us to optimize the "screen updates".
 *
 * The "CAVE_TEMP" flag, and the array of "CAVE_TEMP" grids, is also used
 * for various other purposes, such as spreading lite or darkness during
 * "lite_room()" / "unlite_room()", and for calculating monster flow.
 *
 *
 * Any grid can be marked as "CAVE_GLOW" which means that the grid itself is
 * in some way permanently lit.  However, for the player to "see" anything
 * in the grid, as determined by "player_can_see()", the player must not be
 * blind, the grid must be marked as "CAVE_VIEW", and, in addition, "wall"
 * grids, even if marked as "perma lit", are only illuminated if they touch
 * a grid which is not a wall and is marked both "CAVE_GLOW" and "CAVE_VIEW".
 *
 *
 * To simplify various things, a grid may be marked as "CAVE_MARK", meaning
 * that even if the player cannot "see" the grid, he "knows" the terrain in
 * that grid.  This is used to "remember" walls/doors/stairs/floors when they
 * are "seen" or "detected", and also to "memorize" floors, after "wiz_lite(p_ptr, )",
 * or when one of the "memorize floor grids" options induces memorization.
 *
 * Objects are "memorized" in a different way, using a special "marked" flag
 * on the object itself, which is set when an object is observed or detected.
 *
 *
 * A grid may be marked as "CAVE_ROOM" which means that it is part of a "room",
 * and should be illuminated by "lite room" and "darkness" spells.
 *
 *
 * A grid may be marked as "CAVE_ICKY" which means it is part of a "vault",
 * and should be unavailable for "teleportation" destinations.
 *
 *
 * The "view_perma_grids" allows the player to "memorize" every perma-lit grid
 * which is observed, and the "view_torch_grids" allows the player to memorize
 * every torch-lit grid.  The player will always memorize important walls,
 * doors, stairs, and other terrain features, as well as any "detected" grids.
 *
 * Note that the new "update_view()" method allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone of
 * floor appearing as the player gets closer to the door.  Also, by not turning
 * on the "memorize perma-lit grids" option, the player will only "see" those
 * floor grids which are actually in line of sight.
 *
 * And my favorite "plus" is that you can now use a special option to draw the
 * "floors" in the "viewable region" brightly (actually, to draw the *other*
 * grids dimly), providing a "pretty" effect as the player runs around, and
 * to efficiently display the "torch lite" in a special color.
 *
 *
 * Some comments on the "update_view()" algorithm...
 *
 * The algorithm is very fast, since it spreads "obvious" grids very quickly,
 * and only has to call "los()" on the borderline cases.  The major axes/diags
 * even terminate early when they hit walls.  I need to find a quick way
 * to "terminate" the other scans.
 *
 * Note that in the worst case (a big empty area with say 5% scattered walls),
 * each of the 1500 or so nearby grids is checked once, most of them getting
 * an "instant" rating, and only a small portion requiring a call to "los()".
 *
 * The only time that the algorithm appears to be "noticeably" too slow is
 * when running, and this is usually only important in town, since the town
 * provides about the worst scenario possible, with large open regions and
 * a few scattered obstructions.  There is a special "efficiency" option to
 * allow the player to reduce his field of view in town, if needed.
 *
 * In the "best" case (say, a normal stretch of corridor), the algorithm
 * makes one check for each viewable grid, and makes no calls to "los()".
 * So running in corridors is very fast, and if a lot of monsters are
 * nearby, it is much faster than the old methods.
 *
 * Note that resting, most normal commands, and several forms of running,
 * plus all commands executed near large groups of monsters, are strictly
 * more efficient with "update_view()" that with the old "compute los() on
 * demand" method, primarily because once the "field of view" has been
 * calculated, it does not have to be recalculated until the player moves
 * (or a wall or door is created or destroyed).
 *
 * Note that we no longer have to do as many "los()" checks, since once the
 * "view" region has been built, very few things cause it to be "changed"
 * (player movement, and the opening/closing of doors, changes in wall status).
 * Note that door/wall changes are only relevant when the door/wall itself is
 * in the "view" region.
 *
 * The algorithm seems to only call "los()" from zero to ten times, usually
 * only when coming down a corridor into a room, or standing in a room, just
 * misaligned with a corridor.  So if, say, there are five "nearby" monsters,
 * we will be reducing the calls to "los()".
 *
 * I am thinking in terms of an algorithm that "walks" from the central point
 * out to the maximal "distance", at each point, determining the "view" code
 * (above).  For each grid not on a major axis or diagonal, the "view" code
 * depends on the "cave_los_bold(p_ptr->current_floor_ptr, )" and "view" of exactly two other grids
 * (the one along the nearest diagonal, and the one next to that one, see
 * "update_view_aux()"...).
 *
 * We "memorize" the viewable space array, so that at the cost of under 3000
 * bytes, we reduce the time taken by "forget_view()" to one assignment for
 * each grid actually in the "viewable space".  And for another 3000 bytes,
 * we prevent "erase + redraw" ineffiencies via the "seen" set.  These bytes
 * are also used by other routines, thus reducing the cost to almost nothing.
 *
 * A similar thing is done for "forget_lite()" in which case the savings are
 * much less, but save us from doing bizarre maintenance checking.
 *
 * In the worst "normal" case (in the middle of the town), the reachable space
 * actually reaches to more than half of the largest possible "circle" of view,
 * or about 800 grids, and in the worse case (in the middle of a dungeon level
 * where all the walls have been removed), the reachable space actually reaches
 * the theoretical maximum size of just under 1500 grids.
 *
 * Each grid G examines the "state" of two (?) other (adjacent) grids, G1 & G2.
 * If G1 is lite, G is lite.  Else if G2 is lite, G is half.  Else if G1 and G2
 * are both half, G is half.  Else G is dark.  It only takes 2 (or 4) bits to
 * "name" a grid, so (for MAX_RAD of 20) we could use 1600 bytes, and scan the
 * entire possible space (including initialization) in one step per grid.  If
 * we do the "clearing" as a separate step (and use an array of "view" grids),
 * then the clearing will take as many steps as grids that were viewed, and the
 * algorithm will be able to "stop" scanning at various points.
 * Oh, and outside of the "torch radius", only "lite" grids need to be scanned.
 */

/*
 * Mega-Hack -- Delayed visual update
 * Only used if update_view(), update_lite() or update_mon_lite() was called
 */
void delayed_visual_update(void)
{
	int i;
	POSITION y, x;
	grid_type *g_ptr;

	/* Update needed grids */
	for (i = 0; i < p_ptr->current_floor_ptr->redraw_n; i++)
	{
		y = p_ptr->current_floor_ptr->redraw_y[i];
		x = p_ptr->current_floor_ptr->redraw_x[i];
		g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

		/* Update only needed grids (prevent multiple updating) */
		if (!(g_ptr->info & CAVE_REDRAW)) continue;

		/* If required, note */
		if (g_ptr->info & CAVE_NOTE) note_spot(y, x);

		lite_spot(y, x);

		/* Hack -- Visual update of monster on this grid */
		if (g_ptr->m_idx) update_monster(p_ptr, g_ptr->m_idx, FALSE);

		/* No longer in the array */
		g_ptr->info &= ~(CAVE_NOTE | CAVE_REDRAW);
	}

	/* None left */
	p_ptr->current_floor_ptr->redraw_n = 0;
}


/*
 * Hack - speed up the update_flow algorithm by only doing
 * it everytime the player moves out of LOS of the last
 * "way-point".
 */
static POSITION flow_x = 0;
static POSITION flow_y = 0;



/*
 * Hack -- fill in the "cost" field of every grid that the player
 * can "reach" with the number of steps needed to reach that grid.
 * This also yields the "distance" of the player from every grid.
 *
 * In addition, mark the "when" of the grids that can reach
 * the player with the incremented value of "flow_n".
 *
 * Hack -- use the "seen" array as a "circular queue".
 *
 * We do not need a priority queue because the cost from grid
 * to grid is always "one" and we process them in order.
 */
void update_flow(void)
{
	POSITION x, y;
	DIRECTION d;
	int flow_head = 1;
	int flow_tail = 0;

	/* Paranoia -- make sure the array is empty */
	if (tmp_pos.n) return;

	/* The last way-point is on the map */
	if (p_ptr->running && in_bounds(p_ptr->current_floor_ptr, flow_y, flow_x))
	{
		/* The way point is in sight - do not update.  (Speedup) */
		if (p_ptr->current_floor_ptr->grid_array[flow_y][flow_x].info & CAVE_VIEW) return;
	}

	/* Erase all of the current flow information */
	for (y = 0; y < p_ptr->current_floor_ptr->height; y++)
	{
		for (x = 0; x < p_ptr->current_floor_ptr->width; x++)
		{
			p_ptr->current_floor_ptr->grid_array[y][x].cost = 0;
			p_ptr->current_floor_ptr->grid_array[y][x].dist = 0;
		}
	}

	/* Save player position */
	flow_y = p_ptr->y;
	flow_x = p_ptr->x;

	/* Add the player's grid to the queue */
	tmp_pos.y[0] = p_ptr->y;
	tmp_pos.x[0] = p_ptr->x;

	/* Now process the queue */
	while (flow_head != flow_tail)
	{
		int ty, tx;

		/* Extract the next entry */
		ty = tmp_pos.y[flow_tail];
		tx = tmp_pos.x[flow_tail];

		/* Forget that entry */
		if (++flow_tail == TEMP_MAX) flow_tail = 0;

		/* Add the "children" */
		for (d = 0; d < 8; d++)
		{
			int old_head = flow_head;
			byte_hack m = p_ptr->current_floor_ptr->grid_array[ty][tx].cost + 1;
			byte_hack n = p_ptr->current_floor_ptr->grid_array[ty][tx].dist + 1;
			grid_type *g_ptr;

			/* Child location */
			y = ty + ddy_ddd[d];
			x = tx + ddx_ddd[d];

			/* Ignore player's grid */
			if (player_bold(p_ptr, y, x)) continue;

			g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

			if (is_closed_door(g_ptr->feat)) m += 3;

			/* Ignore "pre-stamped" entries */
			if (g_ptr->dist != 0 && g_ptr->dist <= n && g_ptr->cost <= m) continue;

			/* Ignore "walls" and "rubble" */
			if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(g_ptr->feat)) continue;

			/* Save the flow cost */
			if (g_ptr->cost == 0 || g_ptr->cost > m) g_ptr->cost = m;
			if (g_ptr->dist == 0 || g_ptr->dist > n) g_ptr->dist = n;

			/* Hack -- limit flow depth */
			if (n == MONSTER_FLOW_DEPTH) continue;

			/* Enqueue that entry */
			tmp_pos.y[flow_head] = y;
			tmp_pos.x[flow_head] = x;

			/* Advance the queue */
			if (++flow_head == TEMP_MAX) flow_head = 0;

			/* Hack -- notice overflow by forgetting new entry */
			if (flow_head == flow_tail) flow_head = old_head;
		}
	}
}


FEAT_IDX conv_dungeon_feat(FEAT_IDX newfeat)
{
	feature_type *f_ptr = &f_info[newfeat];

	if (have_flag(f_ptr->flags, FF_CONVERT))
	{
		switch (f_ptr->subtype)
		{
		case CONVERT_TYPE_FLOOR:
			return feat_ground_type[randint0(100)];
		case CONVERT_TYPE_WALL:
			return feat_wall_type[randint0(100)];
		case CONVERT_TYPE_INNER:
			return feat_wall_inner;
		case CONVERT_TYPE_OUTER:
			return feat_wall_outer;
		case CONVERT_TYPE_SOLID:
			return feat_wall_solid;
		case CONVERT_TYPE_STREAM1:
			return d_info[p_ptr->dungeon_idx].stream1;
		case CONVERT_TYPE_STREAM2:
			return d_info[p_ptr->dungeon_idx].stream2;
		default:
			return newfeat;
		}
	}
	else return newfeat;
}


/*
 * Take a feature, determine what that feature becomes
 * through applying the given action.
 */
FEAT_IDX feat_state(FEAT_IDX feat, int action)
{
	feature_type *f_ptr = &f_info[feat];
	int i;

	/* Get the new feature */
	for (i = 0; i < MAX_FEAT_STATES; i++)
	{
		if (f_ptr->state[i].action == action) return conv_dungeon_feat(f_ptr->state[i].result);
	}

	if (have_flag(f_ptr->flags, FF_PERMANENT)) return feat;

	return (feature_action_flags[action] & FAF_DESTROY) ? conv_dungeon_feat(f_ptr->destroyed) : feat;
}

/*
 * Takes a location and action and changes the feature at that
 * location through applying the given action.
 */
void cave_alter_feat(POSITION y, POSITION x, int action)
{
	/* Set old feature */
	FEAT_IDX oldfeat = p_ptr->current_floor_ptr->grid_array[y][x].feat;

	/* Get the new feat */
	FEAT_IDX newfeat = feat_state(oldfeat, action);

	/* No change */
	if (newfeat == oldfeat) return;

	/* Set the new feature */
	cave_set_feat(p_ptr->current_floor_ptr, y, x, newfeat);

	if (!(feature_action_flags[action] & FAF_NO_DROP))
	{
		feature_type *old_f_ptr = &f_info[oldfeat];
		feature_type *f_ptr = &f_info[newfeat];
		bool found = FALSE;

		/* Handle gold */
		if (have_flag(old_f_ptr->flags, FF_HAS_GOLD) && !have_flag(f_ptr->flags, FF_HAS_GOLD))
		{
			/* Place some gold */
			place_gold(y, x);
			found = TRUE;
		}

		/* Handle item */
		if (have_flag(old_f_ptr->flags, FF_HAS_ITEM) && !have_flag(f_ptr->flags, FF_HAS_ITEM) && (randint0(100) < (15 - p_ptr->current_floor_ptr->dun_level / 2)))
		{
			/* Place object */
			place_object(y, x, 0L);
			found = TRUE;
		}

		if (found && current_world_ptr->character_dungeon && player_can_see_bold(y, x))
		{
			msg_print(_("何かを発見した！", "You have found something!"));
		}
	}

	if (feature_action_flags[action] & FAF_CRASH_GLASS)
	{
		feature_type *old_f_ptr = &f_info[oldfeat];

		if (have_flag(old_f_ptr->flags, FF_GLASS) && current_world_ptr->character_dungeon)
		{
			project(p_ptr, PROJECT_WHO_GLASS_SHARDS, 1, y, x, MIN(p_ptr->current_floor_ptr->dun_level, 100) / 4, GF_SHARDS,
				(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
		}
	}
}


/* Remove a mirror */
void remove_mirror(POSITION y, POSITION x)
{
	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	/* Remove the mirror */
	g_ptr->info &= ~(CAVE_OBJECT);
	g_ptr->mimic = 0;

	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
	{
		g_ptr->info &= ~(CAVE_GLOW);
		if (!view_torch_grids) g_ptr->info &= ~(CAVE_MARK);
		if (g_ptr->m_idx) update_monster(p_ptr, g_ptr->m_idx, FALSE);

		update_local_illumination(p_ptr, y, x);
	}

	note_spot(y, x);

	lite_spot(y, x);
}


/*
 *  Return TRUE if there is a mirror on the grid.
 */
bool is_mirror_grid(grid_type *g_ptr)
{
	if ((g_ptr->info & CAVE_OBJECT) && have_flag(f_info[g_ptr->mimic].flags, FF_MIRROR))
		return TRUE;
	else
		return FALSE;
}


/*
 *  Return TRUE if there is a mirror on the grid.
 */
bool is_glyph_grid(grid_type *g_ptr)
{
	if ((g_ptr->info & CAVE_OBJECT) && have_flag(f_info[g_ptr->mimic].flags, FF_GLYPH))
		return TRUE;
	else
		return FALSE;
}


/*
 *  Return TRUE if there is a mirror on the grid.
 */
bool is_explosive_rune_grid(grid_type *g_ptr)
{
	if ((g_ptr->info & CAVE_OBJECT) && have_flag(f_info[g_ptr->mimic].flags, FF_MINOR_GLYPH))
		return TRUE;
	else
		return FALSE;
}

/*!
* @brief 指定されたマスがモンスターのテレポート可能先かどうかを判定する。
* @param m_idx モンスターID
* @param y 移動先Y座標
* @param x 移動先X座標
* @param mode オプション
* @return テレポート先として妥当ならばtrue
*/
bool cave_monster_teleportable_bold(MONSTER_IDX m_idx, POSITION y, POSITION x, BIT_FLAGS mode)
{
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[m_idx];
	grid_type    *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Require "teleportable" space */
	if (!have_flag(f_ptr->flags, FF_TELEPORTABLE)) return FALSE;

	if (g_ptr->m_idx && (g_ptr->m_idx != m_idx)) return FALSE;
	if (player_bold(p_ptr, y, x)) return FALSE;

	/* Hack -- no teleport onto glyph of warding */
	if (is_glyph_grid(g_ptr)) return FALSE;
	if (is_explosive_rune_grid(g_ptr)) return FALSE;

	if (!(mode & TELEPORT_PASSIVE))
	{
		if (!monster_can_cross_terrain(g_ptr->feat, &r_info[m_ptr->r_idx], 0)) return FALSE;
	}

	return TRUE;
}

/*!
* @brief 指定されたマスにプレイヤーがテレポート可能かどうかを判定する。
* @param y 移動先Y座標
* @param x 移動先X座標
* @param mode オプション
* @return テレポート先として妥当ならばtrue
*/
bool cave_player_teleportable_bold(POSITION y, POSITION x, BIT_FLAGS mode)
{
	grid_type    *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	/* Require "teleportable" space */
	if (!have_flag(f_ptr->flags, FF_TELEPORTABLE)) return FALSE;

	/* No magical teleporting into vaults and such */
	if (!(mode & TELEPORT_NONMAGICAL) && (g_ptr->info & CAVE_ICKY)) return FALSE;

	if (g_ptr->m_idx && (g_ptr->m_idx != p_ptr->riding)) return FALSE;

	/* don't teleport on a trap. */
	if (have_flag(f_ptr->flags, FF_HIT_TRAP)) return FALSE;

	if (!(mode & TELEPORT_PASSIVE))
	{
		if (!player_can_enter(g_ptr->feat, 0)) return FALSE;

		if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP))
		{
			if (!p_ptr->levitation && !p_ptr->can_swim) return FALSE;
		}

		if (have_flag(f_ptr->flags, FF_LAVA) && !p_ptr->immune_fire && !IS_INVULN())
		{
			/* Always forbid deep lava */
			if (have_flag(f_ptr->flags, FF_DEEP)) return FALSE;

			/* Forbid shallow lava when the player don't have levitation */
			if (!p_ptr->levitation) return FALSE;
		}

	}

	return TRUE;
}

/*!
 * @brief 地形は開くものであって、かつ開かれているかを返す /
 * Attempt to open the given chest at the given location
 * @param feat 地形ID
 * @return 開いた地形である場合TRUEを返す /  Return TRUE if the given feature is an open door
 */
bool is_open(FEAT_IDX feat)
{
	return have_flag(f_info[feat].flags, FF_CLOSE) && (feat != feat_state(feat, FF_CLOSE));
}

/*!
 * @brief プレイヤーが地形踏破可能かを返す
 * @param feature 判定したい地形ID
 * @param mode 移動に関するオプションフラグ
 * @return 移動可能ならばTRUEを返す
 */
bool player_can_enter(FEAT_IDX feature, BIT_FLAGS16 mode)
{
	feature_type *f_ptr = &f_info[feature];

	if (p_ptr->riding) return monster_can_cross_terrain(feature, &r_info[p_ptr->current_floor_ptr->m_list[p_ptr->riding].r_idx], mode | CEM_RIDING);

	if (have_flag(f_ptr->flags, FF_PATTERN))
	{
		if (!(mode & CEM_P_CAN_ENTER_PATTERN)) return FALSE;
	}

	if (have_flag(f_ptr->flags, FF_CAN_FLY) && p_ptr->levitation) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_SWIM) && p_ptr->can_swim) return TRUE;
	if (have_flag(f_ptr->flags, FF_CAN_PASS) && p_ptr->pass_wall) return TRUE;

	if (!have_flag(f_ptr->flags, FF_MOVE)) return FALSE;

	return TRUE;
}

