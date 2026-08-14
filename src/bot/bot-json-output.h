#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

class PlayerType;
enum class StoreSaleType;

enum class BotKnowledgeCategory {
    ARTIFACTS_KNOWN,
    ARTIFACTS_IDENTIFIED,
    OBJECTS_KNOWN,
    UNIQUES_ALIVE,
    UNIQUES_DEAD,
    BOUNTY,
    HOME,
    EQUIP_RESISTANCES,
    FEATURES,
    SELF_INFO,
    MUTATIONS,
    WEAPON_EXP,
    SPELL_EXP,
    SKILL_EXP,
    VIRTUES,
    DUNGEONS,
    QUESTS,
    PETS,
    AUTOPICK,
    MAX,
};

std::string to_json_utf8(std::string_view str);
nlohmann::json make_message_history_json(int count);
nlohmann::json make_bot_json_snapshot(PlayerType *player_ptr, bool include_map = true);
void output_bot_json_snapshot(PlayerType *player_ptr);
void output_bot_json_store_snapshot(PlayerType *player_ptr, StoreSaleType store_num);
void output_bot_json_character_snapshot(PlayerType *player_ptr);
void output_bot_json_knowledge_snapshot(PlayerType *player_ptr, BotKnowledgeCategory category);
void output_bot_json_look_snapshot(PlayerType *player_ptr);
