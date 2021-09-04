#pragma once

struct player_type;
bool inc_stat(player_type *creature_ptr, int stat);
bool dec_stat(player_type *creature_ptr, int stat, int amount, int permanent);
bool res_stat(player_type *creature_ptr, int stat);
bool do_dec_stat(player_type *creature_ptr, int stat);
bool do_res_stat(player_type *creature_ptr, int stat);
bool do_inc_stat(player_type *creature_ptr, int stat);
bool lose_all_info(player_type *creature_ptr);
