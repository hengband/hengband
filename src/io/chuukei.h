#pragma once

#include "system/angband.h"

void prepare_movie_hooks(player_type *player_ptr);
void prepare_browse_movie_aux(concptr filename);
void browse_movie(void);
#ifndef WINDOWS
void prepare_browse_movie(concptr filename);
#endif WINDOWS
