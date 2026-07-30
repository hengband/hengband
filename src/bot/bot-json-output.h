#pragma once

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

void output_bot_json_snapshot(PlayerType *player_ptr);
void output_bot_json_store_snapshot(PlayerType *player_ptr, StoreSaleType store_num);
void output_bot_json_character_snapshot(PlayerType *player_ptr);
void output_bot_json_knowledge_snapshot(PlayerType *player_ptr, BotKnowledgeCategory category);
void output_bot_json_look_snapshot(PlayerType *player_ptr);
