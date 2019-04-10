
/*
 * Flags for change floor mode
 */
#define CFM_UP        	 0x0001  /* Move up */
#define CFM_DOWN      	 0x0002  /* Move down */
#define CFM_LONG_STAIRS  0x0004  /* Randomly occurred long stairs/shaft */
#define CFM_XXX  	 0x0008  /* XXX */
#define CFM_SHAFT     	 0x0010  /* Shaft */
#define CFM_RAND_PLACE   0x0020  /* Arrive at random grid */
#define CFM_RAND_CONNECT 0x0040  /* Connect with random stairs */
#define CFM_SAVE_FLOORS  0x0080  /* Save floors */
#define CFM_NO_RETURN    0x0100  /* Flee from random quest etc... */
#define CFM_FIRST_FLOOR  0x0200  /* Create exit from the dungeon */

/*
 * Determines if a map location is fully inside the outer walls
 */
#define in_bounds(Y,X) \
   (((Y) > 0) && ((X) > 0) && ((Y) < current_floor_ptr->height-1) && ((X) < current_floor_ptr->width-1))

/*
 * Determines if a map location is on or inside the outer walls
 */
#define in_bounds2(Y,X) \
   (((Y) >= 0) && ((X) >= 0) && ((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))

/*
 * Determines if a map location is on or inside the outer walls
 * (unsigned version)
 */
#define in_bounds2u(Y,X) \
   (((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))


/*
 * Determine if player is on this grid
 */
#define player_bold(Y,X) \
	(((Y) == p_ptr->y) && ((X) == p_ptr->x))

/*
 * Grid based version of "player_bold()"
 */
#define player_grid(C) \
	((C) == &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x])


#define cave_have_flag_bold(Y,X,INDEX) \
	(have_flag(f_info[current_floor_ptr->grid_array[(Y)][(X)].feat].flags, (INDEX)))


#define cave_have_flag_grid(C,INDEX) \
	(have_flag(f_info[(C)->feat].flags, (INDEX)))


/*
 * Determine if a "feature" supports "los"
 */
#define feat_supports_los(F) \
	(have_flag(f_info[(F)].flags, FF_LOS))


/*
 * Determine if a "legal" grid supports "los"
 */
#define cave_los_bold(Y,X) \
	(feat_supports_los(current_floor_ptr->grid_array[(Y)][(X)].feat))

#define cave_los_grid(C) \
	(feat_supports_los((C)->feat))


/*
 * Determine if a "legal" grid is a "clean" floor grid
 * Determine if terrain-change spells are allowed in a grid.
 *
 * Line 1 -- forbid non-floors
 * Line 2 -- forbid object terrains
 * Line 3 -- forbid normal objects
 */
#define cave_clean_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_FLOOR) && \
	 !(current_floor_ptr->grid_array[Y][X].info & CAVE_OBJECT) && \
	  (current_floor_ptr->grid_array[Y][X].o_idx == 0))


/*
 * Determine if an object can be dropped on a "legal" grid
 *
 * Line 1 -- forbid non-drops
 * Line 2 -- forbid object terrains
 */
#define cave_drop_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_DROP) && \
	 !(current_floor_ptr->grid_array[Y][X].info & CAVE_OBJECT))


/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monsters are allowed to move into a grid
 *
 * Line 1 -- forbid non-placement grids
 * Line 2 -- forbid normal monsters
 * Line 3 -- forbid the player
 */
#define cave_empty_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PLACE) && \
	 !(current_floor_ptr->grid_array[Y][X].m_idx) && \
	 !player_bold(Y,X))


/*
 * Determine if a "legal" grid is an "empty" floor grid
 * Determine if monster generation is allowed in a grid
 *
 * Line 1 -- forbid non-empty grids
 * Line 2 -- forbid trees while dungeon generation
 */
#define cave_empty_bold2(Y,X) \
	(cave_empty_bold(Y,X) && \
	 (character_dungeon || !cave_have_flag_bold((Y), (X), FF_TREE)))


/*
 * Determine if a "legal" grid is an "naked" floor grid
 *
 * Line 1 -- forbid non-clean gird
 * Line 2 -- forbid monsters
 * Line 3 -- forbid the player
 */
#define cave_naked_bold(Y,X) \
	(cave_clean_bold(Y,X) && \
	 !(current_floor_ptr->grid_array[Y][X].m_idx) && \
	 !player_bold(Y,X))


/*
 * Determine if a "legal" grid is "permanent"
 *
 * Line 1 -- permanent flag
 */
#define cave_perma_bold(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PERMANENT))


/*
 * Grid based version of "cave_empty_bold()"
 */
#define cave_empty_grid(C) \
	(cave_have_flag_grid((C), FF_PLACE) && \
	 !((C)->m_idx) && \
	 !player_grid(C))


/*
 * Grid based version of "cave_perma_bold()"
 */
#define cave_perma_grid(C) \
	(cave_have_flag_grid((C), FF_PERMANENT))


#define pattern_tile(Y,X) \
	(cave_have_flag_bold((Y), (X), FF_PATTERN))

/*
 * Does the grid stop disintegration?
 */
#define cave_stop_disintegration(Y,X) \
	(!cave_have_flag_bold((Y), (X), FF_PROJECT) && \
	 (!cave_have_flag_bold((Y), (X), FF_HURT_DISI) || \
	  cave_have_flag_bold((Y), (X), FF_PERMANENT)))


/*
 * Determine if a "legal" grid is within "los" of the player
 *
 * Note the use of comparison to zero to force a "boolean" result
 */
#define player_has_los_grid(C) \
    (((C)->info & (CAVE_VIEW)) != 0)

/*
 * Determine if a "legal" grid is within "los" of the player
 *
 * Note the use of comparison to zero to force a "boolean" result
 */
#define player_has_los_bold(Y,X) \
    (((current_floor_ptr->grid_array[Y][X].info & (CAVE_VIEW)) != 0) || p_ptr->inside_battle)


/*
 * Determine if a "feature" is "permanent wall"
 */
#define permanent_wall(F) \
	(have_flag((F)->flags, FF_WALL) && \
	 have_flag((F)->flags, FF_PERMANENT))

/*
 * Get feature mimic from f_info[] (applying "mimic" field)
 */
#define get_feat_mimic(C) \
	(f_info[(C)->mimic ? (C)->mimic : (C)->feat].mimic)
