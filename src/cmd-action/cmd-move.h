#pragma once

class PlayerType;
void do_cmd_go_up(PlayerType *player_ptr);
void do_cmd_go_down(PlayerType *player_ptr);
void do_cmd_walk(PlayerType *player_ptr, bool pickup);
void do_cmd_run(PlayerType *player_ptr);
void do_cmd_stay(PlayerType *player_ptr, bool pickup);
void do_cmd_rest(PlayerType *player_ptr);
