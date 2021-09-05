#pragma once

struct player_type;
void print_title(player_type *creature_ptr);
void print_level(player_type *creature_ptr);
void print_exp(player_type *creature_ptr);
void print_ac(player_type *creature_ptr);
void print_hp(player_type *creature_ptr);
void print_sp(player_type *creature_ptr);
void print_gold(player_type *creature_ptr);
void print_depth(player_type *creature_ptr);
void print_frame_basic(player_type *creature_ptr);
void health_redraw(player_type *creature_ptr, bool riding);
