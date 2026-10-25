#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/parse-error-messages.h"
#include "info-reader/town-map-condition.h"
#include "io/files-util.h"
#include "system/angband-system.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/z-util.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-input-term.h"
#include "test/scoped-restore.h"
#include "test/system/quest-list-test-access.h"
#include "test/temporary-json-files.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <limits>
#include <nlohmann/json.hpp>
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

namespace {
struct TownLoadFailure {
    std::string message;
};

void check_town_load_failure(PlayerType *player, ParseErrorType error, std::string_view filename, std::string_view diagnostic = {})
{
    // 既存の終了フックで終了文を受け取り、テストプロセスの終了だけを避ける。
    const auto restore_quit = test::scoped_restore(quit_aux);
    quit_aux = [](std::string_view message) { throw TownLoadFailure{ std::string(message) }; };
    const auto history_size = message_num();
    try {
        load_town_map(player);
        FAIL("Town load must quit on failure");
    } catch (const TownLoadFailure &failure) {
        const auto summary = fmt::format("Error {} ({}) loading '{}'.", enum2i(error), describe_parse_error(error), filename);
        const auto prefix = diagnostic.empty() ? summary : std::string(diagnostic) + "\n" + summary;
        REQUIRE(failure.message.starts_with(prefix + "\n"));
        CHECK(failure.message.size() > prefix.size() + 1);
        CHECK(failure.message.find('\n', prefix.size() + 1) == std::string::npos);
#ifndef JP
        CHECK(failure.message == prefix + "\nFailed to load the town definition");
#endif
    }
    CHECK(message_num() == history_size);
}
}

TEST_CASE("Town load failures collect diagnostics and quit without requesting input")
{
    const test::ScopedReaderState reader_state;
    test::ScopedInputTerm term;
    term.player().playing = false;
    test::TemporaryJsonFiles files;
    const auto restore = test::scoped_restore(ANGBAND_DIR_EDIT, init_flags, vanilla_town, lite_town);
    ANGBAND_DIR_EDIT = files.directory;
    init_flags = INIT_ONLY_BUILDINGS;
    vanilla_town = false;
    lite_town = false;
    auto &world = AngbandWorld::get_instance();
    const auto restore_town = util::make_finalizer([town = world.get_town_index()] { AngbandWorld::get_instance().set_town_index(town); });
    world.set_town_index(1);
    world.character_generated = true;

    SUBCASE("missing preferences in floor generation")
    {
        init_flags = INIT_CREATE_DUNGEON;
        check_town_load_failure(&term.player(), ParseErrorType::GENERIC, TOWN_PREFERENCES);
    }
    SUBCASE("invalid preferences in floor generation")
    {
        init_flags = INIT_CREATE_DUNGEON;
        files.write(TOWN_PREFERENCES, nlohmann::json::object());
        check_town_load_failure(&term.player(), ParseErrorType::INVALID_TYPE, TOWN_PREFERENCES);
    }
    SUBCASE("missing list at startup")
    {
        check_town_load_failure(&term.player(), ParseErrorType::GENERIC, TOWN_DEFINITION_LIST);
    }
    SUBCASE("pending message does not make diagnostics wait")
    {
        msg_print("Pending message");
        check_town_load_failure(&term.player(), ParseErrorType::GENERIC, TOWN_DEFINITION_LIST);
        // テストが表示した通常メッセージは、入力を待たないプレイ中の設定で片付ける。
        term.player().playing = true;
        skip_more = true;
        msg_erase();
    }
    SUBCASE("invalid list at startup")
    {
        files.write(TOWN_DEFINITION_LIST, nlohmann::json::object());
        check_town_load_failure(&term.player(), ParseErrorType::INVALID_TYPE, TOWN_DEFINITION_LIST);
    }
    SUBCASE("selected map failures at startup")
    {
        constexpr auto filename = "towns/test.jsonc";
        files.write(TOWN_DEFINITION_LIST, { { "version", 1 }, { "towns", { { "1", filename } } } });
        SUBCASE("missing file")
        {
            check_town_load_failure(&term.player(), ParseErrorType::GENERIC, filename);
        }
        SUBCASE("JSON syntax error")
        {
            files.write_raw(filename, "{");
            check_town_load_failure(&term.player(), ParseErrorType::INVALID_VALUE, filename);
        }
        SUBCASE("invalid structure")
        {
            files.write(filename, nlohmann::json::object());
            check_town_load_failure(&term.player(), ParseErrorType::INVALID_TYPE, filename);
        }
        SUBCASE("invalid condition preserves detail before the summary")
        {
            const std::string expression = "[EQU $RANDOM0 0]";
            files.write(filename, {
                                      { "version", 3 },
                                      { "featureRules", nlohmann::json::array() },
                                      { "mapVariants", { { { "rows", { "." } } } } },
                                      { "startingPositions", { { { "y", 0 }, { "x", 0 } } } },
                                      { "buildingRules", { { { "index", 0 }, { "locale", "en" }, { "command", "Z" },
                                                             { "fields", nlohmann::json::array() }, { "when", expression } } } },
                                  });
            check_town_load_failure(&term.player(), ParseErrorType::INVALID_VALUE, filename,
                format_town_map_condition_error(filename, "buildingRules[0].when", expression, "RANDOM0"));
        }
    }
    // 例外で終了経路を抜けても捕捉先が復元され、次のメッセージを受け取れる。
    ScopedMessageCapture after_load;
    msg_print("After town load");
    REQUIRE(after_load.messages().size() == 1);
    CHECK(after_load.messages().front() == "After town load");
}

TEST_CASE("Town load outside towns succeeds without loading a map or requesting input")
{
    const test::ScopedReaderState reader_state;
    test::ScopedInputTerm term;
    term.player().playing = false;
    test::TemporaryJsonFiles files;
    const auto restore = test::scoped_restore(ANGBAND_DIR_EDIT, init_flags);
    ANGBAND_DIR_EDIT = files.directory;
    init_flags = INIT_ONLY_BUILDINGS;
    auto &world = AngbandWorld::get_instance();
    const auto restore_town = util::make_finalizer([town = world.get_town_index()] { AngbandWorld::get_instance().set_town_index(town); });
    world.set_town_index(0);
    files.write(TOWN_DEFINITION_LIST, { { "version", 1 }, { "towns", { { "1", "towns/missing.jsonc" } } } });
    CHECK_NOTHROW(load_town_map(&term.player()));
}
