/* File: cave.c */

/* Purpose: low level dungeon routines -BEN- */


#include "angband.h"


/*
 * Support for Adam Bolt's tileset, lighting and transparency effects
 * by Robert Ruehlmann (rr9@angband.org)
 */

static byte display_autopick;
static int match_autopick;
static object_type *autopick_obj;
static int feat_priority;

/*
 * Distance between two points via Newton-Raphson technique
 */
int distance (int y1, int x1, int y2, int x2)
{
	int dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
	int dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);

	/* Squared distance */
	int target = (dy * dy) + (dx * dx);

	/* Approximate distance: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2 */
	int d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

	int err;

	/* Simple case */
	if (!dy || !dx) return d;

	while (1)
	{
		/* Approximate error */
		err = (target - d * d) / (2 * d);

		/* No error - we are done */
		if (!err) break;

		/* Adjust distance */
		d += err;
	}

	return d;
}


/*
 * Return TRUE if the given feature is a trap
 */
bool is_trap(int feat)
{
	switch (feat)
	{
		case FEAT_TRAP_TRAPDOOR:
		case FEAT_TRAP_PIT:
		case FEAT_TRAP_SPIKED_PIT:
		case FEAT_TRAP_POISON_PIT:
		case FEAT_TRAP_TY_CURSE:
		case FEAT_TRAP_TELEPORT:
		case FEAT_TRAP_FIRE:
		case FEAT_TRAP_ACID:
		case FEAT_TRAP_SLOW:
		case FEAT_TRAP_LOSE_STR:
		case FEAT_TRAP_LOSE_DEX:
		case FEAT_TRAP_LOSE_CON:
		case FEAT_TRAP_BLIND:
		case FEAT_TRAP_CONFUSE:
		case FEAT_TRAP_POISON:
		case FEAT_TRAP_SLEEP:
		case FEAT_TRAP_TRAPS:
		case FEAT_TRAP_ALARM:
                case FEAT_TRAP_OPEN:
		{
			/* A trap */
			return (TRUE);
		}
		default:
		{
			/* Not a trap */
			return (FALSE);
		}
	}
}


/*
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.
 *
 * Returns TRUE if a line of sight can be traced from (x1,y1) to (x2,y2).
 *
 * The LOS begins at the center of the tile (x1,y1) and ends at the center of
 * the tile (x2,y2).  If los() is to return TRUE, all of the tiles this line
 * passes through must be floor tiles, except for (x1,y1) and (x2,y2).
 *
 * We assume that the "mathematical corner" of a non-floor tile does not
 * block line of sight.
 *
 * Because this function uses (short) ints for all calculations, overflow may
 * occur if dx and dy exceed 90.
 *
 * Once all the degenerate cases are eliminated, the values "qx", "qy", and
 * "m" are multiplied by a scale factor "f1 = abs(dx * dy * 2)", so that
 * we can use integer arithmetic.
 *
 * We travel from start to finish along the longer axis, starting at the border
 * between the first and second tiles, where the y offset = .5 * slope, taking
 * into account the scale factor.  See below.
 *
 * Also note that this function and the "move towards target" code do NOT
 * share the same properties.  Thus, you can see someone, target them, and
 * then fire a bolt at them, but the bolt may hit a wall, not them.  However,
 * by clever choice of target locations, you can sometimes throw a "curve".
 *
 * Note that "line of sight" is not "reflexive" in all cases.
 *
 * Use the "projectable()" routine to test "spell/missile line of sight".
 *
 * Use the "update_view()" function to determine player line-of-sight.
 */
bool los(int y1, int x1, int y2, int x2)
{
	/* Delta */
	int dx, dy;

	/* Absolute */
	int ax, ay;

	/* Signs */
	int sx, sy;

	/* Fractions */
	int qx, qy;

	/* Scanners */
	int tx, ty;

	/* Scale factors */
	int f1, f2;

	/* Slope, or 1/Slope, of LOS */
	int m;


	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);


	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (TRUE);


	/* Paranoia -- require "safe" origin */
	/* if (!in_bounds(y1, x1)) return (FALSE); */
	/* if (!in_bounds(y2, x2)) return (FALSE); */


	/* Directly South/North */
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (!cave_floor_bold(ty, x1)) return (FALSE);
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (!cave_floor_bold(ty, x1)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (!cave_floor_bold(y1, tx)) return (FALSE);
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (!cave_floor_bold(y1, tx)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;


	/* Vertical "knights" */
	if (ax == 1)
	{
		if (ay == 2)
		{
			if (cave_floor_bold(y1 + sy, x1)) return (TRUE);
		}
	}

	/* Horizontal "knights" */
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (cave_floor_bold(y1, x1 + sx)) return (TRUE);
		}
	}


	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay)
	{
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (!cave_floor_bold(ty, tx)) return (FALSE);

			qy += m;

			if (qy < f2)
			{
				tx += sx;
			}
			else if (qy > f2)
			{
				ty += sy;
				if (!cave_floor_bold(ty, tx)) return (FALSE);
				qy -= f1;
				tx += sx;
			}
			else
			{
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else
	{
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2)
		{
			tx = x1 + sx;
			qx -= f1;
		}
		else
		{
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty)
		{
			if (!cave_floor_bold(ty, tx)) return (FALSE);

			qx += m;

			if (qx < f2)
			{
				ty += sy;
			}
			else if (qx > f2)
			{
				tx += sx;
				if (!cave_floor_bold(ty, tx)) return (FALSE);
				qx -= f1;
				ty += sy;
			}
			else
			{
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (TRUE);
}






/*
 * Can the player "see" the given grid in detail?
 *
 * He must have vision, illumination, and line of sight.
 *
 * Note -- "CAVE_LITE" is only set if the "torch" has "los()".
 * So, given "CAVE_LITE", we know that the grid is "fully visible".
 *
 * Note that "CAVE_GLOW" makes little sense for a wall, since it would mean
 * that a wall is visible from any direction.  That would be odd.  Except
 * under wizard light, which might make sense.  Thus, for walls, we require
 * not only that they be "CAVE_GLOW", but also, that they be adjacent to a
 * grid which is not only "CAVE_GLOW", but which is a non-wall, and which is
 * in line of sight of the player.
 *
 * This extra check is expensive, but it provides a more "correct" semantics.
 *
 * Note that we should not run this check on walls which are "outer walls" of
 * the dungeon, or we will induce a memory fault, but actually verifying all
 * of the locations would be extremely expensive.
 *
 * Thus, to speed up the function, we assume that all "perma-walls" which are
 * "CAVE_GLOW" are "illuminated" from all sides.  This is correct for all cases
 * except "vaults" and the "buildings" in town.  But the town is a hack anyway,
 * and the player has more important things on his mind when he is attacking a
 * monster vault.  It is annoying, but an extremely important optimization.
 *
 * Note that "glowing walls" are only considered to be "illuminated" if the
 * grid which is next to the wall in the direction of the player is also a
 * "glowing" grid.  This prevents the player from being able to "see" the
 * walls of illuminated rooms from a corridor outside the room.
 */
bool player_can_see_bold(int y, int x)
{
	int xx, yy;

	cave_type *c_ptr;

	/* Blind players see nothing */
	if (p_ptr->blind) return (FALSE);

	/* Access the cave grid */
	c_ptr = &cave[y][x];

	/* Note that "torch-lite" yields "illumination" */
	if (c_ptr->info & (CAVE_LITE)) return (TRUE);

	/* Require line of sight to the grid */
	if (!player_has_los_bold(y, x)) return (FALSE);

	if (p_ptr->pclass == CLASS_NINJA) return TRUE;

	/* Require "perma-lite" of the grid */
	if (!(c_ptr->info & (CAVE_GLOW | CAVE_MNLT))) return (FALSE);

	/* Floors are simple */
	if (cave_floor_bold(y, x)) return (TRUE);

	/* Hack -- move towards player */
	yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
	xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

	/* Check for "local" illumination */
	if (cave[yy][xx].info & (CAVE_GLOW | CAVE_MNLT))
	{
		/* Assume the wall is really illuminated */
		return (TRUE);
	}

	/* Assume not visible */
	return (FALSE);
}



/*
 * Returns true if the player's grid is dark
 */
bool no_lite(void)
{
	return (!player_can_see_bold(py, px));
}




/*
 * Determine if a given location may be "destroyed"
 *
 * Used by destruction spells, and for placing stairs, etc.
 */
bool cave_valid_bold(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];

	s16b this_o_idx, next_o_idx = 0;


	/* Forbid perma-grids */
	if (cave_perma_grid(c_ptr)) return (FALSE);

	/* Check objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Forbid artifact grids */
		if ((o_ptr->art_name) || artifact_p(o_ptr)) return (FALSE);
	}

	/* Accept */
	return (TRUE);
}




/*
 * Determine if a given location may be "destroyed"
 *
 * Used by destruction spells, and for placing stairs, etc.
 */
bool cave_valid_grid(cave_type *c_ptr)
{
	s16b this_o_idx, next_o_idx = 0;


	/* Forbid perma-grids */
	if (cave_perma_grid(c_ptr)) return (FALSE);

	/* Check objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Forbid artifact grids */
		if ((o_ptr->art_name) || artifact_p(o_ptr)) return (FALSE);
	}

	/* Accept */
	return (TRUE);
}




/*
 * Hack -- Legal monster codes
 */
static cptr image_monster_hack = \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Hack -- Legal monster codes for IBM pseudo-graphics
 */
static cptr image_monster_hack_ibm = \
"aaa";

/*
 * Mega-Hack -- Hallucinatory monster
 */
static void image_monster(byte *ap, char *cp)
{
	int n = strlen(image_monster_hack);

	/* Random symbol from set above */
	if (use_graphics)
	{
		/* Normal graphics */
		if (!(streq(ANGBAND_SYS, "ibm")))
		{
			(*cp) = r_info[randint1(max_r_idx-1)].x_char;
			(*ap) = r_info[randint1(max_r_idx-1)].x_attr;
		}
		else
		/* IBM-pseudo graphics */
		{
			n = strlen(image_monster_hack_ibm);
			(*cp) = (image_monster_hack_ibm[randint0(n)]);

			/* Random color */
			(*ap) = randint1(15);
		}
	}
	else
	/* Text mode */
	{
		(*cp) = (image_monster_hack[randint0(n)]);

		/* Random color */
		(*ap) = randint1(15);
	}
}



/*
 * Hack -- Legal object codes
 */
static cptr image_object_hack = \
"?/|\\\"!$()_-=[]{},~";

static cptr image_object_hack_ibm = \
"aaa";

/*
 * Mega-Hack -- Hallucinatory object
 */
static void image_object(byte *ap, char *cp)
{
	int n = strlen(image_object_hack);

	if (use_graphics)
	{
		if (!(streq(ANGBAND_SYS, "ibm")))
		{
			(*cp) = k_info[randint1(max_k_idx-1)].x_char;
			(*ap) = k_info[randint1(max_k_idx-1)].x_attr;
		}
		else
		{
			n = strlen(image_object_hack_ibm);
			(*cp) = (image_object_hack_ibm[randint0(n)]);

			/* Random color */
			(*ap) = randint1(15);
		}
	}
	else
	{
		(*cp) = (image_object_hack[randint0(n)]);

		/* Random color */
		(*ap) = randint1(15);
	}
}



/*
 * Hack -- Random hallucination
 */
static void image_random(byte *ap, char *cp)
{
	/* Normally, assume monsters */
	if (randint0(100) < 75)
	{
		image_monster(ap, cp);
	}

	/* Otherwise, assume objects */
	else
	{
		image_object(ap, cp);
	}
}

/*
 * Not using graphical tiles for this feature?
 */
#define is_ascii_graphics(C , A) \
    (!(((C) & 0x80) && ((A) & 0x80)))

/*
 * The 16x16 tile of the terrain supports lighting
 */
static bool feat_supports_lighting(byte feat)
{
	if (is_trap(feat)) return streq(ANGBAND_GRAF, "new");

	switch (feat)
	{
	case FEAT_FLOOR:
	case FEAT_INVIS:
	case FEAT_GLYPH:
	case FEAT_LESS:
	case FEAT_MORE:
	case FEAT_LESS_LESS:
	case FEAT_MORE_MORE:
	case FEAT_SECRET:
	case FEAT_RUBBLE:
	case FEAT_MAGMA:
	case FEAT_QUARTZ:
	case FEAT_MAGMA_H:
	case FEAT_QUARTZ_H:
	case FEAT_MAGMA_K:
	case FEAT_QUARTZ_K:
	case FEAT_WALL_EXTRA:
	case FEAT_WALL_INNER:
	case FEAT_WALL_OUTER:
	case FEAT_WALL_SOLID:
	case FEAT_PERM_EXTRA:
	case FEAT_PERM_INNER:
	case FEAT_PERM_OUTER:
	case FEAT_PERM_SOLID:
	case FEAT_MINOR_GLYPH:
	case FEAT_DEEP_WATER:
	case FEAT_SHAL_WATER:
	case FEAT_DEEP_LAVA:
	case FEAT_SHAL_LAVA:
	case FEAT_DARK_PIT:
	case FEAT_DIRT:
	case FEAT_GRASS:
	case FEAT_FLOWER:
	case FEAT_DEEP_GRASS:
	case FEAT_TREES:
	case FEAT_MOUNTAIN:
	case FEAT_MIRROR:
		return TRUE;
	default:
		return FALSE;
	}
}

/*
 * This array lists the effects of "brightness" on various "base" colours.
 *
 * This is used to do dynamic lighting effects in ascii :-)
 * At the moment, only the various "floor" tiles are affected.
 *
 * The layout of the array is [x][0] = light and [x][1] = dark.
 */

static byte lighting_colours[16][2] =
{
	/* TERM_DARK */
	{TERM_L_DARK, TERM_DARK},

	/* TERM_WHITE */
	{TERM_YELLOW, TERM_SLATE},

	/* TERM_SLATE */
	{TERM_WHITE, TERM_L_DARK},

	/* TERM_ORANGE */
	{TERM_L_UMBER, TERM_UMBER},

	/* TERM_RED */
	{TERM_RED, TERM_RED},

	/* TERM_GREEN */
	{TERM_L_GREEN, TERM_GREEN},

	/* TERM_BLUE */
	{TERM_BLUE, TERM_BLUE},

	/* TERM_UMBER */
	{TERM_L_UMBER, TERM_RED},

	/* TERM_L_DARK */
	{TERM_SLATE, TERM_L_DARK},

	/* TERM_L_WHITE */
	{TERM_WHITE, TERM_SLATE},

	/* TERM_VIOLET */
	{TERM_L_RED, TERM_BLUE},

	/* TERM_YELLOW */
	{TERM_YELLOW, TERM_ORANGE},

	/* TERM_L_RED */
	{TERM_L_RED, TERM_L_RED},

	/* TERM_L_GREEN */
	{TERM_L_GREEN, TERM_GREEN},

	/* TERM_L_BLUE */
	{TERM_L_BLUE, TERM_L_BLUE},

	/* TERM_L_UMBER */
	{TERM_L_UMBER, TERM_UMBER}
};

/*
 * Extract the attr/char to display at the given (legal) map location
 *
 * Basically, we "paint" the chosen attr/char in several passes, starting
 * with any known "terrain features" (defaulting to darkness), then adding
 * any known "objects", and finally, adding any known "monsters".  This
 * is not the fastest method but since most of the calls to this function
 * are made for grids with no monsters or objects, it is fast enough.
 *
 * Note that this function, if used on the grid containing the "player",
 * will return the attr/char of the grid underneath the player, and not
 * the actual player attr/char itself, allowing a lot of optimization
 * in various "display" functions.
 *
 * Note that the "zero" entry in the feature/object/monster arrays are
 * used to provide "special" attr/char codes, with "monster zero" being
 * used for the player attr/char, "object zero" being used for the "stack"
 * attr/char, and "feature zero" being used for the "nothing" attr/char,
 * though this function makes use of only "feature zero".
 *
 * Note that monsters can have some "special" flags, including "ATTR_MULTI",
 * which means their color changes, and "ATTR_CLEAR", which means they take
 * the color of whatever is under them, and "CHAR_CLEAR", which means that
 * they take the symbol of whatever is under them.  Technically, the flag
 * "CHAR_MULTI" is supposed to indicate that a monster looks strange when
 * examined, but this flag is currently ignored.
 *
 * Currently, we do nothing with multi-hued objects, because there are
 * not any.  If there were, they would have to set "shimmer_objects"
 * when they were created, and then new "shimmer" code in "dungeon.c"
 * would have to be created handle the "shimmer" effect, and the code
 * in "cave.c" would have to be updated to create the shimmer effect.
 *
 * Note the effects of hallucination.  Objects always appear as random
 * "objects", monsters as random "monsters", and normal grids occasionally
 * appear as random "monsters" or "objects", but note that these random
 * "monsters" and "objects" are really just "colored ascii symbols".
 *
 * Note that "floors" and "invisible traps" (and "zero" features) are
 * drawn as "floors" using a special check for optimization purposes,
 * and these are the only features which get drawn using the special
 * lighting effects activated by "view_special_lite".
 *
 * Note the use of the "mimic" field in the "terrain feature" processing,
 * which allows any feature to "pretend" to be another feature.  This is
 * used to "hide" secret doors, and to make all "doors" appear the same,
 * and all "walls" appear the same, and "hidden" treasure stay hidden.
 * It is possible to use this field to make a feature "look" like a floor,
 * but the "special lighting effects" for floors will not be used.
 *
 * Note the use of the new "terrain feature" information.  Note that the
 * assumption that all interesting "objects" and "terrain features" are
 * memorized allows extremely optimized processing below.  Note the use
 * of separate flags on objects to mark them as memorized allows a grid
 * to have memorized "terrain" without granting knowledge of any object
 * which may appear in that grid.
 *
 * Note the efficient code used to determine if a "floor" grid is
 * "memorized" or "viewable" by the player, where the test for the
 * grid being "viewable" is based on the facts that (1) the grid
 * must be "lit" (torch-lit or perma-lit), (2) the grid must be in
 * line of sight, and (3) the player must not be blind, and uses the
 * assumption that all torch-lit grids are in line of sight.
 *
 * Note that floors (and invisible traps) are the only grids which are
 * not memorized when seen, so only these grids need to check to see if
 * the grid is "viewable" to the player (if it is not memorized).  Since
 * most non-memorized grids are in fact walls, this induces *massive*
 * efficiency, at the cost of *forcing* the memorization of non-floor
 * grids when they are first seen.  Note that "invisible traps" are
 * always treated exactly like "floors", which prevents "cheating".
 *
 * Note the "special lighting effects" which can be activated for floor
 * grids using the "view_special_lite" option (for "white" floor grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_yellow_lite" option is set, we will use
 * "yellow", else if the grid is "dark", we will use "dark gray", else
 * if the grid is not "viewable", and the "view_bright_lite" option is
 * set, and the we will use "slate" (gray).  We will use "white" for all
 * other cases, in particular, for illuminated viewable floor grids.
 *
 * Note the "special lighting effects" which can be activated for wall
 * grids using the "view_granite_lite" option (for "white" wall grids),
 * causing certain grids to be displayed using special colors.  If the
 * player is "blind", we will use "dark gray", else if the grid is lit
 * by the torch, and the "view_yellow_lite" option is set, we will use
 * "yellow", else if the "view_bright_lite" option is set, and the grid
 * is not "viewable", or is "dark", or is glowing, but not when viewed
 * from the player's current location, we will use "slate" (gray).  We
 * will use "white" for all other cases, in particular, for correctly
 * illuminated viewable wall grids.
 *
 * Note that, when "view_granite_lite" is set, we use an inline version
 * of the "player_can_see_bold()" function to check the "viewability" of
 * grids when the "view_bright_lite" option is set, and we do NOT use
 * any special colors for "dark" wall grids, since this would allow the
 * player to notice the walls of illuminated rooms from a hallway that
 * happened to run beside the room.  The alternative, by the way, would
 * be to prevent the generation of hallways next to rooms, but this
 * would still allow problems when digging towards a room.
 *
 * Note that bizarre things must be done when the "attr" and/or "char"
 * codes have the "high-bit" set, since these values are used to encode
 * various "special" pictures in some versions, and certain situations,
 * such as "multi-hued" or "clear" monsters, cause the attr/char codes
 * to be "scrambled" in various ways.
 *
 * Note that eventually we may use the "&" symbol for embedded treasure,
 * and use the "*" symbol to indicate multiple objects, though this will
 * have to wait for Angband 2.8.0 or later.  Note that currently, this
 * is not important, since only one object or terrain feature is allowed
 * in each grid.  If needed, "k_info[0]" will hold the "stack" attr/char.
 *
 * Note the assumption that doing "x_ptr = &x_info[x]" plus a few of
 * "x_ptr->xxx", is quicker than "x_info[x].xxx", if this is incorrect
 * then a whole lot of code should be changed...  XXX XXX
 */
#ifdef USE_TRANSPARENCY
void map_info(int y, int x, byte *ap, char *cp, byte *tap, char *tcp)
#else /* USE_TRANSPARENCY */
void map_info(int y, int x, byte *ap, char *cp)
#endif /* USE_TRANSPARENCY */
{
	cave_type *c_ptr;

	feature_type *f_ptr;

	s16b this_o_idx, next_o_idx = 0;

	byte feat;

	byte a;
	byte c;

	/* Get the cave */
	c_ptr = &cave[y][x];

	/* Feature code */
	feat = c_ptr->mimic ? c_ptr->mimic : c_ptr->feat;
	feat = (c_ptr->info & CAVE_IN_MIRROR) ? FEAT_MIRROR : feat;

	/* Floors (etc) */
	if ((feat <= FEAT_INVIS) || (feat == FEAT_DIRT) || (feat == FEAT_GRASS))
	{
		/* Memorized (or visible) floor */
		if   ((c_ptr->info & CAVE_MARK) ||
		    (((c_ptr->info & CAVE_LITE) || (c_ptr->info & CAVE_MNLT) ||
		     ((c_ptr->info & CAVE_GLOW) &&
		      (c_ptr->info & CAVE_VIEW))) &&
		     !p_ptr->blind))
		{
			/* Access floor */
			f_ptr = &f_info[feat];

			/* Normal char */
			c = f_ptr->x_char;

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Special lighting effects */
			if (view_special_lite && (!p_ptr->wild_mode) && ((a == TERM_WHITE) || use_graphics))
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					if (use_graphics)
					{
						/*
						 * feat_supports_lighting(feat)
						 * is always TRUE here
						 */
						
						/* Use a dark tile */
						c++;
					}
					else
					{
						/* Use "dark gray" */
						a = TERM_L_DARK;
					}
                                }

				/* Handle "torch-lit" grids */
				else if (c_ptr->info & (CAVE_LITE | CAVE_MNLT))
				{
                                        /* Torch lite */
                                        if (view_yellow_lite && !p_ptr->wild_mode)
                                        {
						if (use_graphics)
						{
							/*
							 * feat_supports_lighting(feat)
							 * is always TRUE here
							 */

							/* Use a brightly lit tile */
							c += 2;
						}
						else
						{
							/* Use "yellow" */
							a = TERM_YELLOW;
						}
					}
				}

                                /* Handle "dark" grids */
                                else if (!(c_ptr->info & CAVE_GLOW))
                                {
					if (use_graphics)
					{
						/*
						 * feat_supports_lighting(feat)
						 * is always TRUE here
						 */

						/* Use a dark tile */
						c++;
					}
					else
					{
						/* Use "dark gray" */
						a = TERM_L_DARK;
					}
                                }

				/* Handle "out-of-sight" grids */
				else if (!(c_ptr->info & CAVE_VIEW))
				{
					/* Special flag */
					if (view_bright_lite && !p_ptr->wild_mode)
					{
						if (use_graphics)
						{
							/*
							 * feat_supports_lighting(feat)
							 * is always TRUE here
							 */

							/* Use a dark tile */
							c++;
						}
						else
						{
							/* Use "gray" */
							a = TERM_SLATE;
						}
					}
				}
			}
		}

		/* Unknown */
		else
		{
			/* Unsafe cave grid -- idea borrowed from Unangband */
			if (view_unsafe_grids && (c_ptr->info & (CAVE_UNSAFE)))
				feat = FEAT_UNDETECTD;
			else
				feat = FEAT_NONE;

			/* Access darkness */
			f_ptr = &f_info[feat];

			/* Normal attr */
			a = f_ptr->x_attr;

			/* Normal char */
			c = f_ptr->x_char;
		}
	}

	/* Non floors */
	else
	{
		/* Memorized grids */
		if ((c_ptr->info & CAVE_MARK) && (view_granite_lite || new_ascii_graphics))
		{
			/* Apply "mimic" field */
			if (c_ptr->mimic)
				feat = c_ptr->mimic;
			else
				feat = f_info[feat].mimic;

			/* Access feature */
			f_ptr = &f_info[feat];

			/* Normal char */
			c = f_ptr->x_char;

			/* Normal attr */
			a = f_ptr->x_attr;

			if (new_ascii_graphics)
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					if (is_ascii_graphics(c,a))
					{
						/* Use darkened colour */
						a = lighting_colours[a][1];
					}
					else if (use_graphics && feat_supports_lighting(feat))
					{
						/* Use a dark tile */
						c++;
					}
				}

				/* Handle "torch-lit" grids */
				else if (c_ptr->info & (CAVE_LITE | CAVE_MNLT))
				{
					/* Torch lite */
					if (view_yellow_lite && !p_ptr->wild_mode && ((use_graphics && feat_supports_lighting(feat)) || is_ascii_graphics(c,a)))
					{
						if (is_ascii_graphics(c,a))
						{
							/* Use lightened colour */
							a = lighting_colours[a][0];
						}
						else if (use_graphics &&
								feat_supports_lighting(c_ptr->feat))
						{
							/* Use a brightly lit tile */
							c += 2;
						}
					}
				}

				/* Handle "view_bright_lite" */
				else if (view_bright_lite && !p_ptr->wild_mode && ((use_graphics && feat_supports_lighting(feat)) || is_ascii_graphics(c,a)))
				{
					/* Not viewable */
					if (!(c_ptr->info & CAVE_VIEW))
					{
						if (is_ascii_graphics(c,a))
						{
							/* Use darkened colour */
							a = lighting_colours[a][1];
						}
						else if (use_graphics && feat_supports_lighting(feat))
						{
							/* Use a dark tile */
							c++;
						}
					}

					/* Not glowing */
					else if (!(c_ptr->info & CAVE_GLOW))
					{
						if (is_ascii_graphics(c,a))
						{
							/* Use darkened colour */
							a = lighting_colours[a][1];
						}
					}
				}
			}
			/* Special lighting effects */
			else if (view_granite_lite && !p_ptr->wild_mode &&
			   (((a == TERM_WHITE) && !use_graphics) ||
			   (use_graphics && feat_supports_lighting(c_ptr->feat))))
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					if (use_graphics)
					{
						/* Use a dark tile */
						c++;
					}
					else
					{
						/* Use "dark gray" */
						a = TERM_L_DARK;
					}
				}

				/* Handle "torch-lit" grids */
				else if (c_ptr->info & (CAVE_LITE | CAVE_MNLT))
				{
					/* Torch lite */
					if (view_yellow_lite && !p_ptr->wild_mode)
					{
						if (use_graphics)
						{
							/* Use a brightly lit tile */
							c += 2;
						}
						else
						{
							/* Use "yellow" */
							a = TERM_YELLOW;
						}
					}
				}

				/* Handle "view_bright_lite" */
				else if (view_bright_lite && !p_ptr->wild_mode)
				{
					/* Not viewable */
					if (!(c_ptr->info & CAVE_VIEW))
					{
						if (use_graphics)
						{
							/* Use a dark tile */
							c++;
						}
						else
						{
							/* Use "gray" */
							a = TERM_SLATE;
						}
					}

					/* Not glowing */
					else if (!(c_ptr->info & CAVE_GLOW))
					{
						if (use_graphics)
						{
							/* Use a lit tile */
						}
						else
						{
							/* Use "gray" */
							a = TERM_SLATE;
						}
					}

					/* Not glowing correctly */
					else
					{
						int xx, yy;

						/* Hack -- move towards player */
						yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
						xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

						/* Check for "local" illumination */
						if (!(cave[yy][xx].info & CAVE_GLOW))
						{
							if (use_graphics)
							{
								/* Use a lit tile */
							}
							else
							{
								/* Use "gray" */
								a = TERM_SLATE;
							}
						}
					}
				}
			}
		}

                /* "Simple Lighting" */
                else
                {
                        /* Handle "blind" */
                        if (!(c_ptr->info & CAVE_MARK))
                        {
				/* Unsafe cave grid -- idea borrowed from Unangband */
				if (view_unsafe_grids && (c_ptr->info & (CAVE_UNSAFE)))
					feat = FEAT_UNDETECTD;
				else
					feat = FEAT_NONE;
                        }

                        /* Access feature */
                        f_ptr = &f_info[feat];

                        /* Normal attr */
                        a = f_ptr->x_attr;

                        /* Normal char */
                        c = f_ptr->x_char;
                }
        }

	if (feat_priority == -1)
	{
		switch (feat)
		{
		case FEAT_NONE:
		case FEAT_UNDETECTD:
		case FEAT_DARK_PIT:
			feat_priority = 1;
			break;

		case FEAT_FLOOR:
		case FEAT_INVIS:
		case FEAT_TRAP_TRAPDOOR:
		case FEAT_TRAP_PIT:
		case FEAT_TRAP_SPIKED_PIT:
		case FEAT_TRAP_POISON_PIT:
		case FEAT_TRAP_TY_CURSE:
		case FEAT_TRAP_TELEPORT:
		case FEAT_TRAP_FIRE:
		case FEAT_TRAP_ACID:
		case FEAT_TRAP_SLOW:
		case FEAT_TRAP_LOSE_STR:
		case FEAT_TRAP_LOSE_DEX:
		case FEAT_TRAP_LOSE_CON:
		case FEAT_TRAP_BLIND:
		case FEAT_TRAP_CONFUSE:
		case FEAT_TRAP_POISON:
		case FEAT_TRAP_SLEEP:
		case FEAT_TRAP_TRAPS:
		case FEAT_TRAP_ALARM:
		case FEAT_DIRT:
		case FEAT_GRASS:
		case FEAT_FLOWER:
		case FEAT_DEEP_GRASS:
		case FEAT_SWAMP:
		case FEAT_TREES:
		case FEAT_SECRET:
		case FEAT_RUBBLE:
		case FEAT_MAGMA:
		case FEAT_QUARTZ:
		case FEAT_MAGMA_H:
		case FEAT_QUARTZ_H:
		case FEAT_WALL_EXTRA:
		case FEAT_WALL_INNER:
		case FEAT_WALL_OUTER:
		case FEAT_WALL_SOLID:
		case FEAT_DEEP_WATER:
		case FEAT_SHAL_WATER:
		case FEAT_DEEP_LAVA:
		case FEAT_SHAL_LAVA:
			feat_priority = 2;
			break;
			
		case FEAT_MAGMA_K:
		case FEAT_QUARTZ_K:
			feat_priority = 3;
			break;
			
		case FEAT_MOUNTAIN:
		case FEAT_PERM_EXTRA:
		case FEAT_PERM_INNER:
		case FEAT_PERM_OUTER:
		case FEAT_PERM_SOLID:
			feat_priority = 5;
			break;
			
			/* default is feat_priority = 20; (doors and stores) */ 
			
		case FEAT_GLYPH:
		case FEAT_MINOR_GLYPH:
		case FEAT_MIRROR:
		case FEAT_PATTERN_START:
		case FEAT_PATTERN_1:
		case FEAT_PATTERN_2:
		case FEAT_PATTERN_3:
		case FEAT_PATTERN_4:
		case FEAT_PATTERN_END:
		case FEAT_PATTERN_OLD:
		case FEAT_PATTERN_XTRA1:
		case FEAT_PATTERN_XTRA2:
			feat_priority = 16;
			break;
			
			/* objects have feat_priority = 20 */ 
			/* monsters have feat_priority = 30 */ 
			
		case FEAT_LESS:
		case FEAT_MORE:
		case FEAT_QUEST_ENTER:
		case FEAT_QUEST_EXIT:
		case FEAT_QUEST_DOWN:
		case FEAT_QUEST_UP:
		case FEAT_LESS_LESS:
		case FEAT_MORE_MORE:
		case FEAT_TOWN:
		case FEAT_ENTRANCE:
			feat_priority = 35;
			break;
			
		default:
			feat_priority = 10;
			break;
		}
	}

#ifdef USE_TRANSPARENCY
	/* Save the terrain info for the transparency effects */
	(*tap) = a;
	(*tcp) = c;
#endif /* USE_TRANSPARENCY */

	/* Save the info */
	(*ap) = a;
	(*cp) = c;

	/* Hack -- rare random hallucination, except on outer dungeon walls */
	if (p_ptr->image && (c_ptr->feat < FEAT_PERM_SOLID) && !randint0(256))
	{
		/* Hallucinate */
		image_random(ap, cp);
	}

	/* Objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized objects */
		if (o_ptr->marked)
		{
			if (display_autopick)
			{
				byte act;

				match_autopick = is_autopick(o_ptr);
				if(match_autopick == -1)
					continue;

				act = autopick_list[match_autopick].action;

				if ((act & DO_DISPLAY) && (act & display_autopick))
				{
					autopick_obj = o_ptr;
				}
				else
				{
					match_autopick = -1;
					continue;
				}
			}
			/* Normal char */
			(*cp) = object_char(o_ptr);

			/* Normal attr */
			(*ap) = object_attr(o_ptr);

			feat_priority = 20;

			/* Hack -- hallucination */
			if (p_ptr->image) image_object(ap, cp);

			/* Done */
			break;
		}
	}


	/* Handle monsters */
	if (c_ptr->m_idx && display_autopick == 0 )
	{
		monster_type *m_ptr = &m_list[c_ptr->m_idx];

		/* Visible monster */
		if (m_ptr->ml)
		{
			monster_race *r_ptr;
			r_ptr = &r_info[m_ptr->ap_r_idx];

			/* Desired attr */
			a = r_ptr->x_attr;

			/* Desired char */
			c = r_ptr->x_char;

			feat_priority = 30;

			/* Mimics' colors vary */
			if (strchr("\"!=", c) && !(r_ptr->flags1 & RF1_UNIQUE))
			{
				/* Use char */
				(*cp) = c;

				/* Use semi-random attr */
				(*ap) = c_ptr->m_idx % 15 + 1;
			}

			/* Special attr/char codes */
			else if ((a & 0x80) && (c & 0x80))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Multi-hued monster */
			else if (r_ptr->flags1 & (RF1_ATTR_MULTI))
			{
				/* Is it a shapechanger? */
				if (r_ptr->flags2 & (RF2_SHAPECHANGER))
				{
					if (use_graphics)
					{
						if (!(streq(ANGBAND_SYS, "ibm")))
						{
							(*cp) = r_info[randint1(max_r_idx-1)].x_char;
							(*ap) = r_info[randint1(max_r_idx-1)].x_attr;
						}
						else
						{
							int n =  strlen(image_monster_hack_ibm);
							(*cp) = (image_monster_hack_ibm[randint0(n)]);

							/* Random color */
							(*ap) = randint1(15);
						}
					}
					else
					{
						(*cp) = (one_in_(25) ?
							image_object_hack[randint0(strlen(image_object_hack))] :
							image_monster_hack[randint0(strlen(image_monster_hack))]);
					}
				}
				else
					(*cp) = c;

				/* Multi-hued attr */
				if (r_ptr->flags2 & RF2_ATTR_ANY)
					(*ap) = randint1(15);
				else switch (randint1(7))
				{
					case 1:
						(*ap) = TERM_RED;
						break;
					case 2:
						(*ap) = TERM_L_RED;
						break;
					case 3:
						(*ap) = TERM_WHITE;
						break;
					case 4:
						(*ap) = TERM_L_GREEN;
						break;
					case 5:
						(*ap) = TERM_BLUE;
						break;
					case 6:
						(*ap) = TERM_L_DARK;
						break;
					case 7:
						(*ap) = TERM_GREEN;
						break;
				}
			}

			/* Normal monster (not "clear" in any way) */
			else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR | RF1_CHAR_CLEAR)))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Hack -- Bizarre grid under monster */
			else if ((*ap & 0x80) || (*cp & 0x80))
			{
				/* Use char */
				(*cp) = c;

				/* Use attr */
				(*ap) = a;
			}

			/* Normal */
			else
			{
				/* Normal (non-clear char) monster */
				if (!(r_ptr->flags1 & (RF1_CHAR_CLEAR)))
				{
					/* Normal char */
					(*cp) = c;
				}

				/* Normal (non-clear attr) monster */
				else if (!(r_ptr->flags1 & (RF1_ATTR_CLEAR)))
				{
					/* Normal attr */
					(*ap) = a;
				}
			}

			/* Hack -- hallucination */
			if (p_ptr->image)
			{
				/* Hallucinatory monster */
				image_monster(ap, cp);
			}
		}
	}

	/* Handle "player" */
	if ((y == py) && (x == px))
	{
		monster_race *r_ptr = &r_info[0];

		feat_priority = 31;

		/* Get the "player" attr */
		a = r_ptr->x_attr;

		/* Get the "player" char */
		c = r_ptr->x_char;

#ifdef VARIABLE_PLAYER_GRAPH

		if (!streq(ANGBAND_GRAF, "new"))
		{
			if (streq(ANGBAND_SYS,"ibm"))
			{
				if (use_graphics && player_symbols)
				{
					if (p_ptr->psex == SEX_FEMALE) c = (char)242;
					switch (p_ptr->pclass)
					{
						case CLASS_PALADIN:
							if (p_ptr->lev < 20)
								a = TERM_L_WHITE;
							else
								a = TERM_WHITE;
							c = 253;
							break;
						case CLASS_WARRIOR_MAGE:
						case CLASS_RED_MAGE:
							if (p_ptr->lev < 20)
								a = TERM_L_RED;
							else
								a = TERM_VIOLET;
							break;
						case CLASS_CHAOS_WARRIOR:
							do
							{
								a = randint1(15);
							}
							while (a == TERM_DARK);
							break;
						case CLASS_MAGE:
						case CLASS_HIGH_MAGE:
						case CLASS_SORCERER:
						case CLASS_MAGIC_EATER:
						case CLASS_BLUE_MAGE:
							if (p_ptr->lev < 20)
								a = TERM_L_RED;
							else
								a = TERM_RED;
							c = 248;
							break;
						case CLASS_PRIEST:
						case CLASS_BARD:
							if (p_ptr->lev < 20)
								a = TERM_L_BLUE;
							else
								a = TERM_BLUE;
							c = 248;
							break;
						case CLASS_RANGER:
						case CLASS_ARCHER:
							if (p_ptr->lev < 20)
								a = TERM_L_GREEN;
							else
								a = TERM_GREEN;
							break;
						case CLASS_ROGUE:
						case CLASS_NINJA:
							if (p_ptr->lev < 20)
								a = TERM_SLATE;
							else
								a = TERM_L_DARK;
							break;
						case CLASS_WARRIOR:
						case CLASS_SMITH:
						case CLASS_BERSERKER:
						case CLASS_SAMURAI:
							if (p_ptr->lev < 20)
								a = TERM_L_UMBER;
							else
								a = TERM_UMBER;
							break;
						case CLASS_MONK:
						case CLASS_MINDCRAFTER:
						case CLASS_FORCETRAINER:
						case CLASS_MIRROR_MASTER:
							if (p_ptr->lev < 20)
								a = TERM_L_UMBER;
							else
								a = TERM_UMBER;
							c = 248;
							break;
						default: /* Unknown */
							a = TERM_WHITE;
					}

					switch (p_ptr->prace)
					{
						case RACE_GNOME:
						case RACE_HOBBIT:
							c = 144;
							break;
						case RACE_DWARF:
							c = 236;
							break;
						case RACE_HALF_ORC:
							c = 243;
							break;
						case RACE_HALF_TROLL:
							c = 184;
							break;
						case RACE_ELF:
						case RACE_ENT:
						case RACE_HALF_ELF:
						case RACE_HIGH_ELF:
						case RACE_KUTA:
							c = 223;
							break;
						case RACE_HALF_OGRE:
							c = 168;
							break;
						case RACE_HALF_GIANT:
						case RACE_HALF_TITAN:
						case RACE_CYCLOPS:
							c = 145;
							break;
						case RACE_YEEK:
							c = 209;
							break;
						case RACE_KLACKON:
							c = 229;
							break;
						case RACE_KOBOLD:
							c = 204;
							break;
						case RACE_NIBELUNG:
							c = 144;
							break;
						case RACE_DARK_ELF:
							c = 223;
							break;
						case RACE_DRACONIAN:
							if (p_ptr->lev < 20)
								c = 240;
							else if (p_ptr->lev < 40)
								c = 22;
							else
								c = 137;
							break;
						case RACE_MIND_FLAYER:
							c = 236;
							break;
						case RACE_IMP:
							c = 142;
							break;
						case RACE_GOLEM:
						case RACE_ANDROID:
							c = 6;
							break;
						case RACE_SKELETON:
							if (p_ptr->pclass == CLASS_MAGE ||
								p_ptr->pclass == CLASS_PRIEST ||
								p_ptr->pclass == CLASS_HIGH_MAGE ||
								p_ptr->pclass == CLASS_SORCERER ||
								p_ptr->pclass == CLASS_MONK ||
								p_ptr->pclass == CLASS_FORCETRAINER ||
								p_ptr->pclass == CLASS_BLUE_MAGE ||
								p_ptr->pclass == CLASS_MIRROR_MASTER ||
								p_ptr->pclass == CLASS_MINDCRAFTER)
								c = 159;
							else
								c = 181;
							break;
						case RACE_ZOMBIE:
							c = 221;
							break;
						case RACE_VAMPIRE:
							c = 217;
							break;
						case RACE_SPECTRE:
							c = 241;
							break;
						case RACE_SPRITE:
						case RACE_S_FAIRY:
							c = 244;
							break;
						case RACE_BEASTMAN:
							c = 154;
							break;
						case RACE_ANGEL:
						case RACE_DEMON:
							c = 144;
							break;
					}
				}
			}
		}

		/* Save the info */
		(*ap) = a;
		(*cp) = c;

#endif /* VARIABLE_PLAYER_GRAPH */

	}
}


#ifdef JP
/*
 * Table of Ascii-to-Zenkaku
 * °÷¢£°◊§œ∆Û«‹…˝∆¶…Â§Œ∆‚…Ù•≥°º•…§Àª»Õ—°£
 */
static char ascii_to_zenkaku[2*128+1] =  "\
°°°™°…°Ù°°Û°ı°«° °À°ˆ°‹°§°›°•°ø\
£∞£±£≤£≥£¥£µ£∂£∑£∏£π°ß°®°„°·°‰°©\
°˜£¡£¬£√£ƒ£≈£∆£«£»£…£ £À£Ã£Õ£Œ£œ\
£–£—£“£”£‘£’£÷£◊£ÿ£Ÿ£⁄°Œ°¿°œ°∞°≤\
°∆£·£‚£„£‰£Â£Ê£Á£Ë£È£Í£Î£Ï£Ì£Ó£Ô\
££Ò£Ú£Û£Ù£ı£ˆ£˜£¯£˘£˙°–°√°—°¡¢£";
#endif

/*
 * Prepare Bigtile or 2-bytes character attr/char pairs
 */
static void bigtile_attr(char *cp, byte *ap, char *cp2, byte *ap2)
{
	if (*ap & 0x80)
	{
		*ap2 = 255;
		*cp2 = -1;
		return;
	}

#ifdef JP
	if (isprint(*cp) || *cp == 127)
	{
		*ap2 = (*ap) | 0xf0;
		*cp2 = ascii_to_zenkaku[2*(*cp-' ') + 1];
		*cp = ascii_to_zenkaku[2*(*cp-' ')];
		return;
	}
#endif

	*ap2 = TERM_WHITE;
	*cp2 = ' ';
}


/*
 * Calculate panel colum of a location in the map
 */
static int panel_col_of(int col)
{
	col -= panel_col_min;
	if (use_bigtile) col *= 2;
	return col + 13; 
}


/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
	/* Real co-ords convert to screen positions */
	row -= panel_row_prt;

	/* Go there */
	Term_gotoxy(panel_col_of(col), row);
}



/*
 * Place an attr/char pair at the given map coordinate, if legal.
 */
void print_rel(char c, byte a, int y, int x)
{
	char c2;
	byte a2;

	/* Only do "legal" locations */
	if (panel_contains(y, x))
	{
		/* Hack -- fake monochrome */
		if (!use_graphics || streq(ANGBAND_SYS, "ibm"))
		{
			if (world_monster) a = TERM_DARK;
			else if (p_ptr->invuln || world_player) a = TERM_WHITE;
			else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) a = TERM_WHITE;
			else if (p_ptr->wraith_form) a = TERM_L_DARK;
		}

		if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

		/* Draw the char using the attr */
		Term_draw(panel_col_of(x), y-panel_row_prt, a, c);
		if (use_bigtile)
			Term_draw(panel_col_of(x)+1, y-panel_row_prt, a2, c2);
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
void note_spot(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];

	s16b this_o_idx, next_o_idx = 0;


	/* Blind players see nothing */
	if (p_ptr->blind) return;

	/* Analyze non-torch-lit grids */
	if (!(c_ptr->info & (CAVE_LITE)))
	{
		/* Require line of sight to the grid */
		if (!(c_ptr->info & (CAVE_VIEW))) return;

		if (p_ptr->pclass != CLASS_NINJA)
		{
		/* Require "perma-lite" of the grid */
		if (!(c_ptr->info & (CAVE_GLOW | CAVE_MNLT))) return;
		}
	}


	/* Hack -- memorize objects */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Memorize objects */
		o_ptr->marked |= OM_FOUND;
	}


	/* Hack -- memorize grids */
	if (!(c_ptr->info & (CAVE_MARK)))
	{
		if (p_ptr->pclass == CLASS_NINJA)
		{
			c_ptr->info |= (CAVE_MARK);
		}
		/* Handle floor grids first */
		if ((c_ptr->feat <= FEAT_INVIS) || (c_ptr->feat == FEAT_DIRT) || (c_ptr->feat == FEAT_GRASS))
		{
			/* Option -- memorize all torch-lit floors */
			if (view_torch_grids && (c_ptr->info & (CAVE_LITE | CAVE_MNLT)))
			{
				/* Memorize */
				c_ptr->info |= (CAVE_MARK);
			}

			/* Option -- memorize all perma-lit floors */
			else if (view_perma_grids && (c_ptr->info & (CAVE_GLOW)))
			{
				/* Memorize */
				c_ptr->info |= (CAVE_MARK);
			}
		}

		/* Memorize normal grids */
		else if (cave_floor_grid(c_ptr))
		{
			/* Memorize */
			c_ptr->info |= (CAVE_MARK);
		}

		/* Memorize torch-lit walls */
		else if (c_ptr->info & (CAVE_LITE | CAVE_MNLT))
		{
			/* Memorize */
			c_ptr->info |= (CAVE_MARK);
		}

		/* Memorize certain non-torch-lit wall grids */
		else
		{
			int yy, xx;

			/* Hack -- move one grid towards player */
			yy = (y < py) ? (y + 1) : (y > py) ? (y - 1) : y;
			xx = (x < px) ? (x + 1) : (x > px) ? (x - 1) : x;

			/* Check for "local" illumination */
			if (cave[yy][xx].info & (CAVE_GLOW))
			{
				/* Memorize */
				c_ptr->info |= (CAVE_MARK);
			}
		}
	}
}


void display_dungeon(void)
{
	int x, y;
	byte a;
	char c;

#ifdef USE_TRANSPARENCY
	byte ta;
	char tc;
#endif /* USE_TRANSPARENCY */

	for (x = px - Term->wid / 2 + 1; x <= px + Term->wid / 2; x++)
	{
		for (y = py - Term->hgt / 2 + 1; y <= py + Term->hgt / 2; y++)
		{
			if (in_bounds2(y, x))
			{

#ifdef USE_TRANSPARENCY
				/* Examine the grid */
				map_info(y, x, &a, &c, &ta, &tc);
#else /* USE_TRANSPARENCY */
				/* Examine the grid */
				map_info(y, x, &a, &c);
#endif /* USE_TRANSPARENCY */

				/* Hack -- fake monochrome */
				if (!use_graphics || streq(ANGBAND_SYS, "ibm"))
				{
					if (world_monster) a = TERM_DARK;
					else if (p_ptr->invuln || world_player) a = TERM_WHITE;
					else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) a = TERM_WHITE;
					else if (p_ptr->wraith_form) a = TERM_L_DARK;
				}

#ifdef USE_TRANSPARENCY
				/* Hack -- Queue it */
				Term_queue_char(x - px + Term->wid / 2 - 1, y - py + Term->hgt / 2 - 1, a, c, ta, tc);
#else /* USE_TRANSPARENCY */
				/* Hack -- Queue it */
				Term_queue_char(x - px + Term->wid / 2 - 1, y - py + Term->hgt / 2 - 1, a, c);
#endif /* USE_TRANSPARENCY */

			}
			else
			{
				/* Clear out-of-bound tiles */

				/* Access darkness */
				feature_type *f_ptr = &f_info[FEAT_NONE];

				/* Normal attr */
				a = f_ptr->x_attr;

				/* Normal char */
				c = f_ptr->x_char;

#ifdef USE_TRANSPARENCY
				/* Hack -- Queue it */
				Term_queue_char(x - px + Term->wid / 2 - 1, y - py + Term->hgt / 2 - 1, a, c, ta, tc);
#else /* USE_TRANSPARENCY */
				/* Hack -- Queue it */
				Term_queue_char(x - px + Term->wid / 2 - 1, y - py + Term->hgt / 2 - 1, a, c);
#endif /* USE_TRANSPARENCY */
			}
		}
	}
}


/*
 * Redraw (on the screen) a given MAP location
 *
 * This function should only be called on "legal" grids
 */
void lite_spot(int y, int x)
{
	/* Redraw if on screen */
	if (panel_contains(y, x) && in_bounds2(y, x))
	{
		byte a;
		char c;

#ifdef USE_TRANSPARENCY
		byte ta;
		char tc;

		/* Examine the grid */
		map_info(y, x, &a, &c, &ta, &tc);
#else /* USE_TRANSPARENCY */
		/* Examine the grid */
		map_info(y, x, &a, &c);
#endif /* USE_TRANSPARENCY */

		/* Hack -- fake monochrome */
		if (!use_graphics || streq(ANGBAND_SYS, "ibm"))
		{
			if (world_monster) a = TERM_DARK;
			else if (p_ptr->invuln || world_player) a = TERM_WHITE;
			else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) a = TERM_WHITE;
			else if (p_ptr->wraith_form) a = TERM_L_DARK;
		}

#ifdef JP
		if (use_bigtile && !(a & 0x80) && (isprint(c) || c == 127))
		{
			/* Term_queue_chars §œ¡¥≥—ASCII√œ∑¡§Ú¿µ§∑§Øupdate§π§Î°£ */
			Term_queue_chars(panel_col_of(x), y-panel_row_prt, 2, a, &ascii_to_zenkaku[2*(c-' ')]);
			return;
		}
#endif

#ifdef USE_TRANSPARENCY
		/* Hack -- Queue it */
		Term_queue_char(panel_col_of(x), y-panel_row_prt, a, c, ta, tc);
		if (use_bigtile)
			Term_queue_char(panel_col_of(x)+1, y-panel_row_prt, 255, -1, 0, 0);
#else /* USE_TRANSPARENCY */
		/* Hack -- Queue it */
		Term_queue_char(panel_col_of(x), y-panel_row_prt, a, c);
		if (use_bigtile)
			Term_queue_char(panel_col_of(x)+1, y-panel_row_prt, 255, -1);
#endif /* USE_TRANSPARENCY */
	}
}


/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
void prt_map(void)
{
	int     x, y;
	int     v;

	/* map bounds */
	s16b xmin, xmax, ymin, ymax;

	int wid, hgt;

	bool    fake_monochrome = (!use_graphics || streq(ANGBAND_SYS, "ibm"));

	/* Get size */
	Term_get_size(&wid, &hgt);

	/* Remove map offset */
	wid -= COL_MAP + 2;
	hgt -= ROW_MAP + 2;

	/* Access the cursor state */
	(void)Term_get_cursor(&v);

	/* Hide the cursor */
	(void)Term_set_cursor(0);

	/* Get bounds */
	xmin = (0 < panel_col_min) ? panel_col_min : 0;
	xmax = (cur_wid - 1 > panel_col_max) ? panel_col_max : cur_wid - 1;
	ymin = (0 < panel_row_min) ? panel_row_min : 0;
	ymax = (cur_hgt - 1 > panel_row_max) ? panel_row_max : cur_hgt - 1;

	/* Bottom section of screen */
	for (y = 1; y <= ymin - panel_row_prt; y++)
	{
		/* Erase the section */
		Term_erase(COL_MAP, y, wid);
	}

	/* Top section of screen */
	for (y = ymax - panel_row_prt; y <= hgt; y++)
	{
		/* Erase the section */
		Term_erase(COL_MAP, y, wid);
	}

	/* Dump the map */
	for (y = ymin; y <= ymax; y++)
	{
		/* Scan the columns of row "y" */
		for (x = xmin; x <= xmax; x++)
		{
			byte a, a2;
			char c, c2;

#ifdef USE_TRANSPARENCY
			byte ta;
			char tc;

			/* Determine what is there */
			map_info(y, x, &a, &c, &ta, &tc);
#else
			/* Determine what is there */
			map_info(y, x, &a, &c);
#endif

			/* Hack -- fake monochrome */
			if (fake_monochrome)
			{
				if (world_monster) a = TERM_DARK;
				else if (p_ptr->invuln || world_player) a = TERM_WHITE;
				else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) a = TERM_WHITE;
				else if (p_ptr->wraith_form) a = TERM_L_DARK;
			}

			if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

			/* Efficiency -- Redraw that grid of the map */
#ifdef USE_TRANSPARENCY
			Term_queue_char(panel_col_of(x), y-panel_row_prt, a, c, ta, tc);
			if (use_bigtile) Term_queue_char(panel_col_of(x)+1, y-panel_row_prt, a2, c2, 0, 0);
#else
			Term_queue_char(panel_col_of(x), y-panel_row_prt, a, c);
			if (use_bigtile) Term_queue_char(panel_col_of(x)+1, y-panel_row_prt, a2, c2);
#endif
		}
	}

	/* Display player */
	lite_spot(py, px);

	/* Restore the cursor */
	(void)Term_set_cursor(v);
}



/*
 * print project path
 */
void prt_path(int y, int x)
{
	int i;
	int path_n;
	u16b path_g[512];
	int default_color = TERM_SLATE;
	bool    fake_monochrome = (!use_graphics || streq(ANGBAND_SYS, "ibm"));

	if (!display_path) return;
	if (-1 == project_length)
		return;

	/* Get projection path */
	path_n = project_path(path_g, (project_length ? project_length : MAX_RANGE), py, px, y, x, PROJECT_PATH|PROJECT_THRU);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Redraw stuff */
	redraw_stuff();

	/* Draw path */
	for (i = 0; i < path_n; i++)
	{
		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);

		if (panel_contains(ny, nx))
		{
			byte a2, a = default_color;
			char c, c2;

#ifdef USE_TRANSPARENCY
			byte ta;
			char tc;
#endif

			if (cave[ny][nx].m_idx && m_list[cave[ny][nx].m_idx].ml)
			{
				/* Determine what is there */
#ifdef USE_TRANSPARENCY
				map_info(ny, nx, &a, &c, &ta, &tc);
#else
				map_info(ny, nx, &a, &c);
#endif
				if (a & 0x80)
					a = default_color;
				else if (c == '.' && (a == TERM_WHITE || a == TERM_L_WHITE))
					a = default_color;
				else if (a == default_color)
					a = TERM_WHITE;
			}

			if (fake_monochrome)
			{
				if (world_monster) a = TERM_DARK;
				else if (p_ptr->invuln || world_player) a = TERM_WHITE;
				else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) a = TERM_WHITE;
				else if (p_ptr->wraith_form) a = TERM_L_DARK;
			}

			c = '*';
			if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

			/* Hack -- Queue it */
#ifdef USE_TRANSPARENCY
			Term_queue_char(panel_col_of(nx), ny-panel_row_prt, a, c, ta, tc);
			if (use_bigtile) Term_queue_char(panel_col_of(nx)+1, ny-panel_row_prt, a, c2, 0, 0);
#else
			Term_queue_char(panel_col_of(nx), ny-panel_row_prt, a, c);
			if (use_bigtile) Term_queue_char(panel_col_of(nx)+1, ny-panel_row_prt, a, c2);
#endif
		}

		/* Known Wall */
		if ((cave[ny][nx].info & CAVE_MARK) && !cave_floor_bold(ny, nx)) break;

		/* Change color */
		if (nx == x && ny == y) default_color = TERM_L_DARK;
	}
}


static cptr simplify_list[][2] =
{
#ifdef JP
	{"§ŒÀ‚À°ΩÒ", ""},
	{NULL, NULL}
#else
	{"^Ring of ",   "="},
	{"^Amulet of ", "\""},
	{"^Scroll of ", "?"},
	{"^Scroll titled ", "?"},
	{"^Wand of "  , "-"},
	{"^Rod of "   , "-"},
	{"^Staff of " , "_"},
	{"^Potion of ", "!"},
	{" Spellbook ",""},
	{"^Book of ",   ""},
	{" Magic [",   "["},
	{" Book [",    "["},
	{" Arts [",    "["},
	{"^Set of ",    ""},
	{"^Pair of ",   ""},
	{NULL, NULL}
#endif
};

static void display_shortened_item_name(object_type *o_ptr, int y)
{
	char buf[MAX_NLEN];
	char *c = buf;
	int len = 0;
	byte attr;

	object_desc(buf, o_ptr, FALSE, 0);
	attr = tval_to_attr[o_ptr->tval % 128];

	if (p_ptr->image)
	{
		attr = TERM_WHITE;
#ifdef JP
		strcpy(buf, "≤ø§´¥ÒÃØ§  ™");
#else
		strcpy(buf, "something strange");
#endif
	}

	for (c = buf; *c; c++)
	{
		int i;
		for (i = 0; simplify_list[i][1]; i++)
		{
			cptr org_w = simplify_list[i][0];

			if (*org_w == '^')
			{
				if (c == buf)
					org_w++;
				else
					continue;
			}

			if (!strncmp(c, org_w, strlen(org_w)))
			{
				char *s = c;
				cptr tmp = simplify_list[i][1];
				while (*tmp)
					*s++ = *tmp++;
				tmp = c + strlen(org_w);
				while (*tmp)
					*s++ = *tmp++;
				*s = '\0';
			}
		}
	}

	c = buf;
	len = 0;
	/* »æ≥— 12  ∏ª˙ ¨§«¿⁄§Î */
	while(*c)
	{
#ifdef JP
		if(iskanji(*c))
		{
			if(len + 2 > 12) break;
			c+=2;
			len+=2;
		}
		else
#endif
		{
			if(len + 1 > 12) break;
			c++;
			len++;
		}
	}
	*c='\0';
	Term_putstr(0, y, 12, attr, buf);
}

/*
 * Display a "small-scale" map of the dungeon in the active Term
 */
void display_map(int *cy, int *cx)
{
	int i, j, x, y;

	byte ta, a2;
	char tc, c2;

	byte tp;

	byte **bigma;
	char **bigmc;
	byte **bigmp;

	byte **ma;
	char **mc;
	byte **mp;

	/* Save lighting effects */
	bool old_view_special_lite = view_special_lite;
	bool old_view_granite_lite = view_granite_lite;

	bool fake_monochrome = (!use_graphics || streq(ANGBAND_SYS, "ibm"));

	int hgt, wid, yrat, xrat;

        int **match_autopick_yx;
	object_type ***object_autopick_yx;

	/* Get size */
	Term_get_size(&wid, &hgt);
	hgt -= 2;
	wid -= 14;
	if (use_bigtile) wid /= 2;

	yrat = (cur_hgt + hgt - 1) / hgt;
	xrat = (cur_wid + wid - 1) / wid;

	/* Disable lighting effects */
	view_special_lite = FALSE;
	view_granite_lite = FALSE;

	/* Allocate the maps */
	C_MAKE(ma, (hgt + 2), byte_ptr);
	C_MAKE(mc, (hgt + 2), char_ptr);
	C_MAKE(mp, (hgt + 2), byte_ptr);
	C_MAKE(match_autopick_yx, (hgt + 2), sint_ptr);
	C_MAKE(object_autopick_yx, (hgt + 2), object_type **);

	/* Allocate and wipe each line map */
	for (y = 0; y < (hgt + 2); y++)
	{
		/* Allocate one row each array */
		C_MAKE(ma[y], (wid + 2), byte);
		C_MAKE(mc[y], (wid + 2), char);
		C_MAKE(mp[y], (wid + 2), byte);
		C_MAKE(match_autopick_yx[y], (wid + 2), int);
		C_MAKE(object_autopick_yx[y], (wid + 2), object_type *);

		for (x = 0; x < wid + 2; ++x)
		{
			match_autopick_yx[y][x] = -1;
			object_autopick_yx[y][x] = NULL;

			/* Nothing here */
			ma[y][x] = TERM_WHITE;
			mc[y][x] = ' ';

			/* No priority */
			mp[y][x] = 0;
		}
	}

	/* Allocate the maps */
	C_MAKE(bigma, (cur_hgt + 2), byte_ptr);
	C_MAKE(bigmc, (cur_hgt + 2), char_ptr);
	C_MAKE(bigmp, (cur_hgt + 2), byte_ptr);

	/* Allocate and wipe each line map */
	for (y = 0; y < (cur_hgt + 2); y++)
	{
		/* Allocate one row each array */
		C_MAKE(bigma[y], (cur_wid + 2), byte);
		C_MAKE(bigmc[y], (cur_wid + 2), char);
		C_MAKE(bigmp[y], (cur_wid + 2), byte);

		for (x = 0; x < cur_wid + 2; ++x)
		{
			/* Nothing here */
			bigma[y][x] = TERM_WHITE;
			bigmc[y][x] = ' ';

			/* No priority */
			bigmp[y][x] = 0;
		}
	}

	/* Fill in the map */
	for (i = 0; i < cur_wid; ++i)
	{
		for (j = 0; j < cur_hgt; ++j)
		{
			/* Location */
			x = i / xrat + 1;
			y = j / yrat + 1;

			match_autopick=-1;
			autopick_obj=NULL;
			feat_priority = -1;

			/* Extract the current attr/char at that map location */
#ifdef USE_TRANSPARENCY
			map_info(j, i, &ta, &tc, &ta, &tc);
#else /* USE_TRANSPARENCY */
			map_info(j, i, &ta, &tc);
#endif /* USE_TRANSPARENCY */

			/* Extract the priority */
			tp = feat_priority;

			if(match_autopick!=-1
			   && (match_autopick_yx[y][x] == -1
			       || match_autopick_yx[y][x] > match_autopick))
			{
				match_autopick_yx[y][x] = match_autopick;
				object_autopick_yx[y][x] = autopick_obj;
				tp = 0x7f;
			}

			/* Save the char, attr and priority */
			bigmc[j+1][i+1] = tc;
			bigma[j+1][i+1] = ta;
			bigmp[j+1][i+1] = tp;
		}
	}

	for (j = 0; j < cur_hgt; ++j)
	{
		for (i = 0; i < cur_wid; ++i)
		{
			/* Location */
			x = i / xrat + 1;
			y = j / yrat + 1;

			tc = bigmc[j+1][i+1];
			ta = bigma[j+1][i+1];
			tp = bigmp[j+1][i+1];

			/* rare feature has more priority */
			if (mp[y][x] == tp)
			{
				int t;
				int cnt = 0;

				for (t = 0; t < 8; t++)
				{
					if (tc == bigmc[j+1+ddy_cdd[t]][i+1+ddx_cdd[t]] &&
					    ta == bigma[j+1+ddy_cdd[t]][i+1+ddx_cdd[t]])
						cnt++;
				}
				if (cnt <= 4)
					tp++;
			}

			/* Save "best" */
			if (mp[y][x] < tp)
			{
				/* Save the char, attr and priority */
				mc[y][x] = tc;
				ma[y][x] = ta;
				mp[y][x] = tp;
			}
		}
	}


	/* Corners */
	x = wid + 1;
	y = hgt + 1;

	/* Draw the corners */
	mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';

	/* Draw the horizontal edges */
	for (x = 1; x <= wid; x++) mc[0][x] = mc[y][x] = '-';

	/* Draw the vertical edges */
	for (y = 1; y <= hgt; y++) mc[y][0] = mc[y][x] = '|';


	/* Display each map line in order */
	for (y = 0; y < hgt + 2; ++y)
	{
		/* Start a new line */
		Term_gotoxy(COL_MAP, y);

		/* Display the line */
		for (x = 0; x < wid + 2; ++x)
		{
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (fake_monochrome)
			{
				if (world_monster) ta = TERM_DARK;
				else if (p_ptr->invuln || world_player) ta = TERM_WHITE;
				else if ((p_ptr->pclass == CLASS_BARD) && (p_ptr->magic_num1[0] == MUSIC_INVULN)) ta = TERM_WHITE;
				else if (p_ptr->wraith_form) ta = TERM_L_DARK;
			}

			if (use_bigtile) bigtile_attr(&tc, &ta, &c2, &a2);

			/* Add the character */
			Term_addch(ta, tc);
			if (use_bigtile) Term_addch(a2, c2);
		}
	}


        for (y = 1; y < hgt + 1; ++y)
	{
	  match_autopick = -1;
	  for (x = 1; x <= wid; x++){
	    if (match_autopick_yx[y][x] != -1 &&
		(match_autopick > match_autopick_yx[y][x] ||
		 match_autopick == -1)){
	      match_autopick = match_autopick_yx[y][x];
	      autopick_obj = object_autopick_yx[y][x];
	    }
	  }

	  /* Clear old display */
	  Term_putstr(0, y, 12, 0, "            ");

	  if (match_autopick != -1)
#if 1
		  display_shortened_item_name(autopick_obj, y);
#else
	  {
		  char buf[13] = "\0";
		  strncpy(buf,autopick_list[match_autopick].name,12);
		  buf[12] = '\0';
		  put_str(buf,y,0); 
	  }
#endif

	}

	/* Player location */
		(*cy) = py / yrat + 1 + ROW_MAP;
	if (!use_bigtile)
		(*cx) = px / xrat + 1 + COL_MAP;
	else
		(*cx) = (px / xrat + 1) * 2 + COL_MAP;

	/* Restore lighting effects */
	view_special_lite = old_view_special_lite;
	view_granite_lite = old_view_granite_lite;

	/* Free each line map */
	for (y = 0; y < (hgt + 2); y++)
	{
		/* Free one row each array */
		C_FREE(ma[y], (wid + 2), byte);
		C_FREE(mc[y], (wid + 2), char);
		C_FREE(mp[y], (wid + 2), byte);
		C_FREE(match_autopick_yx[y], (wid + 2), int);
		C_FREE(object_autopick_yx[y], (wid + 2), object_type **);
	}

	/* Free each line map */
	C_FREE(ma, (hgt + 2), byte_ptr);
	C_FREE(mc, (hgt + 2), char_ptr);
	C_FREE(mp, (hgt + 2), byte_ptr);
	C_FREE(match_autopick_yx, (hgt + 2), sint_ptr);
	C_FREE(object_autopick_yx, (hgt + 2), object_type **);

	/* Free each line map */
	for (y = 0; y < (cur_hgt + 2); y++)
	{
		/* Free one row each array */
		C_FREE(bigma[y], (cur_wid + 2), byte);
		C_FREE(bigmc[y], (cur_wid + 2), char);
		C_FREE(bigmp[y], (cur_wid + 2), byte);
	}

 	/* Free each line map */
	C_FREE(bigma, (cur_hgt + 2), byte_ptr);
	C_FREE(bigmc, (cur_hgt + 2), char_ptr);
	C_FREE(bigmp, (cur_hgt + 2), byte_ptr);
}


/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.  XXX XXX XXX
 */
void do_cmd_view_map(void)
{
	int cy, cx;


	/* Save the screen */
	screen_save();

	/* Note */
#ifdef JP
prt("§™¬‘§¡≤º§µ§§...", 0, 0);
#else
	prt("Please wait...", 0, 0);
#endif

	/* Flush */
	Term_fresh();

	/* Clear the screen */
	Term_clear();

        display_autopick = 0;

	/* Display the map */
	display_map(&cy, &cx);

	/* Wait for it */
        if(max_autopick && !p_ptr->wild_mode)
	{
		display_autopick = ITEM_DISPLAY;

		while (1)
		{
			int i;
			byte flag;

			int wid, hgt, row_message;

			Term_get_size(&wid, &hgt);
			row_message = hgt - 1;

#ifdef JP
			put_str("≤ø§´•≠°º§Ú≤°§∑§∆§Ø§¿§µ§§('M':Ω¶§¶ 'N': ¸√÷ 'D':M+N 'K':≤ı§π•¢•§•∆•‡§Ú…Ωº®)", row_message, 1);
#else
			put_str(" Hit M, N(for ~), K(for !), or D(same as M+N) to display auto-picker items.", row_message, 1);
#endif

			/* Hilite the player */
			move_cursor(cy, cx);

			i = inkey();

			if ('M' == i)
				flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK);
			else if ('N' == i)
				flag = DONT_AUTOPICK;
			else if ('K' == i)
				flag = DO_AUTODESTROY;
			else if ('D' == i)
				flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK | DONT_AUTOPICK);
			else
				break;

			Term_fresh();
			
			if (~display_autopick & flag)
				display_autopick |= flag;
			else
				display_autopick &= ~flag;
			/* Display the map */
			display_map(&cy, &cx);
		}
		
		display_autopick = 0;

	}
	else
	{
#ifdef JP
		put_str("≤ø§´•≠°º§Ú≤°§π§»•≤°º•‡§ÀÃ·§Í§ﬁ§π", 23, 30);
#else
		put_str("Hit any key to continue", 23, 30);
#endif		/* Hilite the player */
		move_cursor(cy, cx);
		/* Get any key */
		inkey();
	}

	/* Restore the screen */
	screen_load();
}





/*
 * Some comments on the cave grid flags.  -BEN-
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
 * of the player, allowing the "player_has_los_bold()" macro to work very
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
 * are "seen" or "detected", and also to "memorize" floors, after "wiz_lite()",
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
 * depends on the "cave_floor_bold()" and "view" of exactly two other grids
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
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(void)
{
	int i, x, y;

	/* None to forget */
	if (!lite_n) return;

	/* Clear them all */
	for (i = 0; i < lite_n; i++)
	{
		y = lite_y[i];
		x = lite_x[i];

		/* Forget "LITE" flag */
		cave[y][x].info &= ~(CAVE_LITE);

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	lite_n = 0;
}


/*
 * XXX XXX XXX
 *
 * This macro allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
#define cave_lite_hack(Y,X) \
{\
    if (!(cave[Y][X].info & (CAVE_LITE))) { \
    cave[Y][X].info |= (CAVE_LITE); \
    lite_y[lite_n] = (Y); \
    lite_x[lite_n] = (X); \
			    lite_n++;} \
}


/*
 * Update the set of grids "illuminated" by the player's lite.
 *
 * This routine needs to use the results of "update_view()"
 *
 * Note that "blindness" does NOT affect "torch lite".  Be careful!
 *
 * We optimize most lites (all non-artifact lites) by using "obvious"
 * facts about the results of "small" lite radius, and we attempt to
 * list the "nearby" grids before the more "distant" ones in the
 * array of torch-lit grids.
 *
 * We assume that "radius zero" lite is in fact no lite at all.
 *
 *     Torch     Lantern     Artifacts
 *     (etc)
 *                              ***
 *                 ***         *****
 *      ***       *****       *******
 *      *@*       **@**       ***@***
 *      ***       *****       *******
 *                 ***         *****
 *                              ***
 */
void update_lite(void)
{
	int i, x, y, min_x, max_x, min_y, max_y;
	int p = p_ptr->cur_lite;

	/*** Special case ***/

	/* Hack -- Player has no lite */
	if (p <= 0)
	{
		/* Forget the old lite */
		forget_lite();

		/* Draw the player */
		lite_spot(py, px);
	}


	/*** Save the old "lite" grids for later ***/

	/* Clear them all */
	for (i = 0; i < lite_n; i++)
	{
		y = lite_y[i];
		x = lite_x[i];

		/* Mark the grid as not "lite" */
		cave[y][x].info &= ~(CAVE_LITE);

		/* Mark the grid as "seen" */
		cave[y][x].info |= (CAVE_TEMP);

		/* Add it to the "seen" set */
		temp_y[temp_n] = y;
		temp_x[temp_n] = x;
		temp_n++;
	}

	/* None left */
	lite_n = 0;


	/*** Collect the new "lite" grids ***/

	/* Radius 1 -- torch radius */
	if (p >= 1)
	{
		/* Player grid */
		cave_lite_hack(py, px);

		/* Adjacent grid */
		cave_lite_hack(py+1, px);
		cave_lite_hack(py-1, px);
		cave_lite_hack(py, px+1);
		cave_lite_hack(py, px-1);

		/* Diagonal grids */
		cave_lite_hack(py+1, px+1);
		cave_lite_hack(py+1, px-1);
		cave_lite_hack(py-1, px+1);
		cave_lite_hack(py-1, px-1);
	}

	/* Radius 2 -- lantern radius */
	if (p >= 2)
	{
		/* South of the player */
		if (cave_floor_bold(py+1, px))
		{
			cave_lite_hack(py+2, px);
			cave_lite_hack(py+2, px+1);
			cave_lite_hack(py+2, px-1);
		}

		/* North of the player */
		if (cave_floor_bold(py-1, px))
		{
			cave_lite_hack(py-2, px);
			cave_lite_hack(py-2, px+1);
			cave_lite_hack(py-2, px-1);
		}

		/* East of the player */
		if (cave_floor_bold(py, px+1))
		{
			cave_lite_hack(py, px+2);
			cave_lite_hack(py+1, px+2);
			cave_lite_hack(py-1, px+2);
		}

		/* West of the player */
		if (cave_floor_bold(py, px-1))
		{
			cave_lite_hack(py, px-2);
			cave_lite_hack(py+1, px-2);
			cave_lite_hack(py-1, px-2);
		}
	}

	/* Radius 3+ -- artifact radius */
	if (p >= 3)
	{
		int d;

		/* Paranoia -- see "LITE_MAX" */
		if (p > 14) p = 14;

		/* South-East of the player */
		if (cave_floor_bold(py+1, px+1))
		{
			cave_lite_hack(py+2, px+2);
		}

		/* South-West of the player */
		if (cave_floor_bold(py+1, px-1))
		{
			cave_lite_hack(py+2, px-2);
		}

		/* North-East of the player */
		if (cave_floor_bold(py-1, px+1))
		{
			cave_lite_hack(py-2, px+2);
		}

		/* North-West of the player */
		if (cave_floor_bold(py-1, px-1))
		{
			cave_lite_hack(py-2, px-2);
		}

		/* Maximal north */
		min_y = py - p;
		if (min_y < 0) min_y = 0;

		/* Maximal south */
		max_y = py + p;
		if (max_y > cur_hgt-1) max_y = cur_hgt-1;

		/* Maximal west */
		min_x = px - p;
		if (min_x < 0) min_x = 0;

		/* Maximal east */
		max_x = px + p;
		if (max_x > cur_wid-1) max_x = cur_wid-1;

		/* Scan the maximal box */
		for (y = min_y; y <= max_y; y++)
		{
			for (x = min_x; x <= max_x; x++)
			{
				int dy = (py > y) ? (py - y) : (y - py);
				int dx = (px > x) ? (px - x) : (x - px);

				/* Skip the "central" grids (above) */
				if ((dy <= 2) && (dx <= 2)) continue;

				/* Hack -- approximate the distance */
				d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));

				/* Skip distant grids */
				if (d > p) continue;

				/* Viewable, nearby, grids get "torch lit" */
				if (player_has_los_bold(y, x))
				{
					/* This grid is "torch lit" */
					cave_lite_hack(y, x);
				}
			}
		}
	}


	/*** Complete the algorithm ***/

	/* Draw the new grids */
	for (i = 0; i < lite_n; i++)
	{
		y = lite_y[i];
		x = lite_x[i];

		/* Update fresh grids */
		if (cave[y][x].info & (CAVE_TEMP)) continue;

		/* Note */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* Clear them all */
	for (i = 0; i < temp_n; i++)
	{
		y = temp_y[i];
		x = temp_x[i];

		/* No longer in the array */
		cave[y][x].info &= ~(CAVE_TEMP);

		/* Update stale grids */
		if (cave[y][x].info & (CAVE_LITE)) continue;

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	temp_n = 0;
}


static bool mon_invis;

/*
 * Add a square to the changes array
 */
static void mon_lite_hack(int y, int x)
{
	cave_type *c_ptr;

	/* Out of bounds */
	if (!in_bounds2(y, x)) return;

	c_ptr = &cave[y][x];

	/* Want a unlit square in view of the player */
	if ((c_ptr->info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW) return;

	/* Hack XXX XXX - Is it a wall and monster not in LOS? */
	if (!cave_floor_grid(c_ptr) && mon_invis) return;

	/* Save this square */
	if (temp_n < TEMP_MAX)
	{
		temp_x[temp_n] = x;
		temp_y[temp_n] = y;
		temp_n++;
	}

	/* Light it */
	c_ptr->info |= CAVE_MNLT;
}

 


/*
 * Update squares illuminated by monsters.
 *
 * Hack - use the CAVE_ROOM flag (renamed to be CAVE_MNLT) to
 * denote squares illuminated by monsters.
 *
 * The CAVE_TEMP flag is used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 */
void update_mon_lite(void)
{
	int i, rad;
	cave_type *c_ptr;

	s16b fx, fy;

	s16b end_temp;

	/* Clear all monster lit squares */
	for (i = 0; i < mon_lite_n; i++)
	{
		/* Point to grid */
		c_ptr = &cave[mon_lite_y[i]][mon_lite_x[i]];

		/* Set temp flag */
		c_ptr->info |= (CAVE_TEMP);

		/* Clear monster illumination flag */
		c_ptr->info &= ~(CAVE_MNLT);
	}

	/* Empty temp list of new squares to lite up */
	temp_n = 0;

	/* Loop through monsters, adding newly lit squares to changes list */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Is it too far away? */
		if (m_ptr->cdis > ((d_info[dungeon_type].flags1 & DF1_DARKNESS) ? MAX_SIGHT / 2 + 1 : MAX_SIGHT + 3)) continue;

		/* Get lite radius */
		rad = 0;

		/* Note the radii are cumulative */
		if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_SELF_LITE_1)) rad++;
		if (r_ptr->flags7 & (RF7_HAS_LITE_2 | RF7_SELF_LITE_2)) rad += 2;

		/* Exit if has no light */
		if (!rad) continue;
		if (!(r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) && (m_ptr->csleep || (!dun_level && is_daytime()) || p_ptr->inside_battle)) continue;

		if (world_monster) continue;

		if (d_info[dungeon_type].flags1 & DF1_DARKNESS) rad = 1;

		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;

		/* Is the monster visible? */
		mon_invis = !(cave[fy][fx].info & CAVE_VIEW);

		/* The square it is on */
		mon_lite_hack(fy, fx);

		/* Adjacent squares */
		mon_lite_hack(fy + 1, fx);
		mon_lite_hack(fy - 1, fx);
		mon_lite_hack(fy, fx + 1);
		mon_lite_hack(fy, fx - 1);
		mon_lite_hack(fy + 1, fx + 1);
		mon_lite_hack(fy + 1, fx - 1);
		mon_lite_hack(fy - 1, fx + 1);
		mon_lite_hack(fy - 1, fx - 1);

		/* Radius 2 */
		if (rad >= 2)
		{
			/* South of the monster */
			if (cave_floor_bold(fy + 1, fx))
			{
				mon_lite_hack(fy + 2, fx + 1);
				mon_lite_hack(fy + 2, fx);
				mon_lite_hack(fy + 2, fx - 1);

				c_ptr = &cave[fy + 2][fx];

				/* Radius 3 */
				if ((rad == 3) && cave_floor_grid(c_ptr))
				{
					mon_lite_hack(fy + 3, fx + 1);
					mon_lite_hack(fy + 3, fx);
					mon_lite_hack(fy + 3, fx - 1);
				}
			}

			/* North of the monster */
			if (cave_floor_bold(fy - 1, fx))
			{
				mon_lite_hack(fy - 2, fx + 1);
				mon_lite_hack(fy - 2, fx);
				mon_lite_hack(fy - 2, fx - 1);

				c_ptr = &cave[fy - 2][fx];

				/* Radius 3 */
				if ((rad == 3) && cave_floor_grid(c_ptr))
				{
					mon_lite_hack(fy - 3, fx + 1);
					mon_lite_hack(fy - 3, fx);
					mon_lite_hack(fy - 3, fx - 1);
				}
			}

			/* East of the monster */
			if (cave_floor_bold(fy, fx + 1))
			{
				mon_lite_hack(fy + 1, fx + 2);
				mon_lite_hack(fy, fx + 2);
				mon_lite_hack(fy - 1, fx + 2);

				c_ptr = &cave[fy][fx + 2];

				/* Radius 3 */
				if ((rad == 3) && cave_floor_grid(c_ptr))
				{
					mon_lite_hack(fy + 1, fx + 3);
					mon_lite_hack(fy, fx + 3);
					mon_lite_hack(fy - 1, fx + 3);
				}
			}

			/* West of the monster */
			if (cave_floor_bold(fy, fx - 1))
			{
				mon_lite_hack(fy + 1, fx - 2);
				mon_lite_hack(fy, fx - 2);
				mon_lite_hack(fy - 1, fx - 2);

				c_ptr = &cave[fy][fx - 2];

				/* Radius 3 */
				if ((rad == 3) && cave_floor_grid(c_ptr))
				{
					mon_lite_hack(fy + 1, fx - 3);
					mon_lite_hack(fy, fx - 3);
					mon_lite_hack(fy - 1, fx - 3);
				}
			}
		}

		/* Radius 3 */
		if (rad == 3)
		{
			/* South-East of the monster */
			if (cave_floor_bold(fy + 1, fx + 1))
			{
				mon_lite_hack(fy + 2, fx + 2);
			}

			/* South-West of the monster */
			if (cave_floor_bold(fy + 1, fx - 1))
			{
				mon_lite_hack(fy + 2, fx - 2);
			}

			/* North-East of the monster */
			if (cave_floor_bold(fy - 1, fx + 1))
			{
				mon_lite_hack(fy - 2, fx + 2);
			}

			/* North-West of the monster */
			if (cave_floor_bold(fy - 1, fx - 1))
			{
				mon_lite_hack(fy - 2, fx - 2);
			}
		}
	}

	/* Save end of list of new squares */
	end_temp = temp_n;

	/*
	 * Look at old set flags to see if there are any changes.
	 */
	for (i = 0; i < mon_lite_n; i++)
	{
		fx = mon_lite_x[i];
		fy = mon_lite_y[i];

		if (!in_bounds2(fy, fx)) continue;

		/* Point to grid */
		c_ptr = &cave[fy][fx];

		/* It it no longer lit? */
		if (!(c_ptr->info & CAVE_MNLT) && player_has_los_grid(c_ptr))
		{
			/* It is now unlit */
			note_spot(fy, fx);
			lite_spot(fy, fx);
		}

		/* Add to end of temp array */
		temp_x[temp_n] = (byte)fx;
		temp_y[temp_n] = (byte)fy;
		temp_n++;
	}

	/* Clear the lite array */
	mon_lite_n = 0;

	/* Copy the temp array into the lit array lighting the new squares. */
	for (i = 0; i < temp_n; i++)
	{
		fx = temp_x[i];
		fy = temp_y[i];

		if (!in_bounds2(fy, fx)) continue;

		/* Point to grid */
		c_ptr = &cave[fy][fx];

		if (i >= end_temp)
		{
			/* Clear the temp flag for the old lit grids */
			c_ptr->info &= ~(CAVE_TEMP);
		}
		else
		{
			/* The is the square newly lit and visible? */
			if ((c_ptr->info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW)
			{
				/* It is now lit */
				lite_spot(fy, fx);
				note_spot(fy, fx);
			}

			/* Save in the monster lit array */
			mon_lite_x[mon_lite_n] = fx;
			mon_lite_y[mon_lite_n] = fy;
			mon_lite_n++;
		}
	}

	/* Finished with temp_n */
	temp_n = 0;

	p_ptr->monlite = (cave[py][px].info & CAVE_MNLT) ? TRUE : FALSE;

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (p_ptr->old_monlite != p_ptr->monlite)
		{
			if (p_ptr->monlite)
			{
#ifdef JP
				msg_print("±∆§Œ §§§§¨«ˆ§Ï§øµ§§¨§π§Î°£");
#else
				msg_print("Your mantle of shadow become thin.");
#endif
			}
			else
			{
#ifdef JP
				msg_print("±∆§Œ §§§§¨«ª§Ø§ §√§ø°™");
#else
				msg_print("Your mantle of shadow restored its original darkness.");
#endif
			}
		}
	}
	p_ptr->old_monlite = p_ptr->monlite;
}

void clear_mon_lite(void)
{
	int i;
	cave_type *c_ptr;

	/* Clear all monster lit squares */
	for (i = 0; i < mon_lite_n; i++)
	{
		/* Point to grid */
		c_ptr = &cave[mon_lite_y[i]][mon_lite_x[i]];

		/* Clear monster illumination flag */
		c_ptr->info &= ~(CAVE_MNLT);
	}

	/* Empty the array */
	mon_lite_n = 0;
}



/*
 * Clear the viewable space
 */
void forget_view(void)
{
	int i;

	cave_type *c_ptr;

	/* None to forget */
	if (!view_n) return;

	/* Clear them all */
	for (i = 0; i < view_n; i++)
	{
		int y = view_y[i];
		int x = view_x[i];

		/* Access the grid */
		c_ptr = &cave[y][x];

		/* Forget that the grid is viewable */
		c_ptr->info &= ~(CAVE_VIEW);

		if (!panel_contains(y, x)) continue;

		/* Update the screen */
		lite_spot(y, x);
	}

	/* None left */
	view_n = 0;
}



/*
 * This macro allows us to efficiently add a grid to the "view" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "view" array, and we are never
 * called when the "view" array is full.
 */
#define cave_view_hack(C,Y,X) \
{\
    if (!((C)->info & (CAVE_VIEW))){\
    (C)->info |= (CAVE_VIEW); \
    view_y[view_n] = (Y); \
    view_x[view_n] = (X); \
    view_n++;}\
}



/*
 * Helper function for "update_view()" below
 *
 * We are checking the "viewability" of grid (y,x) by the player.
 *
 * This function assumes that (y,x) is legal (i.e. on the map).
 *
 * Grid (y1,x1) is on the "diagonal" between (py,px) and (y,x)
 * Grid (y2,x2) is "adjacent", also between (py,px) and (y,x).
 *
 * Note that we are using the "CAVE_XTRA" field for marking grids as
 * "easily viewable".  This bit is cleared at the end of "update_view()".
 *
 * This function adds (y,x) to the "viewable set" if necessary.
 *
 * This function now returns "TRUE" if vision is "blocked" by grid (y,x).
 */
static bool update_view_aux(int y, int x, int y1, int x1, int y2, int x2)
{
	bool f1, f2, v1, v2, z1, z2, wall;

	cave_type *c_ptr;

	cave_type *g1_c_ptr;
	cave_type *g2_c_ptr;

	/* Access the grids */
	g1_c_ptr = &cave[y1][x1];
	g2_c_ptr = &cave[y2][x2];


	/* Check for walls */
	f1 = (cave_floor_grid(g1_c_ptr));
	f2 = (cave_floor_grid(g2_c_ptr));

	/* Totally blocked by physical walls */
	if (!f1 && !f2) return (TRUE);


	/* Check for visibility */
	v1 = (f1 && (g1_c_ptr->info & (CAVE_VIEW)));
	v2 = (f2 && (g2_c_ptr->info & (CAVE_VIEW)));

	/* Totally blocked by "unviewable neighbors" */
	if (!v1 && !v2) return (TRUE);


	/* Access the grid */
	c_ptr = &cave[y][x];


	/* Check for walls */
	wall = (!cave_floor_grid(c_ptr));


	/* Check the "ease" of visibility */
	z1 = (v1 && (g1_c_ptr->info & (CAVE_XTRA)));
	z2 = (v2 && (g2_c_ptr->info & (CAVE_XTRA)));

	/* Hack -- "easy" plus "easy" yields "easy" */
	if (z1 && z2)
	{
		c_ptr->info |= (CAVE_XTRA);

		cave_view_hack(c_ptr, y, x);

		return (wall);
	}

	/* Hack -- primary "easy" yields "viewed" */
	if (z1)
	{
		cave_view_hack(c_ptr, y, x);

		return (wall);
	}

	/* Hack -- "view" plus "view" yields "view" */
	if (v1 && v2)
	{
		/* c_ptr->info |= (CAVE_XTRA); */

		cave_view_hack(c_ptr, y, x);

		return (wall);
	}


	/* Mega-Hack -- the "los()" function works poorly on walls */
	if (wall)
	{
		cave_view_hack(c_ptr, y, x);

		return (wall);
	}


	/* Hack -- check line of sight */
	if (los(py, px, y, x))
	{
		cave_view_hack(c_ptr, y, x);

		return (wall);
	}


	/* Assume no line of sight. */
	return (TRUE);
}



/*
 * Calculate the viewable space
 *
 *  1: Process the player
 *  1a: The player is always (easily) viewable
 *  2: Process the diagonals
 *  2a: The diagonals are (easily) viewable up to the first wall
 *  2b: But never go more than 2/3 of the "full" distance
 *  3: Process the main axes
 *  3a: The main axes are (easily) viewable up to the first wall
 *  3b: But never go more than the "full" distance
 *  4: Process sequential "strips" in each of the eight octants
 *  4a: Each strip runs along the previous strip
 *  4b: The main axes are "previous" to the first strip
 *  4c: Process both "sides" of each "direction" of each strip
 *  4c1: Each side aborts as soon as possible
 *  4c2: Each side tells the next strip how far it has to check
 *
 * Note that the octant processing involves some pretty interesting
 * observations involving when a grid might possibly be viewable from
 * a given grid, and on the order in which the strips are processed.
 *
 * Note the use of the mathematical facts shown below, which derive
 * from the fact that (1 < sqrt(2) < 1.5), and that the length of the
 * hypotenuse of a right triangle is primarily determined by the length
 * of the longest side, when one side is small, and is strictly less
 * than one-and-a-half times as long as the longest side when both of
 * the sides are large.
 *
 *   if (manhatten(dy,dx) < R) then (hypot(dy,dx) < R)
 *   if (manhatten(dy,dx) > R*3/2) then (hypot(dy,dx) > R)
 *
 *   hypot(dy,dx) is approximated by (dx+dy+MAX(dx,dy)) / 2
 *
 * These observations are important because the calculation of the actual
 * value of "hypot(dx,dy)" is extremely expensive, involving square roots,
 * while for small values (up to about 20 or so), the approximations above
 * are correct to within an error of at most one grid or so.
 *
 * Observe the use of "full" and "over" in the code below, and the use of
 * the specialized calculation involving "limit", all of which derive from
 * the observations given above.  Basically, we note that the "circle" of
 * view is completely contained in an "octagon" whose bounds are easy to
 * determine, and that only a few steps are needed to derive the actual
 * bounds of the circle given the bounds of the octagon.
 *
 * Note that by skipping all the grids in the corners of the octagon, we
 * place an upper limit on the number of grids in the field of view, given
 * that "full" is never more than 20.  Of the 1681 grids in the "square" of
 * view, only about 1475 of these are in the "octagon" of view, and even
 * fewer are in the "circle" of view, so 1500 or 1536 is more than enough
 * entries to completely contain the actual field of view.
 *
 * Note also the care taken to prevent "running off the map".  The use of
 * explicit checks on the "validity" of the "diagonal", and the fact that
 * the loops are never allowed to "leave" the map, lets "update_view_aux()"
 * use the optimized "cave_floor_bold()" macro, and to avoid the overhead
 * of multiple checks on the validity of grids.
 *
 * Note the "optimizations" involving the "se","sw","ne","nw","es","en",
 * "ws","wn" variables.  They work like this: While travelling down the
 * south-bound strip just to the east of the main south axis, as soon as
 * we get to a grid which does not "transmit" viewing, if all of the strips
 * preceding us (in this case, just the main axis) had terminated at or before
 * the same point, then we can stop, and reset the "max distance" to ourself.
 * So, each strip (named by major axis plus offset, thus "se" in this case)
 * maintains a "blockage" variable, initialized during the main axis step,
 * and checks it whenever a blockage is observed.  After processing each
 * strip as far as the previous strip told us to process, the next strip is
 * told not to go farther than the current strip's farthest viewable grid,
 * unless open space is still available.  This uses the "k" variable.
 *
 * Note the use of "inline" macros for efficiency.  The "cave_floor_grid()"
 * macro is a replacement for "cave_floor_bold()" which takes a pointer to
 * a cave grid instead of its location.  The "cave_view_hack()" macro is a
 * chunk of code which adds the given location to the "view" array if it
 * is not already there, using both the actual location and a pointer to
 * the cave grid.  See above.
 *
 * By the way, the purpose of this code is to reduce the dependancy on the
 * "los()" function which is slow, and, in some cases, not very accurate.
 *
 * It is very possible that I am the only person who fully understands this
 * function, and for that I am truly sorry, but efficiency was very important
 * and the "simple" version of this function was just not fast enough.  I am
 * more than willing to replace this function with a simpler one, if it is
 * equally efficient, and especially willing if the new function happens to
 * derive "reverse-line-of-sight" at the same time, since currently monsters
 * just use an optimized hack of "you see me, so I see you", and then use the
 * actual "projectable()" function to check spell attacks.
 */
void update_view(void)
{
	int n, m, d, k, y, x, z;

	int se, sw, ne, nw, es, en, ws, wn;

	int full, over;

	int y_max = cur_hgt - 1;
	int x_max = cur_wid - 1;

	cave_type *c_ptr;

	/*** Initialize ***/

	/* Optimize */
	if (view_reduce_view && !dun_level)
	{
		/* Full radius (10) */
		full = MAX_SIGHT / 2;

		/* Octagon factor (15) */
		over = MAX_SIGHT * 3 / 4;
	}

	/* Normal */
	else
	{
		/* Full radius (20) */
		full = MAX_SIGHT;

		/* Octagon factor (30) */
		over = MAX_SIGHT * 3 / 2;
	}


	/*** Step 0 -- Begin ***/

	/* Save the old "view" grids for later */
	for (n = 0; n < view_n; n++)
	{
		y = view_y[n];
		x = view_x[n];

		/* Access the grid */
		c_ptr = &cave[y][x];

		/* Mark the grid as not in "view" */
		c_ptr->info &= ~(CAVE_VIEW);

		/* Mark the grid as "seen" */
		c_ptr->info |= (CAVE_TEMP);

		/* Add it to the "seen" set */
		temp_y[temp_n] = y;
		temp_x[temp_n] = x;
		temp_n++;
	}

	/* Start over with the "view" array */
	view_n = 0;

	/*** Step 1 -- adjacent grids ***/

	/* Now start on the player */
	y = py;
	x = px;

	/* Access the grid */
	c_ptr = &cave[y][x];

	/* Assume the player grid is easily viewable */
	c_ptr->info |= (CAVE_XTRA);

	/* Assume the player grid is viewable */
	cave_view_hack(c_ptr, y, x);


	/*** Step 2 -- Major Diagonals ***/

	/* Hack -- Limit */
	z = full * 2 / 3;

	/* Scan south-east */
	for (d = 1; d <= z; d++)
	{
		c_ptr = &cave[y+d][x+d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y+d, x+d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Scan south-west */
	for (d = 1; d <= z; d++)
	{
		c_ptr = &cave[y+d][x-d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y+d, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Scan north-east */
	for (d = 1; d <= z; d++)
	{
		c_ptr = &cave[y-d][x+d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y-d, x+d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Scan north-west */
	for (d = 1; d <= z; d++)
	{
		c_ptr = &cave[y-d][x-d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y-d, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}


	/*** Step 3 -- major axes ***/

	/* Scan south */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &cave[y+d][x];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y+d, x);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "south strips" */
	se = sw = d;

	/* Scan north */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &cave[y-d][x];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y-d, x);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "north strips" */
	ne = nw = d;

	/* Scan east */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &cave[y][x+d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y, x+d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "east strips" */
	es = en = d;

	/* Scan west */
	for (d = 1; d <= full; d++)
	{
		c_ptr = &cave[y][x-d];
		c_ptr->info |= (CAVE_XTRA);
		cave_view_hack(c_ptr, y, x-d);
		if (!cave_floor_grid(c_ptr)) break;
	}

	/* Initialize the "west strips" */
	ws = wn = d;


	/*** Step 4 -- Divide each "octant" into "strips" ***/

	/* Now check each "diagonal" (in parallel) */
	for (n = 1; n <= over / 2; n++)
	{
		int ypn, ymn, xpn, xmn;


		/* Acquire the "bounds" of the maximal circle */
		z = over - n - n;
		if (z > full - n) z = full - n;
		while ((z + n + (n>>1)) > full) z--;


		/* Access the four diagonal grids */
		ypn = y + n;
		ymn = y - n;
		xpn = x + n;
		xmn = x - n;


		/* South strip */
		if (ypn < y_max)
		{
			/* Maximum distance */
			m = MIN(z, y_max - ypn);

			/* East side */
			if ((xpn <= x_max) && (n < se))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ypn+d, xpn, ypn+d-1, xpn-1, ypn+d-1, xpn))
					{
						if (n + d >= se) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				se = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < sw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ypn+d, xmn, ypn+d-1, xmn+1, ypn+d-1, xmn))
					{
						if (n + d >= sw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				sw = k + 1;
			}
		}


		/* North strip */
		if (ymn > 0)
		{
			/* Maximum distance */
			m = MIN(z, ymn);

			/* East side */
			if ((xpn <= x_max) && (n < ne))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ymn-d, xpn, ymn-d+1, xpn-1, ymn-d+1, xpn))
					{
						if (n + d >= ne) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				ne = k + 1;
			}

			/* West side */
			if ((xmn >= 0) && (n < nw))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ymn-d, xmn, ymn-d+1, xmn+1, ymn-d+1, xmn))
					{
						if (n + d >= nw) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				nw = k + 1;
			}
		}


		/* East strip */
		if (xpn < x_max)
		{
			/* Maximum distance */
			m = MIN(z, x_max - xpn);

			/* South side */
			if ((ypn <= x_max) && (n < es))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ypn, xpn+d, ypn-1, xpn+d-1, ypn, xpn+d-1))
					{
						if (n + d >= es) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				es = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < en))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ymn, xpn+d, ymn+1, xpn+d-1, ymn, xpn+d-1))
					{
						if (n + d >= en) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				en = k + 1;
			}
		}


		/* West strip */
		if (xmn > 0)
		{
			/* Maximum distance */
			m = MIN(z, xmn);

			/* South side */
			if ((ypn <= y_max) && (n < ws))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ypn, xmn-d, ypn-1, xmn-d+1, ypn, xmn-d+1))
					{
						if (n + d >= ws) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				ws = k + 1;
			}

			/* North side */
			if ((ymn >= 0) && (n < wn))
			{
				/* Scan */
				for (k = n, d = 1; d <= m; d++)
				{
					/* Check grid "d" in strip "n", notice "blockage" */
					if (update_view_aux(ymn, xmn-d, ymn+1, xmn-d+1, ymn, xmn-d+1))
					{
						if (n + d >= wn) break;
					}

					/* Track most distant "non-blockage" */
					else
					{
						k = n + d;
					}
				}

				/* Limit the next strip */
				wn = k + 1;
			}
		}
	}


	/*** Step 5 -- Complete the algorithm ***/

	/* Update all the new grids */
	for (n = 0; n < view_n; n++)
	{
		y = view_y[n];
		x = view_x[n];

		/* Access the grid */
		c_ptr = &cave[y][x];

		/* Clear the "CAVE_XTRA" flag */
		c_ptr->info &= ~(CAVE_XTRA);

		/* Update only newly viewed grids */
		if (c_ptr->info & (CAVE_TEMP)) continue;

		/* Note */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* Wipe the old grids, update as needed */
	for (n = 0; n < temp_n; n++)
	{
		y = temp_y[n];
		x = temp_x[n];

		/* Access the grid */
		c_ptr = &cave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~(CAVE_TEMP);

		/* Update only non-viewable grids */
		if (c_ptr->info & (CAVE_VIEW)) continue;

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	temp_n = 0;
}



/*
 * Hack -- forget the "flow" information
 */
void forget_flow(void)
{
	int x, y;

	/* Check the entire dungeon */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			/* Forget the old data */
			cave[y][x].dist = 0;
			cave[y][x].cost = 0;
			cave[y][x].when = 0;
		}
	}
}


/*
 * Hack - speed up the update_flow algorithm by only doing
 * it everytime the player moves out of LOS of the last
 * "way-point".
 */
static u16b flow_x = 0;
static u16b flow_y = 0;



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
	int x, y, d;
	int flow_head = 1;
	int flow_tail = 0;

	/* Hack -- disabled */
	if (stupid_monsters) return;

	/* Paranoia -- make sure the array is empty */
	if (temp_n) return;

	/* The last way-point is on the map */
	if (running && in_bounds(flow_y, flow_x))
	{
		/* The way point is in sight - do not update.  (Speedup) */
		if (cave[flow_y][flow_x].info & CAVE_VIEW) return;
	}

	/* Erase all of the current flow information */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave[y][x].cost = 0;
			cave[y][x].dist = 0;
		}
	}

	/* Save player position */
	flow_y = py;
	flow_x = px;

	/* Add the player's grid to the queue */
	temp_y[0] = py;
	temp_x[0] = px;

	/* Now process the queue */
	while (flow_head != flow_tail)
	{
		int ty, tx;

		/* Extract the next entry */
		ty = temp_y[flow_tail];
		tx = temp_x[flow_tail];

		/* Forget that entry */
		if (++flow_tail == TEMP_MAX) flow_tail = 0;

		/* Add the "children" */
		for (d = 0; d < 8; d++)
		{
			int old_head = flow_head;
			int m = cave[ty][tx].cost + 1;
			int n = cave[ty][tx].dist + 1;
			cave_type *c_ptr;

			/* Child location */
			y = ty + ddy_ddd[d];
			x = tx + ddx_ddd[d];

			/* Ignore player's grid */
			if (x == px && y == py) continue;

			c_ptr = &cave[y][x];
				       
			if ((c_ptr->feat >= FEAT_DOOR_HEAD) && (c_ptr->feat <= FEAT_SECRET)) m += 3;

			/* Ignore "pre-stamped" entries */
			if (c_ptr->dist != 0 && c_ptr->dist <= n && c_ptr->cost <= m) continue;

			/* Ignore "walls" and "rubble" */
			if ((c_ptr->feat > FEAT_SECRET) && (c_ptr->feat != FEAT_TREES) && !cave_floor_grid(c_ptr)) continue;

			/* Save the flow cost */
			if (c_ptr->cost == 0 || c_ptr->cost > m) c_ptr->cost = m;
			if (c_ptr->dist == 0 || c_ptr->dist > n) c_ptr->dist = n;

			/* Hack -- limit flow depth */
			if (n == MONSTER_FLOW_DEPTH) continue;

			/* Enqueue that entry */
			temp_y[flow_head] = y;
			temp_x[flow_head] = x;

			/* Advance the queue */
			if (++flow_head == TEMP_MAX) flow_head = 0;

			/* Hack -- notice overflow by forgetting new entry */
			if (flow_head == flow_tail) flow_head = old_head;
		}
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
void update_smell(void)
{
	int i, j;
	int y, x;

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
		for (y = 0; y < cur_hgt; y++)
		{
			for (x = 0; x < cur_wid; x++)
			{
				int w = cave[y][x].when;
				cave[y][x].when = (w > 128) ? (w - 128) : 0;
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
			cave_type *c_ptr;

			/* Translate table to map grids */
			y = i + py - 2;
			x = j + px - 2;

			/* Check Bounds */
			if (!in_bounds(y, x)) continue;

			c_ptr = &cave[y][x];

			/* Walls, water, and lava cannot hold scent. */
			if ((c_ptr->feat > FEAT_SECRET) && (c_ptr->feat != FEAT_TREES) && !cave_floor_grid(c_ptr)) continue;

			/* Grid must not be blocked by walls from the character */
			if (!player_has_los_bold(y, x)) continue;

			/* Note grids that are too far away */
			if (scent_adjust[i][j] == -1) continue;

			/* Mark the grid with new scent */
			c_ptr->when = scent_when + scent_adjust[i][j];
		}
	}
}


/*
 * Hack -- map the current panel (plus some) ala "magic mapping"
 */
void map_area(int range)
{
	int             i, x, y;

	cave_type       *c_ptr;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan that area */
	for (y = 1; y < cur_hgt - 1; y++)
	{
		for (x = 1; x < cur_wid - 1; x++)
		{
			if (distance(py, px, y, x) > range) continue;

			c_ptr = &cave[y][x];

			/* All non-walls are "checked" */
			if ((c_ptr->feat < FEAT_SECRET) ||
			    (c_ptr->feat == FEAT_RUBBLE) ||
			   ((c_ptr->feat >= FEAT_MINOR_GLYPH) &&
			    (c_ptr->feat <= FEAT_TREES)) ||
			    (c_ptr->feat >= FEAT_TOWN))
			{
				/* Memorize normal features */
				if ((c_ptr->feat > FEAT_INVIS) && (c_ptr->feat != FEAT_DIRT) && (c_ptr->feat != FEAT_GRASS))
				{
					/* Memorize the object */
					c_ptr->info |= (CAVE_MARK);
				}

				/* Memorize known walls */
				for (i = 0; i < 8; i++)
				{
					c_ptr = &cave[y + ddy_ddd[i]][x + ddx_ddd[i]];

					/* Memorize walls (etc) */
					if ((c_ptr->feat >= FEAT_SECRET) && (c_ptr->feat != FEAT_DIRT) && (c_ptr->feat != FEAT_GRASS))
					{
						/* Memorize the walls */
						c_ptr->info |= (CAVE_MARK);
					}
				}
			}
		}
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}



/*
 * Light up the dungeon using "clairvoyance"
 *
 * This function "illuminates" every grid in the dungeon, memorizes all
 * "objects", memorizes all grids as with magic mapping, and, under the
 * standard option settings (view_perma_grids but not view_torch_grids)
 * memorizes all floor grids too.
 *
 * Note that if "view_perma_grids" is not set, we do not memorize floor
 * grids, since this would defeat the purpose of "view_perma_grids", not
 * that anyone seems to play without this option.
 *
 * Note that if "view_torch_grids" is set, we do not memorize floor grids,
 * since this would prevent the use of "view_torch_grids" as a method to
 * keep track of what grids have been observed directly.
 */
void wiz_lite(bool wizard, bool ninja)
{
	int i, y, x;

	/* Memorize objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

#if 0
		/* Skip objects in vaults, if not a wizard. -LM- */
		if ((wizard == FALSE) && 
			(cave[o_ptr->iy][o_ptr->ix].info & (CAVE_ICKY))) continue;
#endif

		/* Memorize */
		o_ptr->marked |= OM_FOUND;
	}

	/* Scan all normal grids */
	for (y = 1; y < cur_hgt - 1; y++)
	{
		/* Scan all normal grids */
		for (x = 1; x < cur_wid - 1; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			/* Process all non-walls */
			if (cave_floor_bold(y, x) || (c_ptr->feat == FEAT_RUBBLE) || (c_ptr->feat == FEAT_TREES) || (c_ptr->feat == FEAT_MOUNTAIN))
			{
				/* Scan all neighbors */
				for (i = 0; i < 9; i++)
				{
					int yy = y + ddy_ddd[i];
					int xx = x + ddx_ddd[i];

					/* Get the grid */
					c_ptr = &cave[yy][xx];

					/* Memorize normal features */
					if (ninja)
					{
						/* Memorize the grid */
						c_ptr->info |= (CAVE_MARK);
					}
					else
					{
						if ((c_ptr->feat > FEAT_INVIS))
						{
							/* Memorize the grid */
							c_ptr->info |= (CAVE_MARK);
						}

						/* Perma-lite the grid */
						if (!(d_info[dungeon_type].flags1 & DF1_DARKNESS))
						{
							c_ptr->info |= (CAVE_GLOW);

							/* Normally, memorize floors (see above) */
							if (view_perma_grids && !view_torch_grids)
							{
								/* Memorize the grid */
								c_ptr->info |= (CAVE_MARK);
							}
						}
					}
				}
			}
		}
	}

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(void)
{
	int i, y, x;


	/* Forget every grid */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			/* Process the grid */
			c_ptr->info &= ~(CAVE_MARK);
		}
	}

	/* Forget all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Forget the object */
		o_ptr->marked = 0;
	}

	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update the view and lite */
	p_ptr->update |= (PU_VIEW | PU_LITE);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}





/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(int y, int x, int feat)
{
	cave_type *c_ptr = &cave[y][x];

	/* Change the feature */
	c_ptr->feat = feat;

	/* Notice */
	note_spot(y, x);

	/* Redraw */
	lite_spot(y, x);
}

/* Remove a mirror */
void remove_mirror(int y, int x)
{
	/* Remove the mirror */
	cave[y][x].info &= ~(CAVE_IN_MIRROR);

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS)
	{
		cave[y][x].info &= ~(CAVE_GLOW);
		if( !view_torch_grids )cave[y][x].info &= ~(CAVE_MARK);
	}
	/* Notice */
	note_spot(y, x);

	/* Redraw */
	lite_spot(y, x);
}

/*
 * Calculate "incremental motion". Used by project() and shoot().
 * Assumes that (*y,*x) lies on the path from (y1,x1) to (y2,x2).
 */
void mmove2(int *y, int *x, int y1, int x1, int y2, int x2)
{
	int dy, dx, dist, shift;

	/* Extract the distance travelled */
	dy = (*y < y1) ? y1 - *y : *y - y1;
	dx = (*x < x1) ? x1 - *x : *x - x1;

	/* Number of steps */
	dist = (dy > dx) ? dy : dx;

	/* We are calculating the next location */
	dist++;


	/* Calculate the total distance along each axis */
	dy = (y2 < y1) ? (y1 - y2) : (y2 - y1);
	dx = (x2 < x1) ? (x1 - x2) : (x2 - x1);

	/* Paranoia -- Hack -- no motion */
	if (!dy && !dx) return;


	/* Move mostly vertically */
	if (dy > dx)
	{
		/* Extract a shift factor */
		shift = (dist * dx + (dy - 1) / 2) / dy;

		/* Sometimes move along the minor axis */
		(*x) = (x2 < x1) ? (x1 - shift) : (x1 + shift);

		/* Always move along major axis */
		(*y) = (y2 < y1) ? (y1 - dist) : (y1 + dist);
	}

	/* Move mostly horizontally */
	else
	{
		/* Extract a shift factor */
		shift = (dist * dy + (dx - 1) / 2) / dx;

		/* Sometimes move along the minor axis */
		(*y) = (y2 < y1) ? (y1 - shift) : (y1 + shift);

		/* Always move along major axis */
		(*x) = (x2 < x1) ? (x1 - dist) : (x1 + dist);
	}
}



/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(int y1, int x1, int y2, int x2)
{
	int y, x;

	int grid_n = 0;
	u16b grid_g[512];

	/* Check the projection path */
	grid_n = project_path(grid_g, (project_length ? project_length : MAX_RANGE), y1, x1, y2, x2, 0);

	/* No grid is ever projectable from itself */
	if (!grid_n) return (FALSE);

	/* Final grid */
	y = GRID_Y(grid_g[grid_n - 1]);
	x = GRID_X(grid_g[grid_n - 1]);

	/* May not end in an unrequested grid */
	if ((y != y2) || (x != x2)) return (FALSE);

	/* Assume okay */
	return (TRUE);
}


/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 */
void scatter(int *yp, int *xp, int y, int x, int d, int m)
{
	int nx, ny;

	/* Unused */
	m = m;

	/* Pick a location */
	while (TRUE)
	{
		/* Pick a new location */
		ny = rand_spread(y, d);
		nx = rand_spread(x, d);

		/* Ignore annoying locations */
		if (!in_bounds(ny, nx)) continue;

		/* Ignore "excessively distant" locations */
		if ((d > 1) && (distance(y, x, ny, nx) > d)) continue;

		/* Require "line of sight" */
		if (los(y, x, ny, nx)) break;
	}

	/* Save the location */
	(*yp) = ny;
	(*xp) = nx;
}




/*
 * Track a new monster
 */
void health_track(int m_idx)
{
	/* Track a new guy */
	p_ptr->health_who = m_idx;

	/* Redraw (later) */
	p_ptr->redraw |= (PR_HEALTH);
}



/*
 * Hack -- track the given monster race
 */
void monster_race_track(int r_idx)
{
	/* Save this monster ID */
	p_ptr->monster_race_idx = r_idx;

	/* Window stuff */
	p_ptr->window |= (PW_MONSTER);
}



/*
 * Hack -- track the given object kind
 */
void object_kind_track(int k_idx)
{
	/* Save this monster ID */
	p_ptr->object_kind_idx = k_idx;

	/* Window stuff */
	p_ptr->window |= (PW_OBJECT);
}



/*
 * Something has happened to disturb the player.
 *
 * The first arg indicates a major disturbance, which affects search.
 *
 * The second arg is currently unused, but could induce output flush.
 *
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(int stop_search, int unused_flag)
{
	/* Unused */
	unused_flag = unused_flag;

	/* Cancel auto-commands */
	/* command_new = 0; */

	/* Cancel repeated commands */
	if (command_rep)
	{
		/* Cancel */
		command_rep = 0;

		/* Redraw the state (later) */
		p_ptr->redraw |= (PR_STATE);
	}

	/* Cancel Resting */
	if ((p_ptr->action == ACTION_REST) || (p_ptr->action == ACTION_FISH) || (stop_search && (p_ptr->action == ACTION_SEARCH)))
	{
		/* Cancel */
		set_action(ACTION_NONE);
	}

	/* Cancel running */
	if (running)
	{
		/* Cancel */
		running = 0;

		/* Check for new panel if appropriate */
		if (center_player && !center_running) verify_panel();

		/* Calculate torch radius */
		p_ptr->update |= (PU_TORCH);

		/* Update monster flow */
		p_ptr->update |= (PU_FLOW);
	}

	/* Flush the input if requested */
	if (flush_disturb) flush();
}
