#pragma once

struct player_type;
void do_cmd_go_up(player_type *player_ptr);
void do_cmd_go_down(player_type *player_ptr);
void do_cmd_walk(player_type *player_ptr, bool pickup);
void do_cmd_run(player_type *player_ptr);
void do_cmd_stay(player_type *player_ptr, bool pickup);
void do_cmd_rest(player_type *player_ptr);
