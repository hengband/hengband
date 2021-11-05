#pragma once

#include "system/angband.h"

class PlayerType;
void rd_dungeons(PlayerType *player_ptr);
void rd_alter_reality(PlayerType *player_ptr);
void set_gambling_monsters(void);
void rd_autopick(PlayerType *player_ptr);
void rd_visited_towns(PlayerType *player_ptr);
void rd_global_configurations(PlayerType *player_ptr);
void load_wilderness_info(PlayerType *player_ptr);
errr analyze_wilderness(void);
