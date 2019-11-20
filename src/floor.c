#include "angband.h"
#include "floor.h"
#include "floor-save.h"
#include "grid.h"
#include "dungeon.h"
#include "rooms.h"

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
		place_floor_bold(y, x);
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
		place_floor_bold(y, x);
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
void update_smell(floor_type *floor_ptr)
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
			y = i + p_ptr->y - 2;
			x = j + p_ptr->x - 2;

			/* Check Bounds */
			if (!in_bounds(floor_ptr, y, x)) continue;

			g_ptr = &floor_ptr->grid_array[y][x];

			/* Walls, water, and lava cannot hold scent. */
			if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(g_ptr->feat)) continue;

			/* Grid must not be blocked by walls from the character */
			if (!player_has_los_bold(p_ptr, y, x)) continue;

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

