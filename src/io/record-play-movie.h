#pragma once

#include <string_view>

class PlayerType;
void prepare_movie_hooks(PlayerType *player_ptr);
void prepare_browse_movie_without_path_build(std::string_view filename);
void browse_movie();
#ifndef WINDOWS
void prepare_browse_movie_with_path_build(std::string_view filename);
#endif
