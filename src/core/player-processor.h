#pragma once

extern bool load; /*!<ロード処理中の分岐フラグ*/
extern bool can_save;

struct player_type;
bool continuous_action_running(player_type *creature_ptr);
void process_player(player_type *creature_ptr);
void process_upkeep_with_speed(player_type *creature_ptr);
