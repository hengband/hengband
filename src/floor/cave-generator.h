#pragma once

#include <string>
#include <tl/optional.hpp>

class PlayerType;
tl::optional<std::string> cave_gen(PlayerType *player_ptr);
