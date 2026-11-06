#pragma once

class PlayerType;
void do_cmd_save_game(PlayerType *player_ptr, int is_autosave);
void do_cmd_save_and_exit(PlayerType *player_ptr);
void request_auto_save();
void do_requested_auto_save(PlayerType *player_ptr);
