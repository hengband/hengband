#pragma once

#include "system/angband.h"

struct player_type;
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level);
void do_cmd_knowledge_dungeon(player_type *player_ptr);
