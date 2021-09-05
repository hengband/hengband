#pragma once

struct player_type;
void do_cmd_inven(player_type *creature_ptr);
void do_cmd_drop(player_type *creature_ptr);
void do_cmd_observe(player_type *creature_ptr);
void do_cmd_uninscribe(player_type *creature_ptr);
void do_cmd_inscribe(player_type *creature_ptr);
void do_cmd_use(player_type *creature_ptr);
void do_cmd_activate(player_type *user_ptr);
