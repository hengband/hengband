#pragma once

#include "system/angband.h"

// Change Floor Mode.
enum cfm_type {
	CFM_UP = 0x0001, /* Move up */
	CFM_DOWN = 0x0002, /* Move down */
	CFM_LONG_STAIRS = 0x0004, /* Randomly occurred long stairs/shaft */
	CFM_XXX = 0x0008, /* XXX */
	CFM_SHAFT = 0x0010, /* Shaft */
	CFM_RAND_PLACE = 0x0020, /* Arrive at random grid */
	CFM_RAND_CONNECT = 0x0040, /* Connect with random stairs */
	CFM_SAVE_FLOORS = 0x0080, /* Save floors */
	CFM_NO_RETURN = 0x0100, /* Flee from random quest etc... */
	CFM_FIRST_FLOOR = 0x0200, /* Create exit from the dungeon */
};

typedef struct player_type player_type;
void prepare_change_floor_mode(player_type *creature_ptr, BIT_FLAGS mode);
