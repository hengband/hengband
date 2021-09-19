#pragma once
/*
 * Windowsのコードからは呼ばれない。よってVSからは見えない
 */

#include "system/angband.h"

struct player_type;
void exit_game_panic(player_type *player_ptr);
