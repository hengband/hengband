#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <set>

class PlayerType;
enum class QuestId : int16_t;
parse_error_type parse_fixed_map(PlayerType *player_ptr, concptr name, int ymin, int xmin, int ymax, int xmax);
std::set<QuestId> parse_quest_info(const char *file_name);
