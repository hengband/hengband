#pragma once

extern void disturb(bool stop_search, bool flush_output);
extern void move_player(DIRECTION dir, bool do_pickup, bool break_trap);
extern void run_step(DIRECTION dir);
extern bool move_player_effect(POSITION ny, POSITION nx, BIT_FLAGS mpe_mode);
extern void py_pickup_aux(OBJECT_IDX o_idx);
extern bool pattern_seq(POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x);
extern bool trap_can_be_ignored(FEAT_IDX feat);
extern void search(void);
extern void carry(bool pickup);
#ifdef TRAVEL
extern void travel_step(void);
#endif

/* Types of pattern tiles */
#define NOT_PATTERN_TILE      -1
#define PATTERN_TILE_START    0
#define PATTERN_TILE_1        1
#define PATTERN_TILE_2        2
#define PATTERN_TILE_3        3
#define PATTERN_TILE_4        4
#define PATTERN_TILE_END      5
#define PATTERN_TILE_OLD      6
#define PATTERN_TILE_TELEPORT 7
#define PATTERN_TILE_WRECKED  8

/*
 * Bit flags for move_player_effect()
 */
#define MPE_STAYING       0x00000001
#define MPE_FORGET_FLOW   0x00000002
#define MPE_HANDLE_STUFF  0x00000004
#define MPE_ENERGY_USE    0x00000008
#define MPE_DONT_PICKUP   0x00000010
#define MPE_DO_PICKUP     0x00000020
#define MPE_BREAK_TRAP    0x00000040
#define MPE_DONT_SWAP_MON 0x00000080



#ifdef TRAVEL
 /*
  *  A structure type for travel command
  */
typedef struct {
	int run; /* Remaining grid number */
	int cost[MAX_HGT][MAX_WID];
	POSITION x; /* Target X */
	POSITION y; /* Target Y */
	DIRECTION dir; /* Running direction */
} travel_type;

/* for travel */
extern travel_type travel;
#endif

