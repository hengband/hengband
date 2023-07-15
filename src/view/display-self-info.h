#pragma once

class PlayerType;
struct self_info_type;
void display_life_rating(PlayerType *player_ptr, self_info_type *self_ptr);
void display_max_base_status(PlayerType *player_ptr, self_info_type *self_ptr);
void display_virtue(PlayerType *player_ptr, self_info_type *self_ptr);
void display_mimic_race_ability(PlayerType *player_ptr, self_info_type *self_ptr);
void display_self_info(self_info_type *self_ptr);
