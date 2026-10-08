#include "load/load-util.h"
#include "load/world-loader.h"
#include "save/player-writer.h"
#include "save/save-util.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/player-type-definition.h"
#include "system/services/dungeon-service.h"
#include "test/save/scoped-save-io.h"
#include "test/system/dungeon-list-test-access.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <string>
#include <utility>
#include <vector>

namespace {
// Called after isolating the singleton with DungeonListTestAccess.
void ensure_dungeon_definitions()
{
    auto &dungeons = DungeonList::get_instance();
    for (const auto id : DUNGEON_IDS) {
        if (!dungeons.contains(id)) {
            DungeonDefinition definition;
            definition.name = "Dungeon-" + std::to_string(enum2i(id));
            definition.mindepth = 1;
            definition.maxdepth = 99;
            dungeons.emplace(id, std::move(definition));
        }
    }
}

auto preserve_records()
{
    auto &records = DungeonRecords::get_instance();
    std::vector<std::pair<DungeonId, DungeonRecord>> saved;
    for (const auto &[id, record] : records) {
        saved.emplace_back(id, *record);
    }
    return util::make_finalizer([saved = std::move(saved)] {
        for (const auto &[id, record] : saved) {
            DungeonRecords::get_instance().get_record(id) = record;
        }
    });
}

auto preserve_save_versions()
{
    return util::make_finalizer([version = loading_savefile_version, game_version = AngbandSystem::get_instance().get_version()] {
        loading_savefile_version = version;
        AngbandSystem::get_instance().set_version(game_version);
    });
}
}

TEST_CASE("DungeonRecord distinguishes unlocked recall from a visit")
{
    DungeonRecord record;
    CHECK_FALSE(record.has_entered());
    CHECK_FALSE(record.has_visited());

    record.set_max_level(20);
    CHECK(record.has_entered());
    CHECK_FALSE(record.has_visited());
    CHECK(record.get_max_level() == 20);

    // Visiting the unlocked depth must not require a new maximum depth.
    record.mark_visited();
    CHECK(record.has_visited());
    CHECK(record.get_max_level() == 20);
}

TEST_CASE("DungeonRecord preserves visits when recall depth changes")
{
    DungeonRecord record;
    record.set_max_level(30);
    record.mark_visited();
    record.set_max_level(10);
    CHECK(record.has_visited());
    CHECK(record.get_max_level() == 10);
    CHECK(record.get_max_max_level() == 30);
}

TEST_CASE("DungeonRecord reset clears both recall availability and visits")
{
    DungeonRecord record;
    record.set_max_level(20);
    record.mark_visited();
    record.reset();
    CHECK_FALSE(record.has_entered());
    CHECK_FALSE(record.has_visited());
    CHECK(record.get_max_level() == 0);
    CHECK(record.get_max_max_level() == 0);
    record.set_max_level(20);
    CHECK_FALSE(record.has_visited());
}

TEST_CASE("Dungeon recall save round trip preserves unlocked-only and visited records")
{
    const test::DungeonListTestAccess restore_dungeons;
    ensure_dungeon_definitions();
    const auto restore_records = preserve_records();
    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    const auto restore_versions = preserve_save_versions();
    auto &records = DungeonRecords::get_instance();
    records.reset_all();
    records.get_record(DungeonId::ANGBAND).set_max_level(20);
    records.get_record(DungeonId::ANGBAND).mark_visited();
    records.get_record(DungeonId::GALGALS).set_max_level(30);
    saving_savefile = file;
    save_xor_byte = 0;
    v_stamp = x_stamp = 0;
    wr_dungeons();
    wr_u32b(0x12345678);
    const auto expected_v = v_stamp;
    const auto expected_x = x_stamp;
    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    loading_savefile_version = SAVEFILE_VERSION;
    AngbandSystem::get_instance().set_version({ 3, 0, 2, 4 });
    // Deliberately stale state makes a missing reset in the loader observable.
    for (auto &[_, record] : records) {
        record->set_max_level(50);
        record->mark_visited();
    }
    PlayerType player;
    rd_dungeons(&player);
    CHECK(records.get_record(DungeonId::ANGBAND).has_visited());
    CHECK(records.get_record(DungeonId::ANGBAND).get_max_level() == 20);
    CHECK_FALSE(records.get_record(DungeonId::GALGALS).has_visited());
    CHECK(records.get_record(DungeonId::GALGALS).get_max_level() == 30);
    CHECK_FALSE(records.get_record(DungeonId::ORC).has_entered());
    CHECK_FALSE(records.get_record(DungeonId::ORC).has_visited());
    CHECK(rd_u32b() == 0x12345678);
    CHECK(v_check == expected_v);
    CHECK(x_check == expected_x);
}

TEST_CASE("Dungeon recall loader accepts legacy depths without consuming visit flags")
{
    const test::DungeonListTestAccess restore_dungeons;
    ensure_dungeon_definitions();
    const auto restore_records = preserve_records();
    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    const auto restore_versions = preserve_save_versions();
    saving_savefile = file;
    save_xor_byte = 0;
    wr_byte(static_cast<uint8_t>(DungeonRecords::get_instance().size()));
    for (const auto id : DUNGEON_IDS) {
        wr_s16b(id == DungeonId::ANGBAND ? 20 : 0);
    }
    wr_u32b(0x12345678);
    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    loading_savefile_version = 26;
    AngbandSystem::get_instance().set_version({ 3, 0, 2, 4 });
    DungeonRecords::get_instance().reset_all();
    PlayerType player;
    rd_dungeons(&player);
    const auto &record = DungeonRecords::get_instance().get_record(DungeonId::ANGBAND);
    CHECK(record.get_max_level() == 20);
    CHECK(record.has_visited());
    CHECK_FALSE(DungeonRecords::get_instance().get_record(DungeonId::GALGALS).has_entered());
    CHECK(rd_u32b() == 0x12345678);
}

TEST_CASE("Dungeon recall loader clamps depths and consumes unsupported records completely")
{
    const test::DungeonListTestAccess restore_dungeons;
    auto &dungeons = DungeonList::get_instance();
    for (const auto id : { DungeonId::WILDERNESS, DungeonId::ANGBAND }) {
        DungeonDefinition definition;
        definition.maxdepth = 99;
        dungeons.emplace(id, std::move(definition));
    }
    const auto restore_records = preserve_records();
    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    const auto restore_versions = preserve_save_versions();
    auto &records = DungeonRecords::get_instance();
    records.reset_all();
    records.get_record(DungeonId::ANGBAND).set_max_level(200);
    records.get_record(DungeonId::ANGBAND).mark_visited();
    // The serialized records outnumber the definitions available to the loader.
    records.get_record(DungeonId::GALGALS).set_max_level(30);
    records.get_record(DungeonId::GALGALS).mark_visited();
    saving_savefile = file;
    save_xor_byte = 0;
    wr_dungeons();
    wr_u32b(0x12345678);
    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    loading_savefile_version = SAVEFILE_VERSION;
    AngbandSystem::get_instance().set_version({ 3, 0, 2, 5 });
    records.reset_all();
    PlayerType player;
    rd_dungeons(&player);
    CHECK(records.get_record(DungeonId::ANGBAND).get_max_level() == 99);
    CHECK(records.get_record(DungeonId::ANGBAND).has_visited());
    CHECK_FALSE(records.get_record(DungeonId::GALGALS).has_entered());
    CHECK_FALSE(records.get_record(DungeonId::GALGALS).has_visited());
    CHECK(rd_u32b() == 0x12345678);
}

TEST_CASE("Recall dump partitions known dungeons without changing recall selection")
{
    const test::DungeonListTestAccess restore_dungeons;
    ensure_dungeon_definitions();
    const auto restore_records = preserve_records();
    auto &records = DungeonRecords::get_instance();
    records.reset_all();
    records.get_record(DungeonId::ANGBAND).set_max_level(20);
    records.get_record(DungeonId::ANGBAND).mark_visited();
    // An unlock at the bottom floor alone must not display a conquered marker.
    const auto &unvisited_dungeon = DungeonList::get_instance().get_dungeon(DungeonId::GALGALS);
    records.get_record(DungeonId::GALGALS).set_max_level(unvisited_dungeon.maxdepth);
    const auto all = DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP);
    const auto visited = DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, true);
    const auto unlocked = DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, false);
    REQUIRE(all.size() == 2);
    REQUIRE(visited.size() == 1);
    REQUIRE(unlocked.size() == 1);
    CHECK(visited.front() == all[0]);
    CHECK(unlocked.front() == all[1]);
    CHECK(visited.front().find(DungeonList::get_instance().get_dungeon(DungeonId::ANGBAND).name) != std::string::npos);
    CHECK(unlocked.front().find(unvisited_dungeon.name) != std::string::npos);
    CHECK(unlocked.front().find('!') == std::string::npos);
    const auto recall = DungeonService::build_known_dungeons(DungeonMessageFormat::RECALL);
    REQUIRE(recall.size() == 2);
    CHECK(recall[0].find("a)") != std::string::npos);
    CHECK(recall[1].find("b)") != std::string::npos);
    records.get_record(DungeonId::GALGALS).mark_visited();
    CHECK(DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, true).size() == 2);
    CHECK(DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, false).empty());
    records.reset_all();
    CHECK(DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, true).empty());
    CHECK(DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP, false).empty());
}
