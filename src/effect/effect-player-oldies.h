#pragma once

struct effect_player_type;
struct player_type;
void effect_player_old_heal(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_old_speed(player_type *target_ptr, effect_player_type *ep_ptr);
void effect_player_old_slow(player_type *target_ptr);
void effect_player_old_sleep(player_type *target_ptr, effect_player_type *ep_ptr);
