#include "player/player-status.h"

typedef enum rate_calc_type_mode {
	CALC_RAND = 0,
    CALC_AVERAGE = 1,
    CALC_MIN = 2,
    CALC_MAX = 3,
} rate_calc_type_mode;


PERCENTAGE calc_acid_damage_rate(player_type *creature_ptr);
PERCENTAGE calc_elec_damage_rate(player_type *creature_ptr);
PERCENTAGE calc_fire_damage_rate(player_type *creature_ptr);
PERCENTAGE calc_cold_damage_rate(player_type *creature_ptr);
PERCENTAGE calc_pois_damage_rate(player_type *creature_ptr);
PERCENTAGE calc_lite_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode);
PERCENTAGE calc_dark_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode);
