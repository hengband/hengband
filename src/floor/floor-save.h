#pragma once

#include "system/angband.h"

#define MAX_SAVED_FLOORS 20 /*!< 保存フロアの最大数 / Maximum number of saved floors. */

/*
 *  A structure type for the saved floor
 */
typedef struct saved_floor_type {
	FLOOR_IDX floor_id;        /* No recycle until 65536 IDs are all used */
	s16b savefile_id;     /* ID for savefile (from 0 to MAX_SAVED_FLOOR) */
	DEPTH dun_level;
	s32b last_visit;      /* Time count of last visit. 0 for new floor. */
	u32b visit_mark;      /* Older has always smaller mark. */
	FLOOR_IDX upper_floor_id;  /* a floor connected with level teleportation */
	FLOOR_IDX lower_floor_id;  /* a floor connected with level tel. and trap door */
} saved_floor_type;

extern u32b saved_floor_file_sign;
extern bool repair_monsters;
extern saved_floor_type saved_floors[MAX_SAVED_FLOORS];

void init_saved_floors(player_type *creature_ptr, bool force);
void change_floor(player_type *creature_ptr);
void leave_floor(player_type *creature_ptr);
void clear_saved_floor_files(player_type *creature_ptr);
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
FLOOR_IDX get_new_floor_id(player_type *creature_ptr);
void precalc_cur_num_of_pet(player_type *creature_ptr);
FLOOR_IDX max_floor_id;
