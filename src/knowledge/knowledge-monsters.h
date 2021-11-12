#pragma once

#include "system/angband.h"

class PlayerType;
void do_cmd_knowledge_monsters(PlayerType *player_ptr, bool *need_redraw, bool visual_only, IDX direct_r_idx);
void do_cmd_knowledge_pets(PlayerType *player_ptr);
void do_cmd_knowledge_kill_count(PlayerType *player_ptr);
void do_cmd_knowledge_bounty(PlayerType *player_ptr);
