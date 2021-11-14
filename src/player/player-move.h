#pragma once

#include "system/angband.h"

#define MPE_STAYING       0x00000001
#define MPE_FORGET_FLOW   0x00000002
#define MPE_HANDLE_STUFF  0x00000004
#define MPE_ENERGY_USE    0x00000008
#define MPE_DONT_PICKUP   0x00000010
#define MPE_DO_PICKUP     0x00000020
#define MPE_BREAK_TRAP    0x00000040
#define MPE_DONT_SWAP_MON 0x00000080

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

extern int flow_head;
extern int flow_tail;
extern POSITION temp2_x[MAX_SHORT];
extern POSITION temp2_y[MAX_SHORT];

class PlayerType;
bool move_player_effect(PlayerType *player_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode);
bool trap_can_be_ignored(PlayerType *player_ptr, FEAT_IDX feat);
void search(PlayerType *player_ptr);
