#include "angband.h"
#include "grid.h"
#include "generate.h"
#include "rooms.h"


/*!
* @brief タイプ14の部屋…特殊トラップ部屋の生成 / Type 14 -- trapped rooms
* @return なし
* @details
* A special trap is placed at center of the room
*/
bool build_type14(void)
{
	POSITION y, x, y2, x2, yval, xval;
	POSITION y1, x1, xsize, ysize;

	bool light;

	cave_type *c_ptr;
	s16b trap;

	/* Pick a room size */
	y1 = randint1(4);
	x1 = randint1(11);
	y2 = randint1(3);
	x2 = randint1(11);

	xsize = x1 + x2 + 1;
	ysize = y1 + y2 + 1;

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
			c_ptr->info |= (CAVE_ROOM);
			if (light) c_ptr->info |= (CAVE_GLOW);
		}
	}

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		c_ptr = &cave[y][x1 - 1];
		place_outer_grid(c_ptr);
		c_ptr = &cave[y][x2 + 1];
		place_outer_grid(c_ptr);
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		c_ptr = &cave[y1 - 1][x];
		place_outer_grid(c_ptr);
		c_ptr = &cave[y2 + 1][x];
		place_outer_grid(c_ptr);
	}

	if (dun_level < 30 + randint1(30))
		trap = feat_trap_piranha;
	else
		trap = feat_trap_armageddon;

	/* Place a special trap */
	c_ptr = &cave[rand_spread(yval, ysize / 4)][rand_spread(xval, xsize / 4)];
	c_ptr->mimic = c_ptr->feat;
	c_ptr->feat = trap;

	msg_format_wizard(CHEAT_DUNGEON, _("%sの部屋が生成されました。", "Room of %s was generated."), f_name + f_info[trap].name);

	return TRUE;
}

