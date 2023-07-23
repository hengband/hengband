#pragma once

#include "system/angband.h"

class PlayerType;
bool detect_traps(PlayerType *player_ptr, POSITION range, bool known);
bool detect_doors(PlayerType *player_ptr, POSITION range);
bool detect_stairs(PlayerType *player_ptr, POSITION range);
bool detect_treasure(PlayerType *player_ptr, POSITION range);
bool detect_objects_gold(PlayerType *player_ptr, POSITION range);
bool detect_objects_normal(PlayerType *player_ptr, POSITION range);
bool detect_objects_magic(PlayerType *player_ptr, POSITION range);
bool detect_monsters_normal(PlayerType *player_ptr, POSITION range);
bool detect_monsters_invis(PlayerType *player_ptr, POSITION range);
bool detect_monsters_evil(PlayerType *player_ptr, POSITION range);
bool detect_monsters_xxx(PlayerType *player_ptr, POSITION range, uint32_t match_flag);
bool detect_monsters_string(PlayerType *player_ptr, POSITION range, concptr);
bool detect_monsters_nonliving(PlayerType *player_ptr, POSITION range);
bool detect_monsters_mind(PlayerType *player_ptr, POSITION range);
bool detect_all(PlayerType *player_ptr, POSITION range);
