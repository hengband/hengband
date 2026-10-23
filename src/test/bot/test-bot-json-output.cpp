/*!
 * @brief 制御サーバ・JSONL の出力のテスト
 */

#include "bot/bot-json-output.h"

#include <doctest/doctest.h>

TEST_CASE("find_bot_knowledge_category maps knowledge menu keys to categories")
{
    CHECK(find_bot_knowledge_category('1') == BotKnowledgeCategory::ARTIFACTS_KNOWN);
    CHECK(find_bot_knowledge_category('2') == BotKnowledgeCategory::ARTIFACTS_IDENTIFIED);
    CHECK(find_bot_knowledge_category('3') == BotKnowledgeCategory::OBJECTS_KNOWN);
    CHECK(find_bot_knowledge_category('4') == BotKnowledgeCategory::UNIQUES_ALIVE);
    CHECK(find_bot_knowledge_category('5') == BotKnowledgeCategory::UNIQUES_DEAD);
    CHECK(find_bot_knowledge_category('6') == BotKnowledgeCategory::MONSTERS);
    CHECK(find_bot_knowledge_category('7') == BotKnowledgeCategory::KILL_COUNT);
    CHECK(find_bot_knowledge_category('8') == BotKnowledgeCategory::BOUNTY);
    CHECK(find_bot_knowledge_category('9') == BotKnowledgeCategory::HOME);
    CHECK(find_bot_knowledge_category('0') == BotKnowledgeCategory::EQUIP_RESISTANCES);
    CHECK(find_bot_knowledge_category('a') == BotKnowledgeCategory::FEATURES);
    CHECK(find_bot_knowledge_category('b') == BotKnowledgeCategory::SELF_INFO);
    CHECK(find_bot_knowledge_category('c') == BotKnowledgeCategory::MUTATIONS);
    CHECK(find_bot_knowledge_category('d') == BotKnowledgeCategory::WEAPON_EXP);
    CHECK(find_bot_knowledge_category('e') == BotKnowledgeCategory::SPELL_EXP);
    CHECK(find_bot_knowledge_category('f') == BotKnowledgeCategory::SKILL_EXP);
    CHECK(find_bot_knowledge_category('g') == BotKnowledgeCategory::VIRTUES);
    CHECK(find_bot_knowledge_category('h') == BotKnowledgeCategory::DUNGEONS);
    CHECK(find_bot_knowledge_category('i') == BotKnowledgeCategory::QUESTS);
    CHECK(find_bot_knowledge_category('j') == BotKnowledgeCategory::PETS);
    CHECK(find_bot_knowledge_category('k') == BotKnowledgeCategory::AUTOPICK);
}

TEST_CASE("find_bot_knowledge_category returns nullopt for keys without a list")
{
    CHECK_FALSE(find_bot_knowledge_category(' ').has_value());
    CHECK_FALSE(find_bot_knowledge_category('l').has_value());
    CHECK_FALSE(find_bot_knowledge_category('A').has_value());
}
