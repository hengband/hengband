#pragma once

/*
 * Flags for save/load temporal saved floor file
 */
#define SLF_SECOND     	 0x0001  /* Called from another save/load function */
#define SLF_NO_KILL      0x0002  /* Don't kill temporal files */

/* save.c */
extern bool save_player(player_type *player_ptr);
extern bool load_player(void);
extern void remove_loc(void);
extern bool save_floor(saved_floor_type *sf_ptr, BIT_FLAGS mode);
