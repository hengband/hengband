#pragma once

#include <string>
#include <vector>

enum class QuestId : short;
extern std::vector<std::string> quest_text_lines;
extern QuestId leaving_quest;

constexpr auto QUEST_TEST_LINES_MAX = 10;

enum class QuestStatusType : short;
class FloorType;
class ItemEntity;
class PlayerType;
class QuestType;
bool populate_quest_text_lines(QuestId quest_id);
void assign_json_quest_metadata(QuestId quest_id);
void determine_random_questor(PlayerType *player_ptr, QuestType &quest);
void record_quest_final_status(QuestType *q_ptr, short lev, QuestStatusType stat);
void complete_quest(PlayerType *player_ptr, QuestId quest_num);
void check_find_art_quest_completion(PlayerType *player_ptr, ItemEntity *o_ptr);
void quest_discovery(QuestId quest_id);
void leave_quest_check(PlayerType *player_ptr);
void leave_tower_check(PlayerType *player_ptr);
void exe_enter_quest(PlayerType *player_ptr, QuestId quest_id);
void do_cmd_quest(PlayerType *player_ptr);
bool inside_quest(QuestId quest_id);
