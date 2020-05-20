#pragma once

#include "system/angband.h"
#include "store/store-owners.h"

 /*
  * Store constants
  */
#define STORE_INVEN_MAX 24              /* Max number of discrete objs in inven */
#define STORE_TURNOVER  9               /* Normal shop turnover, per day */
#define STORE_MIN_KEEP  6               /* Min slots to "always" keep full */
#define STORE_MAX_KEEP  18              /* Max slots to "always" keep full */
#define STORE_SHUFFLE   21              /* 1/Chance (per day) of an owner changing */
#define STORE_TICKS     1000            /* Number of ticks between turnovers */

/* store.c */
extern bool combine_and_reorder_home(int store_num);
extern void do_cmd_store(player_type *player_ptr);
extern void store_shuffle(player_type *player_ptr, int which);
extern void store_maint(player_type *player_ptr, int town_num, int store_num);
extern void store_init(int town_num, int store_num);
