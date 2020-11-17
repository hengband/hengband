#pragma once 

#include "system/angband.h"

/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
typedef struct nest_mon_info_type {
	MONRACE_IDX r_idx;
	bool used;
} nest_mon_info_type;

typedef struct dun_data_type dun_data_type;
bool build_type5(player_type *player_ptr, dun_data_type *dd_ptr);
bool build_type6(player_type *player_ptr, dun_data_type *dd_ptr);
bool build_type13(player_type *player_ptr, dun_data_type *dd_ptr);
