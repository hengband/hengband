#include "io/files-util.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/scoped-restore.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/quest-list-test-access.h"
#include "test/system/terrain-list-test-access.h"
#include "test/temporary-json-files.h"
#include "util/finalizer.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace {
using QuestFiles = test::TemporaryJsonFiles;

nlohmann::json make_quest(int id)
{
    return {
        { "id", id },
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "definition", { { "type", "KILL_ALL" }, { "level", 5 } } },
        { "map", { "..." } },
    };
}

void check_unpublished(QuestId id)
{
    CHECK(QuestList::get_instance().empty());
    CHECK_FALSE(QuestFixedMapList::get_instance().find(id));
}
}

TEST_CASE("QuestList base legend rejects unsafe symbol bytes without publication")
{
    test::QuestListTestAccess quests;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    QuestFiles files;
    const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
    auto &maps = QuestFixedMapList::get_instance();
    const auto original = maps.get_base_legend();
    const auto restore_legend = util::make_finalizer([original] {
        QuestFixedMapList::get_instance().set_base_legend(original);
    });
    ANGBAND_DIR_EDIT = files.directory;
    QuestLegendCell existing;
    existing.grid.special = 73;
    maps.set_base_legend({ { '?', existing } });
    for (const auto byte : { 0x00, 0x1f, 0x7f, 0x80, 0xfe, 0xff }) {
        CAPTURE(byte);
        if (byte < 0x80) {
            files.write("QuestPreferences.jsonc", { { "legend", { { "!", nlohmann::json::object() }, { std::string(1, static_cast<char>(byte)), nlohmann::json::object() } } } });
        } else {
            // Isolated high bytes cannot be represented in valid UTF-8 JSON strings.
            files.write_raw("QuestPreferences.jsonc", "{\"legend\":{\"!\":{},\"" + std::string(1, static_cast<char>(byte)) + "\":{}}}");
        }
        CHECK_THROWS_AS(quests.load_base_legend(), std::runtime_error);
        REQUIRE(maps.get_base_legend().size() == 1);
        CHECK(maps.get_base_legend().at('?').grid.special == 73);
    }
    for (const auto *symbol : { "", "AB" }) {
        CAPTURE(symbol);
        files.write("QuestPreferences.jsonc", { { "legend", { { symbol, nlohmann::json::object() } } } });
        CHECK_THROWS_AS(quests.load_base_legend(), std::runtime_error);
        REQUIRE(maps.get_base_legend().size() == 1);
        CHECK(maps.get_base_legend().at('?').grid.special == 73);
    }
}

TEST_CASE("QuestList base legend accepts printable ASCII boundaries")
{
    test::QuestListTestAccess quests;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    QuestFiles files;
    const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
    auto &maps = QuestFixedMapList::get_instance();
    const auto original = maps.get_base_legend();
    const auto restore_legend = util::make_finalizer([original] {
        QuestFixedMapList::get_instance().set_base_legend(original);
    });
    ANGBAND_DIR_EDIT = files.directory;
    files.write("QuestPreferences.jsonc", { { "legend", { { " ", { { "special", 17 } } }, { "~", { { "special", 29 } } } } } });
    REQUIRE_NOTHROW(quests.load_base_legend());
    REQUIRE(maps.get_base_legend().size() == 2);
    CHECK(maps.get_base_legend().at(' ').grid.special == 17);
    CHECK(maps.get_base_legend().at('~').grid.special == 29);
}

TEST_CASE("QuestList validates raw IDs before conversion or publication")
{
    test::QuestListTestAccess quests;
    QuestFiles files;
    auto data = make_quest(1);
    for (const auto &id : { nlohmann::json(-1), nlohmann::json(0), nlohmann::json(50), nlohmann::json(65537),
             nlohmann::json((int64_t{ 1 } << 32) + 1), nlohmann::json(std::numeric_limits<int64_t>::min()),
             nlohmann::json(std::numeric_limits<int64_t>::max()), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        CAPTURE(id);
        data["id"] = id;
        files.write("quest.jsonc", data);
        CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
        check_unpublished(QuestId::THIEF);
    }
}

TEST_CASE("QuestList rejects missing and incorrectly typed IDs without publication")
{
    test::QuestListTestAccess quests;
    QuestFiles files;
    auto data = make_quest(1);
    data.erase("id");
    files.write("quest.jsonc", data);
    CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
    check_unpublished(QuestId::THIEF);
    for (const auto &id : { nlohmann::json(), nlohmann::json(true), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json::array(), nlohmann::json::object() }) {
        data["id"] = id;
        files.write("quest.jsonc", data);
        CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
        check_unpublished(QuestId::THIEF);
    }
}

TEST_CASE("QuestList rejects non-object roots without publication")
{
    test::QuestListTestAccess quests;
    QuestFiles files;
    for (const auto &data : { nlohmann::json(), nlohmann::json("quest"), nlohmann::json::array() }) {
        files.write("quest.jsonc", data);
        CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
        check_unpublished(QuestId::THIEF);
    }
}

TEST_CASE("QuestList leaves malformed JSON unpublished")
{
    test::QuestListTestAccess quests;
    QuestFiles files;
    files.write_raw("quest.jsonc", "{\"id\":1,");
    CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
    check_unpublished(QuestId::THIEF);
}

TEST_CASE("QuestList publishes valid boundary and unnamed IDs")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    for (const auto id : { 1, 35, 36, 37, 38, 39, 49 }) {
        files.write(std::to_string(id) + ".jsonc", make_quest(id));
    }
    REQUIRE_NOTHROW(quests.load(files.directory));
    CHECK(QuestList::get_instance().size() == 7);
    for (const auto id : { 1, 35, 36, 37, 38, 39, 49 }) {
        const auto quest_id = static_cast<QuestId>(id);
        const auto &quest = QuestList::get_instance().get_quest(quest_id);
        CHECK(quest.name == "Test");
        CHECK(quest.level == 5);
        CHECK(quest.type == QuestKindType::KILL_ALL);
        const auto map = QuestFixedMapList::get_instance().find(quest_id);
        REQUIRE(map);
        CHECK(map->maps == std::vector<std::vector<std::string>>{ { "..." } });
    }
}

TEST_CASE("QuestList leaves failed late reader output unpublished")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    auto &monrace = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    monrace.kind_flags.set(MonsterKindType::UNIQUE);
    QuestFiles files;
    auto data = make_quest(1);
    data["definition"]["monster"] = 1;
    data["start"] = { { "y", 0 } };
    files.write("quest.jsonc", data);
    CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
    check_unpublished(QuestId::THIEF);
    CHECK_FALSE(monrace.misc_flags.has(MonsterMiscType::QUESTOR));
}

TEST_CASE("QuestList leaves failed metadata application unpublished")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    QuestFiles files;
    auto data = make_quest(1);
    data["definition"]["monster"] = 123;
    files.write("quest.jsonc", data);
    CHECK_THROWS_AS(quests.load(files.directory), std::out_of_range);
    check_unpublished(QuestId::THIEF);
    CHECK(MonraceList::get_instance().empty());
}

TEST_CASE("QuestList applies UNIQUE metadata before publishing both records")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    auto &monrace = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    monrace.kind_flags.set(MonsterKindType::UNIQUE);
    QuestFiles files;
    auto data = make_quest(1);
    data["definition"]["monster"] = 1;
    files.write("quest.jsonc", data);
    REQUIRE_NOTHROW(quests.load(files.directory));
    const auto &quest = QuestList::get_instance().get_quest(QuestId::THIEF);
    CHECK(quest.r_idx == static_cast<MonraceId>(1));
    CHECK(quest.level == 5);
    CHECK(monrace.misc_flags.has(MonsterMiscType::QUESTOR));
    REQUIRE(QuestFixedMapList::get_instance().find(QuestId::THIEF));
}

TEST_CASE("QuestList preserves first records when a duplicate follows")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    files.write("01.jsonc", make_quest(1));
    auto duplicate = make_quest(1);
    duplicate["name"] = { { "ja", "Duplicate" }, { "en", "Duplicate" } };
    duplicate["map"] = { "bad" };
    files.write("02.jsonc", duplicate);
    CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
    CHECK(QuestList::get_instance().get_quest(QuestId::THIEF).name == "Test");
    const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
    REQUIRE(map);
    CHECK(map->maps == std::vector<std::vector<std::string>>{ { "..." } });
}

TEST_CASE("QuestList retains earlier success when a later file fails")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    files.write("01.jsonc", make_quest(1));
    auto bad = make_quest(2);
    bad["start"] = { { "y", 0 } };
    files.write("02.jsonc", bad);
    CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
    CHECK(QuestList::get_instance().size() == 1);
    CHECK(QuestList::get_instance().get_quest(QuestId::THIEF).name == "Test");
    CHECK(QuestFixedMapList::get_instance().find(QuestId::THIEF));
    CHECK_FALSE(QuestFixedMapList::get_instance().find(QuestId::SEWER));
}

TEST_CASE("QuestList test access restores quest and monrace identity after exceptions")
{
    test::QuestListTestAccess outer_quests;
    test::MonraceListTestAccess outer_monraces;
    auto &monrace = MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    files.write("quest.jsonc", make_quest(1));
    REQUIRE_NOTHROW(outer_quests.load(files.directory));
    const auto *quest = &QuestList::get_instance().get_quest(QuestId::THIEF);
    const auto saved_monrace = MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0));
    const auto throw_with_fixtures = [] {
        test::QuestListTestAccess quests;
        test::MonraceListTestAccess monraces;
        CHECK(QuestList::get_instance().empty());
        CHECK(MonraceList::get_instance().empty());
        throw std::runtime_error("test exception");
    };
    CHECK_THROWS_AS(throw_with_fixtures(), std::runtime_error);
    CHECK(&QuestList::get_instance().get_quest(QuestId::THIEF) == quest);
    CHECK(MonraceList::get_instance().get_monrace_shared(static_cast<MonraceId>(0)) == saved_monrace);
    CHECK(&MonraceList::get_instance().get_monrace(static_cast<MonraceId>(0)) == &monrace);
    CHECK(QuestFixedMapList::get_instance().find(QuestId::THIEF));
}
