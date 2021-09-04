#pragma once
/*!
 * @file travel-execution.h
 * @brief トラベル移動処理ヘッダ
 */

#include "system/angband.h"
#include "floor/floor-base-definitions.h"

 /*  A structure type for travel command  */
typedef struct travel_type {
    int run; /* Remaining grid number */
    int cost[MAX_HGT][MAX_WID];
    POSITION x; /* Target X */
    POSITION y; /* Target Y */
    DIRECTION dir; /* Running direction */
} travel_type;

extern travel_type travel;

struct floor_type;
struct player_type;
void travel_step(player_type *creature_ptr);
void forget_travel_flow(floor_type *floor_ptr);
