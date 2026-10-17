#pragma once

#include <string>
#include <tl/optional.hpp>

class PlayerType;
tl::optional<std::string> do_cmd_knowledge_uniques(PlayerType *player_ptr, bool is_alive);
