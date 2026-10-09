#pragma once

#include <string>
#include <tl/optional.hpp>

class PlayerType;
void do_cmd_reload_autopick(PlayerType *player_ptr);
tl::optional<std::string> do_cmd_knowledge_autopick(PlayerType *player_ptr);
