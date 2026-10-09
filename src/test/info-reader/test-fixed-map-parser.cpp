#include "info-reader/town-map-condition.h"
#include "system/angband-system.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/player-type-definition.h"
#include "test/system/quest-list-test-access.h"
#include "util/finalizer.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <string>
#include <string_view>

namespace {
void check_condition(PlayerType &player, const std::string &expression, bool expected)
{
    CAPTURE(expression);
    const auto result = is_town_map_condition_met(&player, expression);
    REQUIRE(result);
    CHECK(*result == expected);
}

void check_invalid(PlayerType &player, const std::string &expression)
{
    CAPTURE(expression);
    const auto result = is_town_map_condition_met(&player, expression);
    REQUIRE_FALSE(result);
    CHECK(result.error().cause == ConditionExpressionError::INVALID_VARIABLE);
    CHECK_FALSE(result.error().variable.empty());
}
}

TEST_CASE("TownMapCondition preserves absent conditions and ordinary evaluation")
{
    PlayerType player;
    const auto absent = is_town_map_condition_met(&player, std::nullopt);
    REQUIRE(absent);
    CHECK(*absent);
    player.lev = 17;
    check_condition(player, "[EQU $LEVEL 17]", true);
    check_condition(player, "[EQU $LEVEL 18]", false);
    check_condition(player, "[EQU $VARIANT ZANGBAND]", true);
    for (const auto *expression : { "0", "$UNKNOWN", "[EQU $UNKNOWN 1]", "[LEQ bad 1]", "[EQU a a", "[UNKNOWN 1]" }) {
        check_condition(player, expression, false);
    }
}

TEST_CASE("TownMapCondition rejects malformed numeric variable suffixes")
{
    PlayerType player;
    test::QuestListTestAccess quests;
    QuestType quest;
    quests.seed(QuestId::THIEF, quest, QuestFixedMap{});
    for (const auto *prefix : { "QUEST", "QUEST_TYPE", "RANDOM" }) {
        for (const auto *suffix : { "", "x", "1x", "1.0", "0x1", "+", "+-1", "++1", "2147483648", "-2147483649" }) {
            check_invalid(player, "[EQU $" + std::string(prefix) + suffix + " 0]");
        }
    }
}

TEST_CASE("TownMapCondition rejects zero and negative random divisors")
{
    PlayerType player;
    for (const auto *suffix : { "0", "00", "+0", "-0", "-1", "-2147483648" }) {
        check_invalid(player, "[EQU $RANDOM" + std::string(suffix) + " 0]");
    }
}

TEST_CASE("TownMapCondition preserves random remainder and decimal spelling")
{
    PlayerType player;
    for (const auto seed : { uint32_t{ 0 }, uint32_t{ 42 }, uint32_t{ 0x80000000 }, std::numeric_limits<uint32_t>::max() }) {
        auto &system = AngbandSystem::get_instance();
        const auto restore_seed = util::make_finalizer([old_seed = system.get_seed_town()] { AngbandSystem::get_instance().set_seed_town(old_seed); });
        system.set_seed_town(seed);
        for (const auto divisor : { 1, 3, 5, std::numeric_limits<int>::max() }) {
            const auto expected = std::to_string(seed % static_cast<uint32_t>(divisor));
            check_condition(player, "[EQU $RANDOM" + std::to_string(divisor) + " " + expected + "]", true);
        }
        const auto expected = std::to_string(seed % 5);
        check_condition(player, "[EQU $RANDOM0005 " + expected + "]", true);
        check_condition(player, "[EQU $RANDOM+5 " + expected + "]", true);
    }
}

TEST_CASE("TownMapCondition validates quest IDs before narrowing or lookup")
{
    PlayerType player;
    test::QuestListTestAccess quests;
    QuestType quest;
    quest.status = QuestStatusType::TAKEN;
    quest.type = QuestKindType::KILL_LEVEL;
    quests.seed(QuestId::THIEF, quest, QuestFixedMap{});
    for (const auto *prefix : { "QUEST", "QUEST_TYPE" }) {
        for (const auto *suffix : { "0", "2", "32767", "32768", "65537", "-1", "-65535", "2147483647" }) {
            check_invalid(player, "[EQU $" + std::string(prefix) + suffix + " 1]");
        }
        for (const auto *suffix : { "1", "01", "+1", "+0001" }) {
            const auto expected = std::to_string(std::string_view(prefix) == "QUEST" ? enum2i(quest.status) : enum2i(quest.type));
            check_condition(player, "[EQU $" + std::string(prefix) + suffix + " " + expected + "]", true);
        }
    }
    CHECK(QuestList::get_instance().size() == 1);
    CHECK(QuestList::get_instance().get_quest(QuestId::THIEF).status == quest.status);
}

TEST_CASE("TownMapCondition accepts existing quest IDs at representable boundaries")
{
    PlayerType player;
    test::QuestListTestAccess quests;
    QuestType quest;
    quests.seed(QuestId::NONE, quest, QuestFixedMap{});
    quests.seed(i2enum<QuestId>(std::numeric_limits<short>::max()), quest, QuestFixedMap{});
    for (const auto *prefix : { "QUEST", "QUEST_TYPE" }) {
        for (const auto *suffix : { "0", "-0", "32767" }) {
            check_condition(player, "[EQU $" + std::string(prefix) + suffix + " 0]", true);
        }
    }
}

TEST_CASE("TownMapCondition numeric errors remain errors in nested or false conditions")
{
    PlayerType player;
    test::QuestListTestAccess quests;
    for (const auto *expression : { "[AND 0 [EQU $RANDOM0 0]]", "[IOR 1 $QUEST9999]", "[NOT $QUEST_TYPE2]",
             "[AND $UNKNOWN $RANDOM0]", "[EQU $QUEST1x 1]" }) {
        check_invalid(player, expression);
    }
}

TEST_CASE("TownMapCondition unsigned seed selects exactly one random variant")
{
    PlayerType player;
    auto &system = AngbandSystem::get_instance();
    const auto restore_seed = util::make_finalizer([old_seed = system.get_seed_town()] { AngbandSystem::get_instance().set_seed_town(old_seed); });
    for (const auto seed : { uint32_t{ 0x80000000 }, std::numeric_limits<uint32_t>::max() }) {
        system.set_seed_town(seed);
        auto selected = 0;
        for (const auto value : { 0, 1, 2 }) {
            const auto result = is_town_map_condition_met(&player, "[EQU $RANDOM3 " + std::to_string(value) + "]");
            REQUIRE(result);
            selected += *result ? 1 : 0;
        }
        CHECK(selected == 1);
    }
}

TEST_CASE("TownMapCondition diagnostics identify file field expression and invalid variable")
{
    PlayerType player;
    const test::QuestListTestAccess quests;
    const std::string expression = "[EQU $QUEST2O 1]";
    const auto result = is_town_map_condition_met(&player, expression);
    REQUIRE_FALSE(result);
    CHECK(result.error().variable == "QUEST2O");
    for (const auto *array : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        const auto field = std::string(array) + "[30].when";
        const auto message = format_town_map_condition_error("03_Morivant.jsonc", field, expression, result.error().variable);
        CHECK(message.find("03_Morivant.jsonc") != std::string::npos);
        CHECK(message.find(field) != std::string::npos);
        CHECK(message.find(expression) != std::string::npos);
        CHECK(message.find("variable '$QUEST2O'") != std::string::npos);
    }
}
