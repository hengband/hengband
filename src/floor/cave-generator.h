#pragma once

#include <optional>
#include <string>

class PlayerType;
std::optional<std::string> cave_gen(PlayerType *player_ptr);
