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
#include <cstdint>
#include <doctest/doctest.h>
#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

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
    ANGBAND_DIR_EDIT = files.directory;
    QuestLegendCell existing;
    existing.grid.special = 73;
    maps.set_base_legend({ { '?', existing } });
    for (const auto *escaped_byte : { "\\u0000", "\\u001f", "\\u007f" }) {
        CAPTURE(escaped_byte);
        files.write_raw("QuestPreferences.jsonc", "{\"legend\":{\"!\":{},\"" + std::string(escaped_byte) + "\":{}}}");
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

TEST_CASE("QuestList reports missing target monsters without publication")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    QuestFiles files;
    auto data = make_quest(1);
    data["definition"]["monster"] = 123;
    files.write("quest.jsonc", data);
    try {
        quests.load(files.directory);
        FAIL("Expected a runtime_error for the missing monster");
    } catch (const std::runtime_error &error) {
        const std::string diagnostic(error.what());
        CHECK(diagnostic.find("quest.jsonc") != std::string::npos);
        CHECK(diagnostic.find("monster 123") != std::string::npos);
    }
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

TEST_CASE("QuestList leaves all records unpublished when a duplicate follows")
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
    check_unpublished(QuestId::THIEF);
}

TEST_CASE("QuestList leaves all records unpublished when a later file fails")
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
    check_unpublished(QuestId::THIEF);
    CHECK_FALSE(QuestFixedMapList::get_instance().find(QuestId::SEWER));
}

TEST_CASE("QuestList initialization preserves every store on late input errors")
{
    for (const auto seeded : { false, true }) {
        for (const auto *failure : { "syntax", "root", "id", "duplicate", "type", "range", "start", "monster" }) {
            CAPTURE(seeded);
            CAPTURE(failure);
            test::QuestListTestAccess quests;
            test::MonraceListTestAccess monraces;
            const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
            auto &list = QuestList::get_instance();
            auto &maps = QuestFixedMapList::get_instance();
            MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
            auto &target = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
            target.kind_flags.set(MonsterKindType::UNIQUE);
            target.misc_flags.set(MonsterMiscType::GUARDIAN);
            auto &already_questor = MonraceList::get_instance().emplace(static_cast<MonraceId>(2));
            already_questor.kind_flags.set(MonsterKindType::UNIQUE);
            already_questor.misc_flags.set(MonsterMiscType::QUESTOR);
            const auto target_flags = target.misc_flags;
            const auto questor_flags = already_questor.misc_flags;

            QuestLegendCell old_cell;
            old_cell.grid.special = 73;
            QuestFixedMap old_map;
            old_map.maps = { { "old", "layout" } };
            old_map.legend = { { '?', old_cell } };
            old_map.metadata.present = true;
            old_map.metadata.level = 41;
            old_map.reward_artifact_candidates = { 7, 9 };
            QuestType old_quest;
            old_quest.name = "Old quest";
            old_quest.status = QuestStatusType::TAKEN;
            old_quest.type = QuestKindType::KILL_NUMBER;
            old_quest.level = 41;
            old_quest.cur_num = 3;
            old_quest.max_num = 8;
            old_quest.flags = QUEST_FLAG_SILENT;
            if (seeded) {
                quests.seed(QuestId::NONE, old_quest, old_map);
                quests.seed(QuestId::RANDOM_QUEST10, old_quest, old_map);
                maps.set_base_legend({ { '?', old_cell } });
            }
            const auto *old_none = seeded ? &list.get_quest(QuestId::NONE) : nullptr;
            const auto *old_last = seeded ? &list.get_quest(QuestId::RANDOM_QUEST10) : nullptr;
            QuestFiles files;
            const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
            ANGBAND_DIR_EDIT = files.directory;
            files.write("QuestPreferences.jsonc", { { "legend", { { "+", { { "special", 91 } } } } } });
            auto first = make_quest(1);
            first["definition"]["monster"] = 1;
            files.write("quests/01.jsonc", first);
            auto bad = make_quest(2);
            const std::string kind(failure);
            if (kind == "root") {
                bad = nlohmann::json::array();
            } else if (kind == "id") {
                bad["id"] = uint64_t{ 1 } << 32;
            } else if (kind == "duplicate") {
                bad["id"] = 1;
            } else if (kind == "type") {
                bad["map"] = true;
            } else if (kind == "range") {
                bad["definition"]["level"] = int64_t{ 1 } << 32;
            } else if (kind == "start") {
                bad["start"] = { { "y", 0 } };
            } else if (kind == "monster") {
                bad["definition"]["monster"] = 123;
            }
            if (kind == "syntax") {
                files.write_raw("quests/02.jsonc", "{\"id\":2,");
            } else {
                files.write("quests/02.jsonc", bad);
            }

            try {
                list.initialize();
                FAIL("Expected a runtime_error for the invalid input");
            } catch (const std::runtime_error &error) {
                if (kind == "monster") {
                    const std::string diagnostic(error.what());
                    CHECK(diagnostic.find("02.jsonc") != std::string::npos);
                    CHECK(diagnostic.find("monster 123") != std::string::npos);
                }
            }
            CHECK(list.size() == (seeded ? 2 : 0));
            CHECK_FALSE(list.contains(QuestId::THIEF));
            CHECK_FALSE(list.contains(QuestId::SEWER));
            CHECK_FALSE(maps.find(QuestId::THIEF));
            CHECK_FALSE(maps.find(QuestId::SEWER));
            CHECK(target.misc_flags == target_flags);
            CHECK(already_questor.misc_flags == questor_flags);
            CHECK(maps.get_base_legend().size() == (seeded ? 1 : 0));
            if (seeded) {
                CHECK(&list.get_quest(QuestId::NONE) == old_none);
                CHECK(&list.get_quest(QuestId::RANDOM_QUEST10) == old_last);
                for (const auto id : { QuestId::NONE, QuestId::RANDOM_QUEST10 }) {
                    const auto &quest = list.get_quest(id);
                    CHECK(quest.name == old_quest.name);
                    CHECK(quest.status == old_quest.status);
                    CHECK(quest.type == old_quest.type);
                    CHECK(quest.level == old_quest.level);
                    CHECK(quest.cur_num == old_quest.cur_num);
                    CHECK(quest.max_num == old_quest.max_num);
                    CHECK(quest.flags == old_quest.flags);
                    const auto map = maps.find(id);
                    REQUIRE(map);
                    CHECK(map->maps == old_map.maps);
                    REQUIRE(map->legend.size() == 1);
                    CHECK(map->legend.at('?').grid.special == 73);
                    CHECK(map->metadata.present);
                    CHECK(map->metadata.level == 41);
                    CHECK(map->reward_artifact_candidates == old_map.reward_artifact_candidates);
                }
                CHECK(maps.get_base_legend().at('?').grid.special == 73);
            } else {
                CHECK_FALSE(list.contains(QuestId::NONE));
                CHECK_FALSE(maps.find(QuestId::NONE));
                CHECK_FALSE(maps.find(QuestId::RANDOM_QUEST10));
            }
        }
    }
}

TEST_CASE("QuestList initialization leaves NONE and base legend unpublished on early errors")
{
    for (const auto *failure : { "missing legend", "malformed legend", "invalid legend", "missing directory", "empty directory" }) {
        CAPTURE(failure);
        test::QuestListTestAccess quests;
        const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
        QuestFiles files;
        const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
        ANGBAND_DIR_EDIT = files.directory;
        const std::string kind(failure);
        if (kind == "malformed legend") {
            files.write_raw("QuestPreferences.jsonc", "{\"legend\":");
        } else if (kind == "invalid legend") {
            files.write("QuestPreferences.jsonc", { { "legend", { { "AB", nlohmann::json::object() } } } });
        } else if (kind != "missing legend") {
            files.write("QuestPreferences.jsonc", { { "legend", { { ".", nlohmann::json::object() } } } });
        }
        if (kind == "empty directory") {
            std::filesystem::create_directory(files.directory / "quests");
        }
        CHECK_THROWS_AS(QuestList::get_instance().initialize(), std::runtime_error);
        check_unpublished(QuestId::NONE);
        CHECK(QuestFixedMapList::get_instance().get_base_legend().empty());
    }
}

TEST_CASE("QuestList initialization publishes complete metadata and flags after validation")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    auto &list = QuestList::get_instance();
    auto &maps = QuestFixedMapList::get_instance();
    auto &ordinary = MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    ordinary.misc_flags.set(MonsterMiscType::GUARDIAN);
    const auto ordinary_flags = ordinary.misc_flags;
    auto &unique = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    unique.kind_flags.set(MonsterKindType::UNIQUE);
    unique.misc_flags.set(MonsterMiscType::GUARDIAN);
    QuestFiles files;
    const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
    ANGBAND_DIR_EDIT = files.directory;
    files.write("QuestPreferences.jsonc", { { "legend", { { ".", { { "special", 91 } } } } } });
    auto first = make_quest(1);
    first["definition"]["monster"] = 1;
    first["definition"]["maxNum"] = 8;
    first["definition"]["flags"] = { "SILENT" };
    first["definition"]["reward"] = { { "artifact", 7 }, { "artifacts", { 8, 9 } } };
    first["legend"] = { { "?", { { "special", 17 } } } };
    first["start"] = { { "y", 0 }, { "x", 1 } };
    first["descriptions"] = { { { "text", { { "ja", { "Description" } }, { "en", { "Description" } } } } } };
    files.write("quests/01.jsonc", first);
    auto second = make_quest(2);
    second["name"] = { { "ja", "Second" }, { "en", "Second" } };
    second.erase("map");
    second["mapVariants"] = { { "..." }, { "???", "..." } };
    files.write("quests/02.jsonc", second);

    REQUIRE_NOTHROW(list.initialize());
    CHECK(list.size() == 3);
    CHECK(list.get_quest(QuestId::NONE).status == QuestStatusType::UNTAKEN);
    const auto &quest = list.get_quest(QuestId::THIEF);
    CHECK(quest.name == "Test");
    CHECK(quest.status == QuestStatusType::UNTAKEN);
    CHECK(quest.type == QuestKindType::KILL_ALL);
    CHECK(quest.level == 5);
    CHECK(quest.max_num == 8);
    CHECK(quest.flags == QUEST_FLAG_SILENT);
    CHECK(quest.r_idx == static_cast<MonraceId>(1));
    CHECK_FALSE(quest.has_reward());
    CHECK(unique.misc_flags.has(MonsterMiscType::QUESTOR));
    CHECK(unique.misc_flags.has(MonsterMiscType::GUARDIAN));
    CHECK(ordinary.misc_flags == ordinary_flags);
    CHECK(maps.get_base_legend().at('.').grid.special == 91);
    const auto map = maps.find(QuestId::THIEF);
    REQUIRE(map);
    CHECK(map->maps == std::vector<std::vector<std::string>>{ { "..." } });
    CHECK(map->legend.at('?').grid.special == 17);
    REQUIRE(map->starts.size() == 1);
    CHECK(map->starts[0].y == 0);
    CHECK(map->starts[0].x == 1);
    REQUIRE(map->descriptions.size() == 1);
    CHECK(map->descriptions[0].lines_ja == std::vector<std::string>{ "Description" });
    CHECK(map->descriptions[0].lines_en == std::vector<std::string>{ "Description" });
    CHECK(map->metadata.reward_artifact == 7);
    CHECK(map->reward_artifact_candidates == std::vector<int>{ 8, 9 });
    CHECK(list.get_quest(QuestId::SEWER).name == "Second");
    const auto second_map = maps.find(QuestId::SEWER);
    REQUIRE(second_map);
    CHECK(second_map->maps == std::vector<std::vector<std::string>>{ { "..." }, { "???", "..." } });
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
