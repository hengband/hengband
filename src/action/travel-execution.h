#pragma once
/*!
 * @file travel-execution.h
 * @brief トラベル移動処理ヘッダ
 */

#include "floor/floor-base-definitions.h"
#include "system/angband.h"

/*  A structure type for travel command  */
struct travel_type {
    int run; /* Remaining grid number */
    int cost[MAX_HGT][MAX_WID];
    POSITION x; /* Target X */
    POSITION y; /* Target Y */
    DIRECTION dir; /* Running direction */
};

extern travel_type travel;

class FloorType;
class PlayerType;
void travel_step(PlayerType *player_ptr);
void forget_travel_flow(FloorType *floor_ptr);
