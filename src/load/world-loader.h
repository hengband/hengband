#pragma once

#include "system/angband.h"

struct player_type;
void rd_dungeons(player_type *player_ptr);
void rd_alter_reality(player_type *player_ptr);
void set_gambling_monsters(void);
void rd_autopick(player_type *player_ptr);
void rd_visited_towns(player_type *player_ptr);
void rd_global_configurations(player_type *player_ptr);
void load_wilderness_info(player_type *player_ptr);
errr analyze_wilderness(void);
