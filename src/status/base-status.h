#pragma once

class PlayerType;
bool inc_stat(PlayerType *player_ptr, int stat);
bool dec_stat(PlayerType *player_ptr, int stat, int amount, int permanent);
bool res_stat(PlayerType *player_ptr, int stat);
bool do_dec_stat(PlayerType *player_ptr, int stat);
bool do_res_stat(PlayerType *player_ptr, int stat);
bool do_inc_stat(PlayerType *player_ptr, int stat);
bool lose_all_info(PlayerType *player_ptr);
