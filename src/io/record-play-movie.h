#pragma once

#include "system/angband.h"

class PlayerType;
void prepare_movie_hooks(PlayerType *player_ptr);
void prepare_browse_movie_without_path_build(concptr filename);
void browse_movie(void);
#ifndef WINDOWS
void prepare_browse_movie_with_path_build(concptr filename);
#endif
