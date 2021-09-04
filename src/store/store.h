#pragma once

#include "system/angband.h"
#include "store/store-util.h"

 /* Store constants */
#define STORE_INVEN_MAX 24              /* Max number of discrete objs in inven */
#define STORE_TURNOVER  12              /* Normal shop turnover, per day */
#define STORE_MIN_KEEP  6               /* Min slots to "always" keep full */
#define STORE_MAX_KEEP  21              /* Max slots to "always" keep full */
#define STORE_SHUFFLE   21              /* 1/Chance (per day) of an owner changing */
#define STORE_TICKS     1000            /* Number of ticks between turnovers */

typedef struct owner_type owner_type;
extern int store_top;
extern int store_bottom;
extern int xtra_stock;
extern const owner_type *ot_ptr;
extern int16_t old_town_num;
extern int16_t inner_town_num;

extern int cur_store_feat;
extern bool allow_inc;

struct player_type;
int16_t store_get_stock_max(STORE_TYPE_IDX store_idx, bool powerup = true);
void store_shuffle(player_type *player_ptr, int which);
void store_maintenance(player_type *player_ptr, int town_num, int store_num, int chance);
void store_init(int town_num, int store_num);
void store_examine(player_type *player_ptr);
int store_check_num(object_type *o_ptr);
int get_stock(COMMAND_CODE *com_val, concptr pmt, int i, int j);
