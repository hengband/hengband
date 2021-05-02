#pragma once

enum target_type : unsigned int;
typedef struct player_type player_type;
bool target_set(player_type *creature_ptr, target_type mode);
