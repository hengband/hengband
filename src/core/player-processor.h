#pragma once

#include "system/angband.h"

extern bool load; /*!<ロード処理中の分岐フラグ*/
extern bool can_save;

bool continuous_action_running(player_type *creature_ptr);
void process_player(player_type *creature_ptr);
void process_upkeep_with_speed(player_type *creature_ptr);
