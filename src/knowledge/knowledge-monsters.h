#pragma once

#include <string_view>
#include <tl/optional.hpp>

enum class MonraceId : short;
class PlayerType;
void do_cmd_knowledge_monsters(PlayerType *player_ptr, bool *need_redraw, bool visual_only, tl::optional<MonraceId> direct_r_idx = tl::nullopt);
void do_cmd_knowledge_pets(PlayerType *player_ptr);
void do_cmd_knowledge_kill_count(PlayerType *player_ptr);
void do_cmd_knowledge_bounty(std::string_view player_name);
