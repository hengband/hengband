#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"

class PlayerType;
void do_cmd_checkquest(PlayerType *player_ptr);
void do_cmd_knowledge_quests_completed(PlayerType *player_ptr, FILE *fff, int16_t quest_num[]);
void do_cmd_knowledge_quests_failed(PlayerType *player_ptr, FILE *fff, int16_t quest_num[]);
void do_cmd_knowledge_quests(PlayerType *player_ptr);
