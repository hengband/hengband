#pragma once

#include "system/angband.h"

struct player_type;
bool detect_traps(player_type *player_ptr, POSITION range, bool known);
bool detect_doors(player_type* player_ptr, POSITION range);
bool detect_stairs(player_type* player_ptr, POSITION range);
bool detect_treasure(player_type* player_ptr, POSITION range);
bool detect_objects_gold(player_type* player_ptr, POSITION range);
bool detect_objects_normal(player_type* player_ptr, POSITION range);
bool detect_objects_magic(player_type* player_ptr, POSITION range);
bool detect_monsters_normal(player_type* player_ptr, POSITION range);
bool detect_monsters_invis(player_type* player_ptr, POSITION range);
bool detect_monsters_evil(player_type* player_ptr, POSITION range);
bool detect_monsters_xxx(player_type* player_ptr, POSITION range, uint32_t match_flag);
bool detect_monsters_string(player_type* player_ptr, POSITION range, concptr);
bool detect_monsters_nonliving(player_type* player_ptr, POSITION range);
bool detect_monsters_mind(player_type* player_ptr, POSITION range);
bool detect_all(player_type* player_ptr, POSITION range);
