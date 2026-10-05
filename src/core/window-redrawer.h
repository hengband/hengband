#pragma once

#include "system/angband.h"

class PlayerType;
void redraw_window();
void window_stuff(PlayerType *player_ptr);
void window_stuff_including_deferred(PlayerType *player_ptr);
void redraw_stuff(PlayerType *player_ptr);
