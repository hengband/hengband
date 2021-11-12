#pragma once

extern bool load; /*!<ロード処理中の分岐フラグ*/
extern bool can_save;

class PlayerType;
bool continuous_action_running(PlayerType *player_ptr);
void process_player(PlayerType *player_ptr);
void process_upkeep_with_speed(PlayerType *player_ptr);
