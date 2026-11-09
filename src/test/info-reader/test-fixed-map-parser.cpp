#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/parse-error-messages.h"
#include "info-reader/town-map-condition.h"
#include "io/files-util.h"
#include "locale/language-switcher.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/z-util.h"
#include "test/info-reader/legend-ego-fixture.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-input-term.h"
#include "test/scoped-restore.h"
#include "test/system/terrain-list-test-access.h"
#include "test/temporary-json-files.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

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
        check_town_load_failure(&term.player(), ParseErrorType::TOO_FEW_ARGUMENTS, TOWN_PREFERENCES);
    }
    SUBCASE("incompatible preferences ego preserves detail before the summary")
    {
        const test::LegendEgoFixture ego_state;
        const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
        init_flags = INIT_CREATE_DUNGEON;
        nlohmann::json data;
        data["version"] = 1;
        data["legend"]["!"]["terrain"] = "*";
        data["legend"]["!"]["caveInfo"] = nlohmann::json::array();
        data["legend"]["!"]["object"] = 54;
        data["legend"]["!"]["ego"] = static_cast<int>(EgoType::DF);
        files.write(TOWN_PREFERENCES, data);
        check_town_load_failure(&term.player(), ParseErrorType::INVALID_VALUE, TOWN_PREFERENCES,
            _("町のエゴ指定が不正です: legend['!'].ego 65 (object 54): 品の種類と相性が合わない",
                "Invalid town ego: legend['!'].ego 65 (object 54): incompatible object kind"));
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
        check_town_load_failure(&term.player(), ParseErrorType::TOO_FEW_ARGUMENTS, TOWN_DEFINITION_LIST);
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
            check_town_load_failure(&term.player(), ParseErrorType::TOO_FEW_ARGUMENTS, filename);
        }
        SUBCASE("incompatible map ego preserves detail before the summary")
        {
            const test::LegendEgoFixture ego_state;
            nlohmann::json data;
            data["version"] = 3;
            data["featureRules"][0]["symbol"] = "!";
            data["featureRules"][0]["definition"]["object"] = 54;
            data["featureRules"][0]["definition"]["ego"] = static_cast<int>(EgoType::DF);
            data["buildingRules"] = nlohmann::json::array();
            data["mapVariants"][0]["rows"] = { "!" };
            data["startingPositions"][0]["y"] = 0;
            data["startingPositions"][0]["x"] = 0;
            files.write(filename, data);
            check_town_load_failure(&term.player(), ParseErrorType::INVALID_VALUE, filename,
                _("町のエゴ指定が不正です: featureRules[0] (記号 '!').ego 65 (object 54): 品の種類と相性が合わない",
                    "Invalid town ego: featureRules[0] (symbol '!').ego 65 (object 54): incompatible object kind"));
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
