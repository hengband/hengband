#pragma once

/*
 *  A structure type for the saved floor
 */
typedef struct
{
	FLOOR_IDX floor_id;        /* No recycle until 65536 IDs are all used */
	s16b savefile_id;     /* ID for savefile (from 0 to MAX_SAVED_FLOOR) */
	DEPTH dun_level;
	s32b last_visit;      /* Time count of last visit. 0 for new floor. */
	u32b visit_mark;      /* Older has always smaller mark. */
	FLOOR_IDX upper_floor_id;  /* a floor connected with level teleportation */
	FLOOR_IDX lower_floor_id;  /* a floor connected with level tel. and trap door */
} saved_floor_type;

extern void init_saved_floors(bool force);
extern void change_floor(void);
extern void leave_floor(void);
extern void clear_saved_floor_files(void);
extern saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
extern FLOOR_IDX get_new_floor_id(void);
extern void prepare_change_floor_mode(BIT_FLAGS mode);
extern void precalc_cur_num_of_pet(void);
extern FLOOR_IDX max_floor_id;