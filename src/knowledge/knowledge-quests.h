#pragma once

#include "system/angband.h"

struct player_type;
void do_cmd_checkquest(player_type *player_ptr);
void do_cmd_knowledge_quests_completed(player_type *player_ptr, FILE *fff, QUEST_IDX quest_num[]);
void do_cmd_knowledge_quests_failed(player_type *player_ptr, FILE *fff, QUEST_IDX quest_num[]);
void do_cmd_knowledge_quests(player_type *player_ptr);
