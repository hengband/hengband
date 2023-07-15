#pragma once

#include <filesystem>
#include <string_view>

class PlayerType;
void prepare_movie_hooks(PlayerType *player_ptr);
void prepare_browse_movie_without_path_build(const std::filesystem::path &path);
void browse_movie();
#ifndef WINDOWS
void prepare_browse_movie_with_path_build(std::string_view filename);
#endif
