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
void place_locked_door(POSITION y, POSITION x)
{
	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		set_cave_feat(p_ptr->current_floor_ptr, y, x, feat_locked_door_random((d_info[p_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
		p_ptr->current_floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
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
void place_secret_door(POSITION y, POSITION x, int type)
{
	if (d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_DOORS)
	{
		place_floor_bold(y, x);
	}
	else
	{
		grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

		if (type == DOOR_DEFAULT)
		{
			type = ((d_info[p_ptr->dungeon_idx].flags1 & DF1_CURTAIN) &&
				one_in_((d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) ? 16 : 256)) ? DOOR_CURTAIN :
				((d_info[p_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
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
