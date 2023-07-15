#pragma once
/*
 * Windowsのコードからは呼ばれない。よってVSからは見えない
 */

#include "system/angband.h"

class PlayerType;
void exit_game_panic(PlayerType *player_ptr);
