#pragma once

#include "system/angband.h"

class PlayerType;
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level);
void do_cmd_knowledge_dungeon(PlayerType *player_ptr);
