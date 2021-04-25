#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void rd_dungeons(player_type *creature_ptr);
void rd_alter_reality(player_type *creature_ptr);
void set_gambling_monsters(void);
void rd_autopick(player_type *creature_ptr);
void rd_visited_towns(player_type *creature_ptr);
void rd_global_configurations(player_type *creature_ptr);
void load_wilderness_info(player_type *creature_ptr);
errr analyze_wilderness(void);
