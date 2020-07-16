#pragma once

#include "system/angband.h"
#include "store/store-owners.h"

 /* Store constants */
#define STORE_INVEN_MAX 24              /* Max number of discrete objs in inven */
#define STORE_TURNOVER  9               /* Normal shop turnover, per day */
#define STORE_MIN_KEEP  6               /* Min slots to "always" keep full */
#define STORE_MAX_KEEP  18              /* Max slots to "always" keep full */
#define STORE_SHUFFLE   21              /* 1/Chance (per day) of an owner changing */
#define STORE_TICKS     1000            /* Number of ticks between turnovers */

extern int store_bottom;
extern const owner_type *ot_ptr;

PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip);
bool combine_and_reorder_home(player_type *player_ptr, int store_num);
bool noneedtobargain(PRICE minprice);
void do_cmd_store(player_type *player_ptr);
void store_shuffle(player_type *player_ptr, int which);
void store_maintenance(player_type *player_ptr, int town_num, int store_num);
void store_init(int town_num, int store_num);
