#pragma once

#include <string>
#include <tl/expected.hpp>

class PlayerType;
tl::expected<void, std::string> load_savedata(PlayerType *player_ptr, bool *new_game);
