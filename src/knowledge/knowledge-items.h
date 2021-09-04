#pragma once

#include "system/angband.h"

struct player_type;
void do_cmd_knowledge_artifacts(player_type *player_ptr);
void do_cmd_knowledge_objects(player_type *creature_ptr, bool *need_redraw, bool visual_only, KIND_OBJECT_IDX direct_k_idx);
