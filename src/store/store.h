#pragma once

#include "store/store-util.h"
#include "system/angband.h"
#include <optional>
#include <string_view>

/* Store constants */
#define STORE_INVEN_MAX 24 /* Max number of discrete objs in inven */
#define STORE_TURNOVER 12 /* Normal shop turnover, per day */
#define STORE_MIN_KEEP 6 /* Min slots to "always" keep full */
#define STORE_MAX_KEEP 21 /* Max slots to "always" keep full */
#define STORE_SHUFFLE 21 /* 1/Chance (per day) of an owner changing */
#define STORE_TICKS 1000 /* Number of ticks between turnovers */

struct owner_type;
extern int store_top;
extern int store_bottom;
extern int xtra_stock;
extern const owner_type *ot_ptr;
extern int16_t old_town_num;
extern int16_t inner_town_num;

extern int cur_store_feat;
extern bool allow_inc;

class PlayerType;
int16_t store_get_stock_max(StoreSaleType sst, bool powerup = true);
void store_shuffle(PlayerType *player_ptr, StoreSaleType which);
void store_maintenance(PlayerType *player_ptr, int town_num, StoreSaleType store_num, int chance);
void store_init(int town_num, StoreSaleType store_num);
void store_examine(PlayerType *player_ptr, StoreSaleType store_num);
int store_check_num(ItemEntity *o_ptr, StoreSaleType store_num);
std::optional<short> input_stock(std::string_view fmt, int min, int max, StoreSaleType store_num);
