#pragma once 

#include "system/angband.h"

/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
typedef struct
{
	MONRACE_IDX r_idx;
	bool used;
}
nest_mon_info_type;

bool build_type5(player_type *player_ptr);
bool build_type6(player_type *player_ptr);
bool build_type13(player_type *player_ptr);
