#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"
#include <vector>

class PlayerType;
enum class QuestId : int16_t;
void do_cmd_checkquest(PlayerType *player_ptr);
void do_cmd_knowledge_quests_completed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_numbers);
void do_cmd_knowledge_quests_failed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_numbers);
void do_cmd_knowledge_quests(PlayerType *player_ptr);
