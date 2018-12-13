#include "angband.h"
#include "grid.h"
#include "generate.h"
#include "rooms.h"
#include "monsterrace-hook.h"

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
bool build_type15(void)
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
