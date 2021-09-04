#pragma once

#include "system/angband.h"

struct player_type;
void do_cmd_knowledge_monsters(player_type *creature_ptr, bool *need_redraw, bool visual_only, IDX direct_r_idx);
void do_cmd_knowledge_pets(player_type *creature_ptr);
void do_cmd_knowledge_kill_count(player_type *creature_ptr);
void do_cmd_knowledge_bounty(player_type *creature_ptr);
