#pragma once

#include <string_view>

class PlayerType;
bool activate_resistance_elements(PlayerType *player_ptr);
bool activate_resistance_acid(PlayerType *player_ptr, std::string_view name);
bool activate_resistance_elec(PlayerType *player_ptr, std::string_view name);
bool activate_resistance_fire(PlayerType *player_ptr, std::string_view name);
bool activate_resistance_cold(PlayerType *player_ptr, std::string_view name);
bool activate_resistance_pois(PlayerType *player_ptr, std::string_view name);
bool activate_acid_ball_and_resistance(PlayerType *player_ptr, std::string_view name);
bool activate_elec_ball_and_resistance(PlayerType *player_ptr, std::string_view name);
bool activate_fire_ball_and_resistance(PlayerType *player_ptr, std::string_view name);
bool activate_cold_ball_and_resistance(PlayerType *player_ptr, std::string_view name);
bool activate_pois_ball_and_resistance(PlayerType *player_ptr, std::string_view name);
bool activate_ultimate_resistance(PlayerType *player_ptr);
