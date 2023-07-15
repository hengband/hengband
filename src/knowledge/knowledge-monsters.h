#pragma once

#include "system/angband.h"
#include <optional>

enum class MonsterRaceId : int16_t;
class PlayerType;
void do_cmd_knowledge_monsters(PlayerType *player_ptr, bool *need_redraw, bool visual_only, std::optional<MonsterRaceId> direct_r_idx = std::nullopt);
void do_cmd_knowledge_pets(PlayerType *player_ptr);
void do_cmd_knowledge_kill_count(PlayerType *player_ptr);
void do_cmd_knowledge_bounty(PlayerType *player_ptr);
