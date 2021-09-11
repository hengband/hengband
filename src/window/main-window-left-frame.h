#pragma once

struct player_type;
void print_title(player_type *player_ptr);
void print_level(player_type *player_ptr);
void print_exp(player_type *player_ptr);
void print_ac(player_type *player_ptr);
void print_hp(player_type *player_ptr);
void print_sp(player_type *player_ptr);
void print_gold(player_type *player_ptr);
void print_depth(player_type *player_ptr);
void print_frame_basic(player_type *player_ptr);
void health_redraw(player_type *player_ptr, bool riding);
