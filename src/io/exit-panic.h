#pragma once
/*
 * Windowsのコードからは呼ばれない。よってVSからは見えない
 */

#include "system/angband.h"

struct PlayerType;
void exit_game_panic(PlayerType *player_ptr);
