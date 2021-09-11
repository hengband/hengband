#pragma once

struct player_type;
struct self_info_type;
void display_life_rating(player_type *player_ptr, self_info_type *self_ptr);
void display_max_base_status(player_type *player_ptr, self_info_type *self_ptr);
void display_virtue(player_type *player_ptr, self_info_type *self_ptr);
void display_mimic_race_ability(player_type *player_ptr, self_info_type *self_ptr);
void display_self_info(self_info_type *self_ptr);
