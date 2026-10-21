#include "artifact/fixed-art-types.h"
#include "info-reader/parse-error-types.h"
#include "io/files-util.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-conversion-type.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/scoped-restore.h"
#include "test/scoped-rng.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/dungeon-list-test-access.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/quest-list-test-access.h"
#include "test/temporary-json-files.h"
#include "util/enum-converter.h"
#include <array>
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

// 実際のロード順どおり、参照先の定義を先に用意する。各storeとタグは終了時に復元する。
class LegendReferenceState {
public:
    LegendReferenceState()
    {
        MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
        auto &target = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
        target.kind_flags.set(MonsterKindType::UNIQUE);
        target.misc_flags.set(MonsterMiscType::GUARDIAN);
        ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(1), ArtifactDefinition{});
        DungeonList::get_instance().emplace(static_cast<DungeonId>(1), DungeonDefinition{});
        BaseitemList::get_instance().resize(3);
        BaseitemList::get_instance().get_baseitem(1).name = "Valid";
        auto &terrains = TerrainList::get_instance();
        terrains.resize(7);
        const std::vector<std::string> tags{ "NONE", "QUEST_UP", "QUEST_ENTER", "QUEST_EXIT", "ENTRANCE", "UP_STAIR", "OTHER" };
        for (size_t i = 0; i < tags.size(); ++i) {
            terrains.get_terrain(static_cast<short>(i)).tag = tags[i];
            terrains.get_terrain(static_cast<short>(i)).mimic = static_cast<short>(i);
        }
        terrains.get_terrain(1).flags.set({ TerrainCharacteristics::QUEST, TerrainCharacteristics::UP_STAIRS });
        terrains.get_terrain(2).flags.set({ TerrainCharacteristics::QUEST_ENTER, TerrainCharacteristics::DOWN_STAIRS });
        terrains.get_terrain(3).flags.set({ TerrainCharacteristics::QUEST_EXIT, TerrainCharacteristics::UP_STAIRS });
        terrains.get_terrain(4).flags.set({ TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS });
        terrains.get_terrain(5).flags.set(TerrainCharacteristics::UP_STAIRS);
    }

    LegendReferenceState(const LegendReferenceState &) = delete;
    LegendReferenceState &operator=(const LegendReferenceState &) = delete;

    test::QuestListTestAccess quests;

private:
    test::MonraceListTestAccess monraces;
    test::ArtifactListTestAccess artifacts;
    test::DungeonListTestAccess dungeons;
    test::ScopedVectorWrapper<BaseitemList> baseitems{ BaseitemList::get_instance() };
    test::ScopedVectorWrapper<TerrainList> terrain_definitions{ TerrainList::get_instance() };
};

nlohmann::json make_quest(int id)
{
    return {
        { "id", id },
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "definition", { { "type", "KILL_ALL" }, { "level", 5 } } },
        // 地図の記号は凡例で定義する必要がある。
        { "legend", { { ".", nlohmann::json::object() } } },
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
    // 地図の記号は凡例に定義し、重複した ID の検出で拒否されることを確かめる。
    for (const auto *symbol : { "b", "a", "d" }) {
        duplicate["legend"][symbol] = nlohmann::json::object();
    }
    files.write("02.jsonc", duplicate);
    try {
        quests.load(files.directory);
        FAIL("Expected a runtime_error for the duplicated ID");
    } catch (const std::runtime_error &error) {
        CHECK(std::string(error.what()).find("id 1") != std::string::npos);
    }
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
        for (const auto *failure : { "syntax", "root", "id", "duplicate", "type", "range", "start", "start bounds", "start no map", "start variant", "monster", "dungeon", "reward", "reward candidate", "leaving quest", "leaving wrap" }) {
            CAPTURE(seeded);
            CAPTURE(std::string(failure));
            test::QuestListTestAccess quests;
            test::MonraceListTestAccess monraces;
            test::ArtifactListTestAccess artifacts;
            test::DungeonListTestAccess dungeons;
            test::ScopedVectorWrapper<TerrainList> terrain_definitions(TerrainList::get_instance());
            TerrainList::get_instance().resize(1);
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
            } else if (kind == "start bounds") {
                bad["start"] = { { "y", 0 }, { "x", 3 } };
            } else if (kind == "start no map") {
                bad.erase("map");
                bad["start"] = { { "y", 0 }, { "x", 0 } };
            } else if (kind == "start variant") {
                bad["startVariants"] = { { { "y", 0 }, { "x", 0 } }, { { "leavingQuest", 49 }, { "y", 0 }, { "x", 3 } } };
            } else if (kind == "monster") {
                bad["definition"]["monster"] = 123;
            } else if (kind == "dungeon") {
                bad["definition"]["dungeon"] = 123;
            } else if (kind == "reward") {
                bad["definition"]["reward"] = { { "artifact", 123 } };
            } else if (kind == "reward candidate") {
                bad["definition"]["reward"] = { { "artifacts", { 0, 123 } } };
            } else if (kind == "leaving quest" || kind == "leaving wrap") {
                bad["startVariants"] = { { { "leavingQuest", kind == "leaving wrap" ? 65537 : 48 }, { "y", 0 }, { "x", 0 } } };
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
                } else if (kind == "dungeon" || kind == "reward" || kind == "reward candidate" || kind.starts_with("leaving ")) {
                    const std::string diagnostic(error.what());
                    CHECK(diagnostic.find("02.jsonc") != std::string::npos);
                    const auto field = kind == "dungeon" ? "definition.dungeon" : kind == "reward"         ? "definition.reward.artifact"
                                                                              : kind == "reward candidate" ? "definition.reward.artifacts[1]"
                                                                                                           : "startVariants[0].leavingQuest";
                    const auto id = kind == "leaving wrap" ? "65537" : kind == "leaving quest" ? "48"
                                                                                               : "123";
                    CHECK(diagnostic.find(std::string(field) + " " + id) != std::string::npos);
                } else if (kind.starts_with("start ")) {
                    const std::string diagnostic(error.what());
                    CHECK(diagnostic.find("02.jsonc") != std::string::npos);
                    CHECK(diagnostic.find(std::to_string(enum2i(ParseErrorType::OUT_OF_BOUNDS))) != std::string::npos);
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

TEST_CASE("QuestList rejects invalid maps and starts before publishing")
{
    const std::vector<nlohmann::json> inputs = {
        { { "map", { "...", ".", "..." } }, { "start", { { "y", 1 }, { "x", 1 } } } },
        { { "map", { "..." } }, { "start", { { "y", -1 }, { "x", 0 } } } },
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", -1 } } } },
        { { "map", { "..." } }, { "start", { { "y", 1 }, { "x", 0 } } } },
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", 3 } } } },
        { { "map", { "..." } }, { "start", { { "y", std::numeric_limits<int>::max() }, { "x", 0 } } } },
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", std::numeric_limits<int>::max() } } } },
        { { "map", { "" } }, { "start", { { "y", 0 }, { "x", 0 } } } },
        { { "map", nlohmann::json::array() }, { "start", { { "y", 0 }, { "x", 0 } } } },
        { { "start", { { "y", 0 }, { "x", 0 } } } },
        { { "mapVariants", { { "..." }, { "." } } }, { "start", { { "y", 0 }, { "x", 1 } } } },
        { { "mapVariants", { { "..." }, nlohmann::json::array() } }, { "start", { { "y", 0 }, { "x", 0 } } } },
        { { "map", { "..." } }, { "startVariants", { { { "y", 0 }, { "x", 0 } }, { { "leavingQuest", 49 }, { "y", 0 }, { "x", 3 } } } } },
    };
    for (const auto &input : inputs) {
        CAPTURE(input);
        test::QuestListTestAccess quests;
        test::MonraceListTestAccess monraces;
        MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
        QuestFiles files;
        auto data = make_quest(1);
        data.erase("map");
        data.update(input);
        files.write("quest.jsonc", data);
        CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
        check_unpublished(QuestId::THIEF);
    }
}

TEST_CASE("QuestList preserves valid rectangular starts and optional start semantics")
{
    const std::vector<nlohmann::json> inputs = {
        { { "map", { "...", "...", "..." } }, { "start", { { "y", 1 }, { "x", 0 } } } },
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", 2 } } } },
        { { "mapVariants", { { "..." }, { "..." } } }, { "start", { { "y", 0 }, { "x", 0 } } } },
        { { "map", { "..." } }, { "startVariants", { { { "leavingQuest", 49 }, { "y", 0 }, { "x", 2 } } } } },
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", 2 } } }, { "mapVariants", { nlohmann::json::array() } },
            { "startVariants", { { { "y", -1 }, { "x", -1 } } } } },
        { { "map", { "..." } } },
        nlohmann::json::object(),
        { { "map", nlohmann::json::array() } },
    };
    for (const auto &input : inputs) {
        CAPTURE(input);
        test::QuestListTestAccess quests;
        test::MonraceListTestAccess monraces;
        MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
        QuestFiles files;
        auto data = make_quest(1);
        data.erase("map");
        data.update(input);
        files.write("quest.jsonc", data);
        if (!input.contains("start") && input.contains("startVariants")) {
            files.write("referenced.jsonc", make_quest(49));
        }
        REQUIRE_NOTHROW(quests.load(files.directory));
        const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
        REQUIRE(map);
        if (input.contains("start")) {
            REQUIRE(map->starts.size() == 1);
            CHECK(map->starts[0].y == input["start"]["y"].get<int>());
            CHECK(map->starts[0].x == input["start"]["x"].get<int>());
        } else if (input.contains("startVariants")) {
            REQUIRE(map->starts.size() == 1);
            CHECK(map->starts[0].leaving_quest == 49);
            CHECK(map->starts[0].x == 2);
        } else {
            CHECK(map->starts.empty());
        }
    }
}

TEST_CASE("QuestList rejects map symbols defined in neither legend before publishing")
{
    struct Input {
        nlohmann::json maps;
        bool base_defines;
        bool quest_defines;
        bool valid;
    };
    const std::vector<Input> inputs{
        { { { "map", { ".?." } } }, false, false, false },
        // 使われない候補も、キャラクタによっては選ばれるため確かめる。
        { { { "mapVariants", { { "..." }, { ".?." } } } }, false, false, false },
        { { { "map", { ".?." } } }, true, false, true },
        { { { "map", { ".?." } } }, false, true, true },
        // map があれば mapVariants は読まれない。
        { { { "map", { "..." } }, { "mapVariants", { { "?" }, { "?" } } } }, false, false, true },
    };
    for (const auto &input : inputs) {
        CAPTURE(input.maps);
        CAPTURE(input.base_defines);
        CAPTURE(input.quest_defines);
        test::QuestListTestAccess quests;
        test::MonraceListTestAccess monraces;
        MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
        auto &target = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
        target.kind_flags.set(MonsterKindType::UNIQUE);
        const auto target_flags = target.misc_flags;
        if (input.base_defines) {
            QuestFixedMapList::get_instance().set_base_legend({ { '?', QuestLegendCell{} } });
        }
        QuestFiles files;
        auto first = make_quest(1);
        first["definition"]["monster"] = 1;
        files.write("01.jsonc", first);
        auto data = make_quest(2);
        data.erase("map");
        data.update(input.maps);
        if (input.quest_defines) {
            data["legend"]["?"] = nlohmann::json::object();
        }
        files.write("02.jsonc", data);
        if (input.valid) {
            REQUIRE_NOTHROW(quests.load(files.directory));
            CHECK(QuestFixedMapList::get_instance().find(QuestId::SEWER));
            continue;
        }
        try {
            quests.load(files.directory);
            FAIL("Expected a runtime_error for the undefined symbol");
        } catch (const std::runtime_error &error) {
            const std::string diagnostic(error.what());
            CHECK(diagnostic.find("02.jsonc") != std::string::npos);
            CHECK(diagnostic.find("'?'") != std::string::npos);
        }
        check_unpublished(QuestId::THIEF);
        CHECK(target.misc_flags == target_flags);
    }
}

TEST_CASE("QuestList initialization leaves NONE and base legend unpublished on early errors")
{
    for (const auto *failure : { "missing legend", "malformed legend", "invalid legend", "missing directory", "empty directory" }) {
        CAPTURE(std::string(failure));
        test::QuestListTestAccess quests;
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
    test::ArtifactListTestAccess artifacts;
    for (const auto id : { 7, 8, 9 }) {
        ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(id), ArtifactDefinition{});
    }
    test::ScopedVectorWrapper<TerrainList> terrain_definitions(TerrainList::get_instance());
    TerrainList::get_instance().resize(1);
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
    first["legend"]["?"] = { { "special", 17 } };
    first["start"] = { { "y", 0 }, { "x", 1 } };
    first["descriptions"] = { { { "text", { { "ja", { "Description" } }, { "en", { "Description" } } } } } };
    files.write("quests/01.jsonc", first);
    auto second = make_quest(2);
    second["name"] = { { "ja", "Second" }, { "en", "Second" } };
    second.erase("map");
    second["mapVariants"] = { { "..." }, { "???", "..." } };
    second["legend"]["?"] = nlohmann::json::object();
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

TEST_CASE("QuestList resolves metadata and leaving quests after loading every file")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    test::ArtifactListTestAccess artifacts;
    test::DungeonListTestAccess dungeons;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(1), ArtifactDefinition{});
    ArtifactList::get_instance().emplace(static_cast<FixedArtifactId>(2), ArtifactDefinition{});
    DungeonList::get_instance().emplace(static_cast<DungeonId>(1), DungeonDefinition{});
    QuestFiles files;
    auto first = make_quest(1);
    first["definition"]["dungeon"] = 1;
    first["definition"]["reward"] = { { "artifact", 1 }, { "artifacts", { 0, 2 } } };
    first["startVariants"] = { { { "leavingQuest", 2 }, { "y", 0 }, { "x", 0 } } };
    auto second = make_quest(2);
    second["definition"]["dungeon"] = 0;
    second["definition"]["reward"] = { { "artifact", 0 }, { "artifacts", { 0 } } };
    second["startVariants"] = { { { "leavingQuest", 1 }, { "y", 0 }, { "x", 0 } }, { { "leavingQuest", 0 }, { "y", 0 }, { "x", 1 } } };
    files.write("01.jsonc", first);
    files.write("02.jsonc", second);
    REQUIRE_NOTHROW(quests.load(files.directory));
    const auto &list = QuestList::get_instance();
    CHECK(list.size() == 2);
    CHECK_FALSE(list.contains(QuestId::NONE));
    CHECK(list.get_quest(QuestId::THIEF).dungeon == static_cast<DungeonId>(1));
    CHECK_FALSE(list.get_quest(QuestId::THIEF).has_reward());
    const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
    REQUIRE(map);
    CHECK(map->metadata.reward_artifact == 1);
    CHECK(map->reward_artifact_candidates == std::vector<int>{ 0, 2 });
    REQUIRE(map->starts.size() == 1);
    CHECK(map->starts[0].leaving_quest == 2);
    const auto second_map = QuestFixedMapList::get_instance().find(QuestId::SEWER);
    REQUIRE(second_map);
    REQUIRE(second_map->starts.size() == 2);
    CHECK(second_map->starts[0].leaving_quest == 1);
    CHECK(second_map->starts[1].leaving_quest == 0);
}

TEST_CASE("QuestList rejects negative metadata references and wide leaving quest IDs")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    test::ArtifactListTestAccess artifacts;
    test::DungeonListTestAccess dungeons;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    for (const auto &field : { "dungeon", "artifact", "artifacts", "leavingQuest" }) {
        for (const auto id : { -1, -32768, 32767 }) {
            CAPTURE(field);
            CAPTURE(id);
            auto data = make_quest(1);
            const std::string key(field);
            if (key == "dungeon") {
                data["definition"][key] = id;
            } else if (key == "artifact") {
                data["definition"]["reward"][key] = id;
            } else if (key == "artifacts") {
                data["definition"]["reward"][key] = { 0, id };
            } else {
                data["startVariants"] = { { { key, id }, { "y", 0 }, { "x", 0 } } };
            }
            files.write("quest.jsonc", data);
            CHECK_THROWS_AS(quests.load(files.directory), std::runtime_error);
            check_unpublished(QuestId::THIEF);
        }
    }
}

TEST_CASE("QuestList rejects missing local and base legend references before publication")
{
    struct InvalidCell {
        nlohmann::json data;
        std::string field;
        int id;
    };
    const std::vector<InvalidCell> invalid_cells{
        { { { "monster", 123 } }, "monster", 123 },
        { { { "monster", -123 } }, "monster", -123 },
        { { { "monster", { { "cloneOf", 123 } } } }, "monster", -123 },
        { { { "object", 123 } }, "object", 123 },
        { { { "object", -1 } }, "object", -1 },
        { { { "object", 2 } }, "object", 2 },
        { { { "artifact", 123 } }, "artifact", 123 },
        { { { "artifact", -1 } }, "artifact", -1 },
        { { { "artifact", { { "random", true }, { "id", 123 } } } }, "artifact", 123 },
        { { { "terrain", "QUEST_UP" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "QUEST_ENTER" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "QUEST_EXIT" }, { "special", -1 } }, "special (QuestId)", -1 },
        { { { "terrain", "UP_STAIR" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "ENTRANCE" }, { "special", 48 } }, "special (DungeonId)", 48 },
        { { { "terrain", "OTHER" }, { "trap", "QUEST_UP" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "OTHER" }, { "trap", "QUEST_ENTER" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "OTHER" }, { "trap", "QUEST_EXIT" }, { "special", -1 } }, "special (QuestId)", -1 },
        { { { "terrain", "OTHER" }, { "trap", "UP_STAIR" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "OTHER" }, { "trap", "ENTRANCE" }, { "special", 48 } }, "special (DungeonId)", 48 },
        { { { "terrain", "QUEST_ENTER" }, { "trap", "OTHER" }, { "special", 48 } }, "special (QuestId)", 48 },
        { { { "terrain", "ENTRANCE" }, { "trap", "OTHER" }, { "special", 48 } }, "special (DungeonId)", 48 },
    };
    for (const auto base : { false, true }) {
        for (const auto seeded : { false, true }) {
            for (const auto &input : invalid_cells) {
                CAPTURE(base);
                CAPTURE(seeded);
                CAPTURE(input.data);
                LegendReferenceState state;
                auto &list = QuestList::get_instance();
                auto &maps = QuestFixedMapList::get_instance();
                auto &target = MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1));
                const auto old_flags = target.misc_flags;
                QuestType old_quest;
                old_quest.name = "Existing";
                old_quest.status = QuestStatusType::TAKEN;
                QuestFixedMap old_map;
                old_map.maps = { { "old" } };
                QuestLegendCell old_cell;
                old_cell.grid.special = 73;
                if (seeded) {
                    state.quests.seed(QuestId::NONE, old_quest, old_map);
                    state.quests.seed(QuestId::RANDOM_QUEST10, old_quest, old_map);
                    maps.set_base_legend({ { '?', old_cell } });
                }
                const auto *old_none = seeded ? &list.get_quest(QuestId::NONE) : nullptr;
                QuestFiles files;
                const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
                ANGBAND_DIR_EDIT = files.directory;
                files.write("QuestPreferences.jsonc", { { "legend", base ? nlohmann::json{ { "!", input.data } } : nlohmann::json::object() } });
                auto first = make_quest(1);
                first["definition"]["monster"] = 1;
                first["definition"]["level"] = 0;
                files.write("quests/01.jsonc", first);
                auto second = make_quest(2);
                second["definition"]["level"] = 0;
                if (!base) {
                    second["legend"]["!"] = input.data;
                }
                files.write("quests/02.jsonc", second);
                try {
                    list.initialize();
                    FAIL("Expected an undefined legend reference");
                } catch (const std::runtime_error &error) {
                    const std::string diagnostic(error.what());
                    CHECK(diagnostic.find(base ? "QuestPreferences.jsonc" : "02.jsonc") != std::string::npos);
                    CHECK(diagnostic.find("legend['!']." + input.field + " " + std::to_string(input.id)) != std::string::npos);
                }
                CHECK(list.size() == (seeded ? 2 : 0));
                CHECK_FALSE(list.contains(QuestId::THIEF));
                CHECK_FALSE(list.contains(QuestId::SEWER));
                CHECK_FALSE(maps.find(QuestId::THIEF));
                CHECK_FALSE(maps.find(QuestId::SEWER));
                CHECK(target.misc_flags == old_flags);
                CHECK(maps.get_base_legend().size() == (seeded ? 1 : 0));
                if (seeded) {
                    CHECK(&list.get_quest(QuestId::NONE) == old_none);
                    CHECK(list.get_quest(QuestId::RANDOM_QUEST10).name == "Existing");
                    CHECK(list.get_quest(QuestId::RANDOM_QUEST10).status == QuestStatusType::TAKEN);
                    REQUIRE(maps.find(QuestId::RANDOM_QUEST10));
                    CHECK(maps.find(QuestId::RANDOM_QUEST10)->maps == old_map.maps);
                    CHECK(maps.get_base_legend().at('?').grid.special == 73);
                }
            }
        }
    }
}

TEST_CASE("QuestList preserves legend fixed references and generation parameters")
{
    const std::vector<nlohmann::json> cells{
        { { "ego", 123 } },
        { { "ego", -1 } },
        { { "monster", 1 }, { "object", 1 }, { "ego", 1 }, { "artifact", 1 } },
        { { "monster", -1 } },
        { { "monster", { { "cloneOf", 1 } } } },
        { { "monster", { { "random", true }, { "oodLevel", -32768 } } }, { "object", { { "random", true }, { "oodLevel", 32767 } } },
            { "ego", { { "random", true }, { "id", 123 } } } },
        { { "artifact", { { "random", true }, { "id", 1 } } } },
        { { "artifact", { { "random", true } } } },
        { { "object", { { "questReward", true } } }, { "artifact", { { "questReward", true } } } },
        { { "monster", 0 }, { "object", 0 }, { "ego", 0 }, { "artifact", 0 } },
        { { "terrain", "QUEST_UP" }, { "special", 2 } },
        { { "terrain", "QUEST_ENTER" }, { "special", 2 } },
        { { "terrain", "QUEST_EXIT" }, { "special", 0 } },
        { { "terrain", "UP_STAIR" }, { "special", 2 } },
        { { "terrain", "ENTRANCE" }, { "special", 1 } },
        { { "terrain", "OTHER" }, { "special", -32768 } },
        { { "terrain", "OTHER" }, { "trap", "QUEST_ENTER" }, { "special", 2 } },
        { { "terrain", "OTHER" }, { "trap", "ENTRANCE" }, { "special", 1 } },
        { { "terrain", "QUEST_ENTER" }, { "trap", "OTHER" }, { "special", 2 } },
        // ランダムobjectの分岐では固定trapを配置しないため、元の地形のspecialとして扱う。
        { { "terrain", "OTHER" }, { "trap", "QUEST_ENTER" }, { "object", { { "random", true }, { "oodLevel", 1 } } }, { "special", 48 } },
    };
    for (const auto base : { false, true }) {
        for (const auto &cell : cells) {
            CAPTURE(base);
            CAPTURE(cell);
            LegendReferenceState state;
            QuestFiles files;
            const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
            ANGBAND_DIR_EDIT = files.directory;
            files.write("QuestPreferences.jsonc", { { "legend", base ? nlohmann::json{ { "!", cell } } : nlohmann::json::object() } });
            auto first = make_quest(1);
            first["definition"]["monster"] = 1;
            if (!base) {
                first["legend"]["!"] = cell;
            }
            files.write("quests/01.jsonc", first);
            auto second = make_quest(2);
            second["legend"]["<"] = { { "terrain", "QUEST_UP" }, { "special", 1 } };
            files.write("quests/02.jsonc", second);
            REQUIRE_NOTHROW(QuestList::get_instance().initialize());
            CHECK(QuestList::get_instance().size() == 3);
            CHECK(MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).misc_flags.has(MonsterMiscType::QUESTOR));
            const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
            REQUIRE(map);
            const auto &parsed = base ? QuestFixedMapList::get_instance().get_base_legend().at('!') : map->legend.at('!');
            if (cell.contains("object") && cell["object"].is_object() && cell["object"].contains("oodLevel")) {
                CHECK(parsed.grid.object == cell["object"]["oodLevel"].get<int>());
            }
            if (cell.contains("monster") && cell["monster"].is_object() && cell["monster"].contains("oodLevel")) {
                CHECK(parsed.grid.monster == cell["monster"]["oodLevel"].get<int>());
            }
            if (cell.contains("ego") && cell["ego"].is_object() && cell["ego"].contains("id")) {
                CHECK(enum2i(parsed.grid.ego) == cell["ego"]["id"].get<int>());
            }
            if (cell.contains("ego") && cell["ego"].is_number_integer()) {
                CHECK(enum2i(parsed.grid.ego) == cell["ego"].get<int>());
            }
            if (cell.contains("object") && cell["object"].is_object() && cell["object"].contains("questReward")) {
                CHECK(parsed.object_is_quest_reward);
                CHECK(parsed.artifact_is_quest_reward);
                CHECK_FALSE(QuestList::get_instance().get_quest(QuestId::THIEF).has_reward());
            }
            if (cell.contains("special")) {
                CHECK(parsed.grid.special == cell["special"].get<int>());
            }
        }
    }
}

TEST_CASE("QuestList treats ordinary upstairs special as floor metadata in random quests")
{
    LegendReferenceState state;
    QuestFiles files;
    auto data = make_quest(1);
    data["definition"]["type"] = "RANDOM";
    data["legend"]["<"] = { { "terrain", "UP_STAIR" }, { "special", 48 } };
    files.write("quest.jsonc", data);
    REQUIRE_NOTHROW(state.quests.load(files.directory));
    const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
    REQUIRE(map);
    CHECK(map->legend.at('<').grid.special == 48);
}

TEST_CASE("QuestList reward diagnostics report the JSON array position of an undefined candidate")
{
    test::QuestListTestAccess quests;
    test::MonraceListTestAccess monraces;
    test::ArtifactListTestAccess artifacts;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(0));
    QuestFiles files;
    // 整数でない候補は読み込み時に解析エラーになるため、候補の添字はそのままJSON配列の添字になる。
    for (const auto &candidates : { nlohmann::json::array({ 0, 123 }), nlohmann::json::array({ 0, 0, 0, 123 }) }) {
        auto data = make_quest(1);
        data["definition"]["reward"] = { { "artifacts", candidates } };
        files.write("quest.jsonc", data);
        try {
            quests.load(files.directory);
            FAIL("Expected an undefined reward reference");
        } catch (const std::runtime_error &error) {
            const auto location = "definition.reward.artifacts[" + std::to_string(candidates.size() - 1) + "] 123";
            CHECK(std::string(error.what()).find(location) != std::string::npos);
        }
        check_unpublished(QuestId::THIEF);
    }
}

TEST_CASE("QuestList validates common legends separately for fixed and surface contexts")
{
    for (const auto fixed_context : { false, true }) {
        for (const auto valid : { false, true }) {
            CAPTURE(fixed_context);
            CAPTURE(valid);
            LegendReferenceState state;
            if (!fixed_context) {
                // 表示側はBLDGを優先するが、地上の下り階段はDungeon参照を消費する。
                TerrainList::get_instance().get_terrain(4).flags.set(TerrainCharacteristics::BLDG);
            }
            QuestFiles files;
            const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
            ANGBAND_DIR_EDIT = files.directory;
            files.write("QuestPreferences.jsonc", { { "legend", { { "!", { { "terrain", fixed_context ? "UP_STAIR" : "ENTRANCE" }, { "special", valid ? 1 : 48 } } } } } });
            auto first = make_quest(1);
            first["definition"].update({ { "type", "RANDOM" }, { "level", 1 }, { "monster", 1 } });
            auto second = make_quest(2);
            second["definition"].update({ { "type", fixed_context ? "KILL_ALL" : "RANDOM" }, { "level", fixed_context ? 1 : 0 } });
            files.write("quests/01.jsonc", first);
            files.write("quests/02.jsonc", second);
            auto &list = QuestList::get_instance();
            const auto flags = MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).misc_flags;
            if (valid) {
                REQUIRE_NOTHROW(list.initialize());
                CHECK(list.size() == 3);
            } else {
                CHECK_THROWS_AS(list.initialize(), std::runtime_error);
                CHECK(list.empty());
                CHECK(QuestFixedMapList::get_instance().get_base_legend().empty());
                CHECK(MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).misc_flags == flags);
            }
        }
    }
}

TEST_CASE("QuestList validates every converted terrain candidate without consuming RNG")
{
    for (const auto conversion : { TerrainConversionType::FLOOR, TerrainConversionType::WALL, TerrainConversionType::INNER,
             TerrainConversionType::OUTER, TerrainConversionType::SOLID, TerrainConversionType::STREAM }) {
        for (const auto trap : { false, true }) {
            for (const auto base : { false, true }) {
                for (const auto valid : { false, true }) {
                    CAPTURE(static_cast<int>(conversion));
                    CAPTURE(trap);
                    CAPTURE(base);
                    CAPTURE(valid);
                    LegendReferenceState state;
                    const auto restore_rng = test::scoped_rng();
                    std::array<xso::rng32::word_type, xso::rng32::word_count()> rng_before{};
                    get_game_rng().get_state(rng_before.begin());
                    auto &terrains = TerrainList::get_instance();
                    terrains.resize(8);
                    auto &converted = terrains.get_terrain(7);
                    converted.tag = "CONVERTED";
                    converted.flags.set(TerrainCharacteristics::CONVERT);
                    converted.conversion_type = conversion;
                    converted.stream_index = 0;
                    DungeonDefinition dungeon;
                    dungeon.inner_wall = 2;
                    dungeon.outer_wall = 2;
                    dungeon.prob_table_floor.entry_item(6, 999);
                    dungeon.prob_table_floor.entry_item(2, 1);
                    dungeon.prob_table_wall.entry_item(6, 999);
                    dungeon.prob_table_wall.entry_item(2, 1);
                    dungeon.streams.push_back({ 2, 1, 100, 0 });
                    DungeonList::get_instance().emplace(static_cast<DungeonId>(2), std::move(dungeon));
                    const nlohmann::json cell = trap ? nlohmann::json{ { "terrain", "OTHER" }, { "trap", "CONVERTED" }, { "special", valid ? 2 : 48 } }
                                                     : nlohmann::json{ { "terrain", "CONVERTED" }, { "special", valid ? 2 : 48 } };
                    QuestFiles files;
                    const auto restore_path = test::scoped_restore(ANGBAND_DIR_EDIT);
                    ANGBAND_DIR_EDIT = files.directory;
                    files.write("QuestPreferences.jsonc", { { "legend", base ? nlohmann::json{ { "!", cell } } : nlohmann::json::object() } });
                    auto first = make_quest(1);
                    first["definition"]["monster"] = 1;
                    first["map"] = { "!" };
                    if (!base) {
                        first["legend"]["!"] = cell;
                    }
                    files.write("quests/01.jsonc", first);
                    files.write("quests/02.jsonc", make_quest(2));
                    auto &list = QuestList::get_instance();
                    const auto old_flags = MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).misc_flags;
                    if (valid) {
                        REQUIRE_NOTHROW(list.initialize());
                        CHECK(list.size() == 3);
                    } else {
                        CHECK_THROWS_AS(list.initialize(), std::runtime_error);
                        CHECK(list.empty());
                        CHECK(QuestFixedMapList::get_instance().get_base_legend().empty());
                        CHECK(MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).misc_flags == old_flags);
                    }
                    std::array<xso::rng32::word_type, xso::rng32::word_count()> rng_after{};
                    get_game_rng().get_state(rng_after.begin());
                    CHECK(rng_after == rng_before);
                }
            }
        }
    }
}

TEST_CASE("QuestList preserves converted stream fallback special values")
{
    for (const auto index : { -1, 0, 1 }) {
        LegendReferenceState state;
        auto &terrains = TerrainList::get_instance();
        terrains.resize(8);
        auto &converted = terrains.get_terrain(7);
        converted.tag = "CONVERTED";
        converted.flags.set(TerrainCharacteristics::CONVERT);
        converted.conversion_type = TerrainConversionType::STREAM;
        converted.stream_index = index;
        DungeonDefinition dungeon;
        dungeon.streams.push_back({ 0, 1, 100, 0 });
        DungeonList::get_instance().emplace(static_cast<DungeonId>(2), std::move(dungeon));
        QuestFiles files;
        auto data = make_quest(1);
        data["legend"]["!"] = { { "terrain", "CONVERTED" }, { "special", 48 } };
        files.write("quest.jsonc", data);
        REQUIRE_NOTHROW(state.quests.load(files.directory));
    }
}

TEST_CASE("QuestList validates both reference roles of compound stairs")
{
    for (const auto dungeon_exists : { false, true }) {
        LegendReferenceState state;
        auto &terrain = TerrainList::get_instance().get_terrain(6);
        terrain.flags.set({ TerrainCharacteristics::UP_STAIRS, TerrainCharacteristics::DOWN_STAIRS, TerrainCharacteristics::ENTRANCE });
        if (dungeon_exists) {
            DungeonList::get_instance().emplace(static_cast<DungeonId>(2), DungeonDefinition{});
        }
        QuestFiles files;
        auto data = make_quest(1);
        data["definition"]["level"] = 0;
        data["legend"]["!"] = { { "terrain", "OTHER" }, { "special", 2 } };
        files.write("01.jsonc", data);
        files.write("02.jsonc", make_quest(2));
        if (dungeon_exists) {
            REQUIRE_NOTHROW(state.quests.load(files.directory));
        } else {
            try {
                state.quests.load(files.directory);
                FAIL("Expected the missing Dungeon reference despite a valid Quest reference");
            } catch (const std::runtime_error &error) {
                CHECK(std::string(error.what()).find("special (DungeonId) 2") != std::string::npos);
            }
            check_unpublished(QuestId::THIEF);
        }
    }
    // QUEST_ENTERは下りと対象説明でENTRANCEより優先される。
    LegendReferenceState state;
    TerrainList::get_instance().get_terrain(2).flags.set(TerrainCharacteristics::ENTRANCE);
    QuestFiles files;
    auto data = make_quest(1);
    data["legend"]["!"] = { { "terrain", "QUEST_ENTER" }, { "special", 2 } };
    files.write("01.jsonc", data);
    files.write("02.jsonc", make_quest(2));
    REQUIRE_NOTHROW(state.quests.load(files.directory));
}

TEST_CASE("QuestList validates generation change outcomes and remaining probability")
{
    // 元地形、変化先、確率、special、期待される公開可否。
    struct Input {
        short source;
        short result;
        int probability;
        int special;
        bool valid;
    };
    for (const auto &input : { Input{ 6, 2, 100, 48, false }, Input{ 6, 2, 1, 48, false },
             Input{ 6, 2, 0, 48, true }, Input{ 6, 2, 100, 2, true },
             Input{ 6, 4, 100, 48, false }, Input{ 6, 4, 100, 2, true },
             Input{ 2, 6, 100, 48, true }, Input{ 2, 6, 99, 48, false } }) {
        for (const auto converted : { false, true }) {
            for (const auto trap : { false, true }) {
                CAPTURE(input.source);
                CAPTURE(input.result);
                CAPTURE(input.probability);
                CAPTURE(converted);
                CAPTURE(trap);
                LegendReferenceState state;
                const auto restore_rng = test::scoped_rng();
                std::array<xso::rng32::word_type, xso::rng32::word_count()> before{};
                get_game_rng().get_state(before.begin());
                auto &terrains = TerrainList::get_instance();
                TerrainGenerationChange change;
                change.result = input.result;
                change.probability = input.probability;
                terrains.get_terrain(input.source).generation_changes.push_back(change);
                // 変化先を再帰的に辿らず、生成側と同じ一段の変化にする。
                TerrainGenerationChange next;
                next.result = input.source;
                next.probability = 100;
                terrains.get_terrain(input.result).generation_changes.push_back(next);
                terrains.resize(8);
                auto &feature = terrains.get_terrain(7);
                feature.tag = "CONVERTED";
                feature.flags.set(TerrainCharacteristics::CONVERT);
                feature.conversion_type = TerrainConversionType::FLOOR;
                DungeonDefinition dungeon;
                dungeon.prob_table_floor.entry_item(input.source, 1);
                DungeonList::get_instance().emplace(static_cast<DungeonId>(2), std::move(dungeon));
                const auto tag = converted ? "CONVERTED" : terrains.get_terrain(input.source).tag;
                const nlohmann::json cell = trap ? nlohmann::json{ { "terrain", "OTHER" }, { "trap", tag }, { "special", input.special } }
                                                 : nlohmann::json{ { "terrain", tag }, { "special", input.special } };
                QuestFiles files;
                auto data = make_quest(1);
                data["definition"]["level"] = 0;
                data["legend"]["!"] = cell;
                files.write("01.jsonc", data);
                files.write("02.jsonc", make_quest(2));
                if (input.valid) {
                    REQUIRE_NOTHROW(state.quests.load(files.directory));
                } else {
                    CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
                    check_unpublished(QuestId::THIEF);
                }
                std::array<xso::rng32::word_type, xso::rng32::word_count()> after{};
                get_game_rng().get_state(after.begin());
                CHECK(after == before);
            }
        }
    }
}

TEST_CASE("QuestList follows physical and visible special consumer priorities")
{
    struct Input {
        std::vector<TerrainCharacteristics> flags;
        bool random_quest;
        int level;
        bool valid;
    };
    for (const auto &input : {
             Input{ { TerrainCharacteristics::QUEST }, false, 5, true },
             Input{ { TerrainCharacteristics::QUEST_EXIT }, false, 5, false },
             Input{ { TerrainCharacteristics::QUEST, TerrainCharacteristics::DOWN_STAIRS }, false, 5, false },
             Input{ { TerrainCharacteristics::QUEST_EXIT, TerrainCharacteristics::UP_STAIRS }, true, 5, false },
             Input{ { TerrainCharacteristics::QUEST_EXIT, TerrainCharacteristics::UP_STAIRS }, false, 5, false },
             Input{ { TerrainCharacteristics::QUEST_EXIT, TerrainCharacteristics::STORE }, false, 5, true },
             Input{ { TerrainCharacteristics::QUEST_EXIT, TerrainCharacteristics::BLDG }, false, 5, true },
             Input{ { TerrainCharacteristics::BLDG, TerrainCharacteristics::ENTRANCE }, false, 0, true },
             Input{ { TerrainCharacteristics::BLDG, TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS }, false, 0, false },
             Input{ { TerrainCharacteristics::BLDG, TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS }, false, -1, false },
             Input{ { TerrainCharacteristics::BLDG, TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS }, false, std::numeric_limits<int>::min(), false },
             Input{ { TerrainCharacteristics::BLDG, TerrainCharacteristics::ENTRANCE, TerrainCharacteristics::DOWN_STAIRS }, false, 5, true } }) {
        LegendReferenceState state;
        auto &terrain = TerrainList::get_instance().get_terrain(6);
        for (const auto flag : input.flags) {
            terrain.flags.set(flag);
        }
        QuestFiles files;
        auto data = make_quest(1);
        data["definition"]["level"] = input.level;
        if (input.random_quest) {
            data["definition"]["type"] = "RANDOM";
        }
        data["legend"]["!"] = { { "terrain", "OTHER" }, { "special", 48 } };
        files.write("quest.jsonc", data);
        if (input.valid) {
            REQUIRE_NOTHROW(state.quests.load(files.directory));
        } else {
            CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
            check_unpublished(QuestId::THIEF);
        }
    }
}

TEST_CASE("QuestList validates one visible mimic step and fixed trap underlay")
{
    for (const auto visible : { short{ 2 }, short{ 4 } }) {
        for (const auto trap : { false, true }) {
            for (const auto valid : { false, true }) {
                LegendReferenceState state;
                auto &terrains = TerrainList::get_instance();
                terrains.get_terrain(6).mimic = visible;
                QuestFiles files;
                auto data = make_quest(1);
                nlohmann::json cell{ { "terrain", "OTHER" }, { "special", valid ? 1 : 48 } };
                if (trap) {
                    // trapの実地形は同じOTHERでも、表示は元地形のmimicへ進む。
                    cell["trap"] = "OTHER";
                }
                data["legend"]["!"] = cell;
                files.write("quest.jsonc", data);
                if (valid) {
                    REQUIRE_NOTHROW(state.quests.load(files.directory));
                } else {
                    CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
                    check_unpublished(QuestId::THIEF);
                }
            }
        }
    }
    LegendReferenceState state;
    auto &terrains = TerrainList::get_instance();
    terrains.get_terrain(6).mimic = 2;
    terrains.get_terrain(2).mimic = 4;
    QuestFiles files;
    auto first = make_quest(1);
    first["legend"]["!"] = { { "terrain", "OTHER" }, { "special", 2 } };
    files.write("01.jsonc", first);
    files.write("02.jsonc", make_quest(2));
    // 表示はOTHER->QUEST_ENTERまで。QUEST_ENTER->ENTRANCEは再帰的に辿らない。
    REQUIRE_NOTHROW(state.quests.load(files.directory));
}

TEST_CASE("QuestList validates random trap underlay only on clean floor terrain")
{
    for (const auto floor : { false, true }) {
        LegendReferenceState state;
        auto &terrain = TerrainList::get_instance().get_terrain(6);
        if (floor) {
            terrain.flags.set(TerrainCharacteristics::FLOOR);
        }
        terrain.mimic = 2;
        TerrainGenerationChange change;
        change.result = 0;
        change.probability = 100;
        terrain.generation_changes.push_back(change);
        QuestFiles files;
        auto data = make_quest(1);
        data["legend"]["!"] = { { "terrain", "OTHER" }, { "trap", { { "random", true } } }, { "special", 48 } };
        files.write("quest.jsonc", data);
        if (floor) {
            CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
            check_unpublished(QuestId::THIEF);
        } else {
            REQUIRE_NOTHROW(state.quests.load(files.directory));
        }
    }
}

TEST_CASE("QuestList keeps zero mimic fallback within the same dungeon")
{
    LegendReferenceState state;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(9);
    terrains.get_terrain(0).mimic = 2;
    auto &original = terrains.get_terrain(7);
    original.tag = "CONVERTED_FLOOR";
    original.flags.set(TerrainCharacteristics::CONVERT);
    original.conversion_type = TerrainConversionType::FLOOR;
    auto &trap = terrains.get_terrain(8);
    trap.tag = "CONVERTED_WALL";
    trap.flags.set(TerrainCharacteristics::CONVERT);
    trap.conversion_type = TerrainConversionType::WALL;
    DungeonDefinition first;
    first.prob_table_floor.entry_item(0, 1);
    first.prob_table_wall.entry_item(6, 1);
    DungeonList::get_instance().emplace(static_cast<DungeonId>(2), std::move(first));
    DungeonDefinition second;
    second.prob_table_floor.entry_item(6, 1);
    second.prob_table_wall.entry_item(4, 1);
    DungeonList::get_instance().emplace(static_cast<DungeonId>(3), std::move(second));
    QuestFiles files;
    auto data = make_quest(1);
    data["legend"]["!"] = { { "terrain", "CONVERTED_FLOOR" }, { "trap", "CONVERTED_WALL" }, { "special", 48 } };
    files.write("quest.jsonc", data);
    // Dungeon2ではmimic=0から実地形OTHERへ、Dungeon3では元地形OTHERへ表示する。
    // 別DungeonのENTRANCEや、mimic未指定を表す0のmimicは表示されない。
    REQUIRE_NOTHROW(state.quests.load(files.directory));
}

TEST_CASE("QuestList validates fixed terrain mimic after discovery without terrain changes")
{
    struct Input {
        std::vector<TerrainCharacteristics> flags;
        bool disclosed;
    };
    for (const auto &input : {
             Input{ {}, false },
             Input{ { TerrainCharacteristics::TRAP }, true },
             Input{ { TerrainCharacteristics::DOOR }, true },
             Input{ { TerrainCharacteristics::STAIRS }, true },
             Input{ { TerrainCharacteristics::HAS_GOLD }, true },
             Input{ { TerrainCharacteristics::HIT_TRAP }, true },
             Input{ { TerrainCharacteristics::OPEN }, true },
             Input{ { TerrainCharacteristics::TRAP, TerrainCharacteristics::SECRET }, false },
             Input{ { TerrainCharacteristics::HIT_TRAP, TerrainCharacteristics::BLDG }, false } }) {
        for (const auto valid : { false, true }) {
            LegendReferenceState state;
            auto &terrains = TerrainList::get_instance();
            terrains.resize(8);
            auto &trap = terrains.get_terrain(7);
            trap.tag = "VISIBLE_TRAP";
            trap.mimic = 2;
            for (const auto flag : input.flags) {
                trap.flags.set(flag);
            }
            QuestFiles files;
            auto data = make_quest(1);
            data["legend"]["!"] = { { "terrain", "OTHER" }, { "trap", "VISIBLE_TRAP" }, { "special", valid ? 1 : 48 } };
            files.write("quest.jsonc", data);
            if (valid || !input.disclosed) {
                REQUIRE_NOTHROW(state.quests.load(files.directory));
            } else {
                CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
                check_unpublished(QuestId::THIEF);
            }
        }
    }
}

TEST_CASE("QuestList validates plain up references when reward fallback can change type")
{
    for (const auto &candidates : { nlohmann::json::array(), nlohmann::json::array({ 0 }), nlohmann::json::array({ 1 }) }) {
        for (const auto valid : { false, true }) {
            LegendReferenceState state;
            QuestFiles files;
            auto data = make_quest(1);
            data["definition"]["type"] = "RANDOM";
            data["definition"]["reward"] = { { "artifacts", candidates } };
            data["legend"]["!"] = { { "terrain", "UP_STAIR" }, { "special", valid ? 1 : 48 } };
            files.write("quest.jsonc", data);
            const auto can_fallback = candidates == nlohmann::json::array({ 0 }) || candidates == nlohmann::json::array({ 1 });
            if (valid || !can_fallback) {
                REQUIRE_NOTHROW(state.quests.load(files.directory));
                if (candidates == nlohmann::json::array({ 0 })) {
                    auto &quest = QuestList::get_instance().get_quest(QuestId::THIEF);
                    REQUIRE(quest.type == QuestKindType::RANDOM);
                    quest.status = QuestStatusType::TAKEN;
                    const auto map = QuestFixedMapList::get_instance().find(QuestId::THIEF);
                    REQUIRE(map);
                    resolve_quest_reward(*map, quest);
                    CHECK(quest.type == QuestKindType::KILL_ALL);
                    apply_quest_metadata(*map, quest);
                    CHECK(quest.type == QuestKindType::KILL_ALL);
                }
            } else {
                CHECK_THROWS_AS(state.quests.load(files.directory), std::runtime_error);
                check_unpublished(QuestId::THIEF);
            }
        }
    }
}
