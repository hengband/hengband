#pragma once

#include "system/angband.h"

class PlayerType;
void do_cmd_knowledge_artifacts(PlayerType *player_ptr);
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, KIND_OBJECT_IDX direct_k_idx);
