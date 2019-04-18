#pragma once

extern void init_saved_floors(bool force);
extern void change_floor(void);
extern void leave_floor(void);
extern void clear_saved_floor_files(void);
extern saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
extern FLOOR_IDX get_new_floor_id(void);
extern void prepare_change_floor_mode(BIT_FLAGS mode);
extern void precalc_cur_num_of_pet(void);
