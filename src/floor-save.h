#pragma once

#define MAX_SAVED_FLOORS       20 /*!< 保存フロアの最大数 / Maximum number of saved floors. */

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

extern u32b saved_floor_file_sign;

extern void init_saved_floors(bool force);
extern void change_floor(BIT_FLAGS floor_mode);
extern void leave_floor(player_type *creature_ptr);
extern void clear_saved_floor_files(void);
extern saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
extern FLOOR_IDX get_new_floor_id(void);

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
extern void prepare_change_floor_mode(BIT_FLAGS mode);

extern void precalc_cur_num_of_pet(void);
extern FLOOR_IDX max_floor_id;
