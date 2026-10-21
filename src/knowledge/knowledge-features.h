#pragma once

#include "system/angband.h"
#include <vector>

class PlayerType;
std::vector<FEAT_IDX> make_feature_index_buffer();
FEAT_IDX collect_features(std::vector<FEAT_IDX> &feat_idx, BIT_FLAGS8 mode);
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level);
void do_cmd_knowledge_dungeon(PlayerType *player_ptr);
