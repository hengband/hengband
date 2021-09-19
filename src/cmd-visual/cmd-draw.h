#pragma once

struct player_type;
void do_cmd_redraw(player_type *player_ptr);
void do_cmd_player_status(player_type *player_ptr);
void do_cmd_message_one(void);
void do_cmd_messages(int num_now);
