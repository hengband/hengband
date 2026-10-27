#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "monster/monster-util.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/dungeon-list-test-access.h"
#include "test/system/monrace-list-test-access.h"
#include "view/display-messages.h"
#include <algorithm>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
class DungeonReaderStateGuard {
public:
    DungeonReaderStateGuard()
        : terrains(TerrainList::get_instance())
        , previous_terrains(terrains.begin(), terrains.end())
    {
        terrains.resize(2);
        terrains.get_terrain(0).tag = "FLOOR";
        terrains.get_terrain(1).tag = "GRANITE";
    }

    DungeonReaderStateGuard(const DungeonReaderStateGuard &) = delete;
    DungeonReaderStateGuard &operator=(const DungeonReaderStateGuard &) = delete;

    ~DungeonReaderStateGuard()
    {
        terrains.resize(previous_terrains.size());
        std::copy(previous_terrains.begin(), previous_terrains.end(), terrains.begin());
    }

private:
    test::ScopedReaderState reader_state;
    test::DungeonListTestAccess dungeon_state;
    TerrainList &terrains;
    std::vector<TerrainType> previous_terrains;
};

struct FinalFloorStateGuard {
    // Monraceのfixtureはsession.last_id().value_or(-1)も変更するため、Dungeonのfixtureより先に置く。
    test::MonraceListTestAccess races;
    test::ArtifactListTestAccess artifacts;
    DungeonReaderStateGuard dungeon;

    FinalFloorStateGuard()
    {
        MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
        ArtifactList::get_instance().emplace(FixedArtifactId::GALADRIEL_PHIAL, {});
    }

    FinalFloorStateGuard(const FinalFloorStateGuard &) = delete;
    FinalFloorStateGuard(FinalFloorStateGuard &&) = delete;
    FinalFloorStateGuard &operator=(const FinalFloorStateGuard &) = delete;
    FinalFloorStateGuard &operator=(FinalFloorStateGuard &&) = delete;
};

nlohmann::json make_dungeon(int id = 0)
{
    return {
        { "id", id },
        { "name", { { "ja", "test dungeon" }, { "en", "test dungeon" } } },
        { "position", { { "wild_y", 0 }, { "wild_x", 0 } } },
        { "generation", { { "minDepth", 0 }, { "maxDepth", 0 }, { "minPlayerLevel", 0 }, { "objGood", 0 }, { "objGreat", 0 }, { "pit", nlohmann::json::array() }, { "nest", nlohmann::json::array() } } },
        { "floor", { { "tiles", { { { "type", "FLOOR" }, { "rate", 100 } } } }, { "tunnelRate", 50 } } },
        { "wall", { { "tiles", { { { "type", "GRANITE" }, { "rate", 100 } } } }, { "outer", "GRANITE" }, { "inner", "GRANITE" } } },
        { "monsters", { { "minCount", 14 }, { "additionalSpawnProbability", 6250 }, { "normalMonsterRate", 100 }, { "flagsMode", "NONE" } } },
    };
}

const ProbabilityTable<short> &terrain_table(const DungeonDefinition &dungeon, const std::string &section)
{
    return section == "floor" ? dungeon.prob_table_floor : dungeon.prob_table_wall;
}
}

TEST_CASE("DungeonReader reads floor and wall probabilities")
{
    auto session = DungeonReader::create_session();
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    const auto data = make_dungeon();
    REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
    const auto &dungeons = DungeonList::get_instance();
    REQUIRE(dungeons.size() == 1);
    const auto &dungeon = dungeons.get_dungeon(DungeonId::WILDERNESS);
    CHECK(dungeon.name == "test dungeon");
    CHECK(dungeon.prob_table_floor.item_count() == 1);
    CHECK(dungeon.prob_table_floor.total_prob() == 100);
    CHECK(dungeon.prob_table_wall.item_count() == 1);
    CHECK(dungeon.prob_table_wall.total_prob() == 100);
}

TEST_CASE("DungeonReader stores every supported aura token")
{
    auto session = DungeonReader::create_session();
    for (const auto &[token, aura] : r_info_aura_flags) {
        CAPTURE(token);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["monsters"]["flags"] = { std::string(token) };
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.mon_aura_flags.has(aura));
        CHECK(dungeon.mon_aura_flags.count() == 1);
    }
}

TEST_CASE("DungeonReader rejects unknown aura tokens without publishing a dungeon")
{
    auto session = DungeonReader::create_session();
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    auto data = make_dungeon();
    data["monsters"]["flags"] = { "AURA_FIRE", "AURA_UNKNOWN" };
    CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_FLAG);
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader aura restrictions participate in every monster filter mode")
{
    auto session = DungeonReader::create_session();
    for (const auto *mode : { "NONE", "AND", "NAND", "OR", "NOR" }) {
        for (auto aura_mask = 0; aura_mask < 4; ++aura_mask) {
            for (const auto resistant : { false, true }) {
                CAPTURE(mode);
                CAPTURE(aura_mask);
                CAPTURE(resistant);
                const test::MonraceListTestAccess races;
                DungeonReaderStateGuard guard;
                session = DungeonReader::create_session();
                auto &monrace = MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
                if ((aura_mask & 1) != 0) {
                    monrace.aura_flags.set(MonsterAuraType::FIRE);
                }
                if ((aura_mask & 2) != 0) {
                    monrace.aura_flags.set(MonsterAuraType::COLD);
                }
                if (resistant) {
                    monrace.resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
                }
                auto data = make_dungeon();
                data["monsters"]["normalMonsterRate"] = 0;
                data["monsters"]["flagsMode"] = mode;
                data["monsters"]["flags"] = { "AURA_FIRE", "AURA_COLD", "IM_FIRE" };
                REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
                const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
                const auto all = aura_mask == 3 && resistant;
                const auto any = aura_mask != 0 || resistant;
                auto expected = true;
                switch (dungeon.mode) {
                case DungeonMode::NONE:
                    break;
                case DungeonMode::AND:
                    expected = all;
                    break;
                case DungeonMode::NAND:
                    expected = !all;
                    break;
                case DungeonMode::OR:
                    expected = any;
                    break;
                case DungeonMode::NOR:
                    expected = !any;
                    break;
                }
                CHECK(restrict_monster_to_dungeon(dungeon, 0, MonraceId::FILTHY_URCHIN) == expected);
            }
        }
    }
}

TEST_CASE("DungeonReader visual restrictions participate in every monster filter mode")
{
    auto session = DungeonReader::create_session();
    for (const auto *mode : { "NONE", "AND", "NAND", "OR", "NOR" }) {
        for (auto visual_mask = 0; visual_mask < 4; ++visual_mask) {
            for (const auto resistant : { false, true }) {
                CAPTURE(mode);
                CAPTURE(visual_mask);
                CAPTURE(resistant);
                const test::MonraceListTestAccess races;
                DungeonReaderStateGuard guard;
                session = DungeonReader::create_session();
                auto &monrace = MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
                if ((visual_mask & 1) != 0) {
                    monrace.visual_flags.set(MonsterVisualType::CLEAR);
                }
                if ((visual_mask & 2) != 0) {
                    monrace.visual_flags.set(MonsterVisualType::MULTI_COLOR);
                }
                if (resistant) {
                    monrace.resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
                }
                auto data = make_dungeon();
                data["monsters"]["normalMonsterRate"] = 0;
                data["monsters"]["flagsMode"] = mode;
                data["monsters"]["flags"] = { "CHAR_CLEAR", "ATTR_MULTI", "IM_FIRE" };
                REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
                const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
                const auto all = visual_mask == 3 && resistant;
                const auto any = visual_mask != 0 || resistant;
                auto expected = true;
                switch (dungeon.mode) {
                case DungeonMode::NONE:
                    break;
                case DungeonMode::AND:
                    expected = all;
                    break;
                case DungeonMode::NAND:
                    expected = !all;
                    break;
                case DungeonMode::OR:
                    expected = any;
                    break;
                case DungeonMode::NOR:
                    expected = !any;
                    break;
                }
                CHECK(restrict_monster_to_dungeon(dungeon, 0, MonraceId::FILTHY_URCHIN) == expected);
            }
        }
    }
}

TEST_CASE("DungeonReader special restrictions participate in every monster filter mode")
{
    auto session = DungeonReader::create_session();
    for (const auto *mode : { "NONE", "AND", "NAND", "OR", "NOR" }) {
        for (auto special_mask = 0; special_mask < 2; ++special_mask) {
            for (const auto resistant : { false, true }) {
                CAPTURE(mode);
                CAPTURE(special_mask);
                CAPTURE(resistant);
                const test::MonraceListTestAccess races;
                DungeonReaderStateGuard guard;
                session = DungeonReader::create_session();
                auto &monrace = MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
                if ((special_mask & 1) != 0) {
                    monrace.special_flags.set(MonsterSpecialType::DIMINISH_MAX_DAMAGE);
                }
                if (resistant) {
                    monrace.resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
                }
                auto data = make_dungeon();
                data["monsters"]["normalMonsterRate"] = 0;
                data["monsters"]["flagsMode"] = mode;
                data["monsters"]["flags"] = { "DIMINISH_MAX_DAMAGE", "IM_FIRE" };
                REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
                const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
                const auto all = special_mask == 1 && resistant;
                const auto any = special_mask != 0 || resistant;
                auto expected = true;
                switch (dungeon.mode) {
                case DungeonMode::NONE:
                    break;
                case DungeonMode::AND:
                    expected = all;
                    break;
                case DungeonMode::NAND:
                    expected = !all;
                    break;
                case DungeonMode::OR:
                    expected = any;
                    break;
                case DungeonMode::NOR:
                    expected = !any;
                    break;
                }
                CHECK(restrict_monster_to_dungeon(dungeon, 0, MonraceId::FILTHY_URCHIN) == expected);
            }
        }
    }
}

TEST_CASE("DungeonReader stores every supported sex token")
{
    for (const auto &[token, sex] : r_info_sex) {
        if (sex == MonsterSex::NONE) {
            continue;
        }
        CAPTURE(token);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["monsters"]["flags"] = { std::string(token) };
        auto session = DungeonReader::create_session();
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.mon_sex.has(sex));
        CHECK(dungeon.mon_sex.count() == 1);
    }
}

TEST_CASE("DungeonReader accumulates sex tokens independently of order and duplicates")
{
    for (const auto &tokens : { std::vector<std::string>{ "MALE", "FEMALE", "MALE" }, std::vector<std::string>{ "FEMALE", "MALE", "FEMALE" } }) {
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["monsters"]["flags"] = tokens;
        auto session = DungeonReader::create_session();
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.mon_sex.has(MonsterSex::MALE));
        CHECK(dungeon.mon_sex.has(MonsterSex::FEMALE));
        CHECK(dungeon.mon_sex.count() == 2);
    }
}

TEST_CASE("DungeonReader rejects unknown sex tokens without publishing a dungeon")
{
    for (const auto *token : { "SEX_UNKNOWN", "NONE" }) {
        CAPTURE(token);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["monsters"]["flags"] = { "MALE", token };
        auto session = DungeonReader::create_session();
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_FLAG);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader sex restrictions participate in every monster filter mode")
{
    for (const auto *mode : { "NONE", "AND", "NAND", "OR", "NOR" }) {
        for (auto sex_mask = 0; sex_mask < 4; ++sex_mask) {
            for (const auto sex : { MonsterSex::NONE, MonsterSex::MALE, MonsterSex::FEMALE }) {
                for (const auto require_resistance : { false, true }) {
                    for (const auto resistant : { false, true }) {
                        CAPTURE(mode);
                        CAPTURE(sex_mask);
                        CAPTURE(static_cast<int>(sex));
                        CAPTURE(require_resistance);
                        CAPTURE(resistant);
                        const test::MonraceListTestAccess races;
                        DungeonReaderStateGuard guard;
                        auto &monrace = MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
                        monrace.init_sex(static_cast<uint32_t>(sex));
                        if (resistant) {
                            monrace.resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
                        }
                        auto data = make_dungeon();
                        data["monsters"]["normalMonsterRate"] = 0;
                        data["monsters"]["flagsMode"] = mode;
                        data["monsters"]["flags"] = nlohmann::json::array();
                        if ((sex_mask & 1) != 0) {
                            data["monsters"]["flags"].push_back("MALE");
                        }
                        if ((sex_mask & 2) != 0) {
                            data["monsters"]["flags"].push_back("FEMALE");
                        }
                        if (require_resistance) {
                            data["monsters"]["flags"].push_back("IM_FIRE");
                        }
                        auto session = DungeonReader::create_session();
                        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
                        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
                        CHECK(dungeon.mon_sex.count() == static_cast<size_t>(((sex_mask & 1) != 0) + ((sex_mask & 2) != 0)));
                        const auto male_matches = sex == MonsterSex::MALE;
                        const auto female_matches = sex == MonsterSex::FEMALE;
                        const auto all = ((sex_mask & 1) == 0 || male_matches) && ((sex_mask & 2) == 0 || female_matches) && (!require_resistance || resistant);
                        const auto any = ((sex_mask & 1) != 0 && male_matches) || ((sex_mask & 2) != 0 && female_matches) || (require_resistance && resistant);
                        auto expected = true;
                        switch (dungeon.mode) {
                        case DungeonMode::NONE:
                            break;
                        case DungeonMode::AND:
                            expected = all;
                            break;
                        case DungeonMode::NAND:
                            expected = !all;
                            break;
                        case DungeonMode::OR:
                            expected = any;
                            break;
                        case DungeonMode::NOR:
                            expected = !any;
                            break;
                        }
                        CHECK(restrict_monster_to_dungeon(dungeon, 0, MonraceId::FILTHY_URCHIN) == expected);
                    }
                }
            }
        }
    }
}

TEST_CASE("DungeonReader rejects player levels outside their storage type")
{
    auto session = DungeonReader::create_session();
    for (const auto value : { -32769, 65537 }) {
        CAPTURE(value);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["generation"]["minPlayerLevel"] = value;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader preserves item chance cap boundaries")
{
    auto session = DungeonReader::create_session();
    for (const auto *field : { "objGood", "objGreat" }) {
        for (const auto number : { 0, 100 }) {
            CAPTURE(std::string(field));
            CAPTURE(number);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data["generation"][field] = number;
            REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
            const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
            CHECK(dungeon.obj_good == (std::string(field) == "objGood" ? number : 0));
            CHECK(dungeon.obj_great == (std::string(field) == "objGreat" ? number : 0));
        }
    }
}

TEST_CASE("DungeonReader rejects item chance caps outside zero to one hundred before publishing")
{
    auto session = DungeonReader::create_session();
    for (const auto *field : { "objGood", "objGreat" }) {
        for (const auto number : { -1, 101, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(number);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            const auto original = make_dungeon();
            REQUIRE(test::read_and_commit(DungeonReader(original, session), session).has_value());
            auto data = make_dungeon(1);
            data["name"] = { { "ja", "changed" }, { "en", "changed" } };
            data["generation"][field] = number;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            REQUIRE(DungeonList::get_instance().size() == 1);
            const auto &previous = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
            CHECK(previous.name == "test dungeon");
            CHECK(previous.obj_good == 0);
            CHECK(previous.obj_great == 0);
        }
    }
}

TEST_CASE("DungeonReader preserves monster count boundaries")
{
    auto session = DungeonReader::create_session();
    for (const auto number : { 0, 100 }) {
        CAPTURE(number);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["monsters"]["minCount"] = number;
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_monster_count_on_floor == number);
    }
}

TEST_CASE("DungeonReader rejects monster counts outside zero to one hundred before publishing")
{
    auto session = DungeonReader::create_session();
    for (const auto number : { -1, 101, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
        CAPTURE(number);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        const auto original = make_dungeon();
        REQUIRE(test::read_and_commit(DungeonReader(original, session), session).has_value());
        auto data = make_dungeon(1);
        data["monsters"]["minCount"] = number;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        REQUIRE(DungeonList::get_instance().size() == 1);
        CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_monster_count_on_floor == 14);
    }
}

TEST_CASE("DungeonReader preserves signed short depth bounds and deep dungeons")
{
    auto session = DungeonReader::create_session();
    for (const auto depth : { 0, 696, 32767 }) {
        CAPTURE(depth);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["generation"]["minDepth"] = depth;
        data["generation"]["maxDepth"] = depth;
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.mindepth == depth);
        CHECK(dungeon.maxdepth == depth);
    }
}

TEST_CASE("DungeonReader rejects depths outside save range and inverted depths")
{
    auto session = DungeonReader::create_session();
    for (const auto *field : { "minDepth", "maxDepth" }) {
        for (const auto depth : { -1, 32768, std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(depth);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data["generation"][field] = depth;
            ScopedMessageCapture diagnostics;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            CHECK(DungeonList::get_instance().empty());
            // 範囲外の深さは逆転の判定より前に拒否し、逆転の診断を出さない
            CHECK(diagnostics.messages().empty());
        }
    }
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    auto data = make_dungeon();
    data["generation"]["minDepth"] = 100;
    data["generation"]["maxDepth"] = 99;
    ScopedMessageCapture diagnostics;
    CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
    CHECK(DungeonList::get_instance().empty());
    CHECK(diagnostics.messages().size() == 1);
}

TEST_CASE("DungeonReader restricts required player level to playable levels")
{
    auto session = DungeonReader::create_session();
    for (const auto level : { -1, 0, 50, 51, 32767 }) {
        CAPTURE(level);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["generation"]["minPlayerLevel"] = level;
        if (level >= 0 && level <= 50) {
            REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
            CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_plev == level);
        } else {
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader rejects coordinates outside wilderness storage and IDs outside the dungeon enum")
{
    auto session = DungeonReader::create_session();
    for (const auto *field : { "wild_y", "wild_x" }) {
        for (const auto coordinate : { -1, 32767, std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(coordinate);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data["position"][field] = coordinate;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            CHECK(DungeonList::get_instance().empty());
        }
    }
    for (const auto id : { -1, 21, 9999 }) {
        CAPTURE(id);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        const auto data = make_dungeon(id);
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader preserves nonpercentage tunnel thresholds within the depth range")
{
    auto session = DungeonReader::create_session();
    for (const auto threshold : { -1, 0, 1000, 32767, 32768 }) {
        CAPTURE(threshold);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["floor"]["tunnelRate"] = threshold;
        if (threshold >= 0 && threshold <= 32767) {
            REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
            CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).tunnel_percent == threshold);
        } else {
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader validates entrances against actual wilderness dimensions without changing definitions")
{
    auto session = DungeonReader::create_session();
    // 3×4の荒野で端を除いた内側は y=1・x=1〜2 だけ。
    for (const auto &[y, x] : { std::pair{ 1, 1 }, std::pair{ 1, 2 }, std::pair{ 0, 1 }, std::pair{ 2, 1 }, std::pair{ 1, 0 }, std::pair{ 1, 3 }, std::pair{ 3, 0 }, std::pair{ 0, 4 } }) {
        CAPTURE(y);
        CAPTURE(x);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon(1);
        data["position"] = { { "wild_y", y }, { "wild_x", x } };
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto previous = DungeonList::get_instance().get_dungeon_shared(DungeonId::ANGBAND);
        const auto inside = (y == 1) && (x == 1 || x == 2);
        CHECK(test::to_error_code(DungeonReader::validate_wilderness_positions(3, 4)) == (inside ? ParseErrorType::NONE : ParseErrorType::OUT_OF_BOUNDS));
        CHECK(DungeonList::get_instance().get_dungeon_shared(DungeonId::ANGBAND) == previous);
        CHECK(previous->get_position() == Pos2D(y, x));
    }
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    auto data = make_dungeon();
    data["position"] = { { "wild_y", 32766 }, { "wild_x", 32766 } };
    REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
    CHECK(DungeonReader::validate_wilderness_positions(3, 4).has_value());
}

TEST_CASE("DungeonReader accepts percentage weight boundaries and excludes zero weight entries")
{
    auto session = DungeonReader::create_session();
    struct AcceptedTiles {
        nlohmann::json tiles;
        int item_count;
    };
    const std::vector<AcceptedTiles> accepted_tiles = {
        { { { { "type", "FLOOR" }, { "rate", 0 } }, { { "type", "GRANITE" }, { "rate", 100 } } }, 1 },
        { { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 0 } } }, 1 },
        { { { { "type", "FLOOR" }, { "rate", 1 } }, { { "type", "GRANITE" }, { "rate", 99 } } }, 2 },
        { { { { "type", "FLOOR" }, { "rate", 20 } }, { { "type", "GRANITE" }, { "rate", 30 } }, { { "type", "FLOOR" }, { "rate", 50 } } }, 3 },
    };
    for (const std::string section : { "floor", "wall" }) {
        for (const auto &accepted : accepted_tiles) {
            CAPTURE(section);
            CAPTURE(accepted.tiles);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data[section]["tiles"] = accepted.tiles;
            REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
            const auto &table = terrain_table(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS), section);
            CHECK(table.item_count() == accepted.item_count);
            CHECK(table.total_prob() == 100);
        }
    }
}

TEST_CASE("DungeonReader rejects invalid weights without publishing a dungeon")
{
    auto session = DungeonReader::create_session();
    // 範囲外の重みは不正な値、整数でない重みは不正な型、重みが無ければ引数不足とする
    const std::vector<nlohmann::json> invalid_rates = {
        -1,
        101,
        150,
        32768,
        65537,
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<uint64_t>::max(),
        1.5,
        "1",
        true,
        nullptr,
    };
    const std::vector<ParseErrorType> expected_errors = {
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::TOO_FEW_ARGUMENTS,
    };
    REQUIRE(invalid_rates.size() == expected_errors.size());
    for (const std::string section : { "floor", "wall" }) {
        for (size_t i = 0; i < invalid_rates.size(); ++i) {
            const auto &rate = invalid_rates[i];
            const auto expected = expected_errors[i];
            CAPTURE(section);
            CAPTURE(rate);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data[section]["tiles"][0]["rate"] = rate;
            ScopedMessageCapture diagnostics;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == expected);
            CHECK(DungeonList::get_instance().empty());
            if (expected == ParseErrorType::INVALID_VALUE) {
                // 合計の検証を先に行っても同じ種類になるため、項目だけに出る診断を確かめる。
                REQUIRE(diagnostics.messages().size() == 1);
                CHECK(diagnostics.messages().front().find("rate '") != std::string::npos);
            }
        }
    }
}

TEST_CASE("DungeonReader rejects unusable or malformed terrain tables")
{
    auto session = DungeonReader::create_session();
    // 未知の地形だけを未定義地形タグとし、書式の誤りは欠けているか型が違うかで区別する
    const std::vector<nlohmann::json> invalid_tiles = {
        { { { "type", "FLOOR" } } },
        { { { "type", "UNKNOWN" }, { "rate", 1 } } },
        { { { "type", "UNKNOWN" }, { "rate", 0 } } },
        { { { "type", 1 }, { "rate", 1 } } },
        { 1 },
        nullptr,
        "tiles",
        nlohmann::json::object(),
    };
    const std::vector<ParseErrorType> expected_errors = {
        ParseErrorType::TOO_FEW_ARGUMENTS,
        ParseErrorType::UNDEFINED_TERRAIN_TAG,
        ParseErrorType::UNDEFINED_TERRAIN_TAG,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::TOO_FEW_ARGUMENTS,
        ParseErrorType::INVALID_TYPE,
        ParseErrorType::INVALID_TYPE,
    };
    REQUIRE(invalid_tiles.size() == expected_errors.size());
    for (const std::string section : { "floor", "wall" }) {
        for (size_t i = 0; i < invalid_tiles.size(); ++i) {
            const auto &tiles = invalid_tiles[i];
            const auto expected = expected_errors[i];
            CAPTURE(section);
            CAPTURE(tiles);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data[section]["tiles"] = tiles;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == expected);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader distinguishes missing mistyped and unknown wall terrain tags")
{
    auto session = DungeonReader::create_session();
    for (const auto *key : { "outer", "inner" }) {
        CAPTURE(key);
        // nlohmann::json の一時オブジェクトを波括弧の初期化子リストに並べると MSVC が C4868 を出すため、1 件ずつ足す。
        std::vector<std::pair<nlohmann::json, ParseErrorType>> invalid_tags;
        invalid_tags.emplace_back(nullptr, ParseErrorType::TOO_FEW_ARGUMENTS);
        invalid_tags.emplace_back(1, ParseErrorType::INVALID_TYPE);
        invalid_tags.emplace_back(1.5, ParseErrorType::INVALID_TYPE);
        invalid_tags.emplace_back(true, ParseErrorType::INVALID_TYPE);
        invalid_tags.emplace_back(nlohmann::json::array(), ParseErrorType::INVALID_TYPE);
        invalid_tags.emplace_back(nlohmann::json::object(), ParseErrorType::INVALID_TYPE);
        invalid_tags.emplace_back("", ParseErrorType::UNDEFINED_TERRAIN_TAG);
        invalid_tags.emplace_back("UNKNOWN", ParseErrorType::UNDEFINED_TERRAIN_TAG);
        for (const auto &[tag, expected] : invalid_tags) {
            CAPTURE(tag);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data["wall"][key] = tag;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == expected);
            CHECK(DungeonList::get_instance().empty());
        }
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["wall"].erase(key);
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader does not change published dungeons after a terrain table error")
{
    auto session = DungeonReader::create_session();
    for (const std::string section : { "floor", "wall" }) {
        CAPTURE(section);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto &dungeons = DungeonList::get_instance();
        const auto original = make_dungeon();
        REQUIRE(test::read_and_commit(DungeonReader(original, session), session).has_value());
        const auto before = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
        auto data = make_dungeon(1);
        data["name"]["ja"] = "replacement";
        data["name"]["en"] = "replacement";
        data[section]["tiles"][0]["rate"] = 65537;
        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        // 合計の不一致ではなく、項目の範囲外で拒否したことを確かめる。
        REQUIRE(diagnostics.messages().size() == 1);
        CHECK(diagnostics.messages().front().find("rate '") != std::string::npos);
        CHECK(dungeons.size() == 1);
        CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
        CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == before);
        CHECK(before->name == "test dungeon");
        CHECK(before->prob_table_floor.total_prob() == 100);
        CHECK(before->prob_table_wall.total_prob() == 100);
    }
}

TEST_CASE("DungeonReader rejects terrain weight totals other than one hundred without publishing a dungeon")
{
    auto session = DungeonReader::create_session();
    const std::vector<nlohmann::json> invalid_tiles = {
        nlohmann::json::array(),
        { { { "type", "FLOOR" }, { "rate", 0 } } },
        { { { "type", "FLOOR" }, { "rate", 0 } }, { { "type", "GRANITE" }, { "rate", 0 } } },
        { { { "type", "FLOOR" }, { "rate", 99 } } },
        { { { "type", "FLOOR" }, { "rate", 49 } }, { { "type", "GRANITE" }, { "rate", 50 } } },
        { { { "type", "FLOOR" }, { "rate", 50 } }, { { "type", "GRANITE" }, { "rate", 51 } } },
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 1 } } },
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 100 } } },
    };
    for (const std::string section : { "floor", "wall" }) {
        for (const auto &tiles : invalid_tiles) {
            CAPTURE(section);
            CAPTURE(tiles);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto &dungeons = DungeonList::get_instance();
            const auto original = make_dungeon();
            REQUIRE(test::read_and_commit(DungeonReader(original, session), session).has_value());
            const auto before = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
            auto data = make_dungeon(1);
            data[section]["tiles"] = tiles;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            CHECK(dungeons.size() == 1);
            CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
            CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == before);
        }
    }
}

TEST_CASE("DungeonReader reports invalid terrain entries before the weight total regardless of their position")
{
    auto session = DungeonReader::create_session();
    const std::vector<nlohmann::json> invalid_tiles = {
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "UNKNOWN" }, { "rate", 50 } } },
        { { { "type", "UNKNOWN" }, { "rate", 50 } }, { { "type", "FLOOR" }, { "rate", 100 } } },
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 100 } }, { { "type", "UNKNOWN" }, { "rate", 0 } } },
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 2 } }, { { "type", "FLOOR" }, { "rate", 101 } } },
        { { { "type", "FLOOR" }, { "rate", 100 } }, { { "type", "GRANITE" }, { "rate", 1 } }, { { "type", "FLOOR" } } },
    };
    const std::vector<ParseErrorType> expected_errors = {
        ParseErrorType::UNDEFINED_TERRAIN_TAG,
        ParseErrorType::UNDEFINED_TERRAIN_TAG,
        ParseErrorType::UNDEFINED_TERRAIN_TAG,
        ParseErrorType::INVALID_VALUE,
        ParseErrorType::TOO_FEW_ARGUMENTS,
    };
    REQUIRE(invalid_tiles.size() == expected_errors.size());
    for (const std::string section : { "floor", "wall" }) {
        for (size_t i = 0; i < invalid_tiles.size(); ++i) {
            const auto &tiles = invalid_tiles[i];
            const auto expected = expected_errors[i];
            CAPTURE(section);
            CAPTURE(tiles);
            DungeonReaderStateGuard guard;
            session = DungeonReader::create_session();
            auto data = make_dungeon();
            data[section]["tiles"] = tiles;
            ScopedMessageCapture diagnostics;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == expected);
            CHECK(DungeonList::get_instance().empty());
            if (expected == ParseErrorType::INVALID_VALUE) {
                // 重みの範囲外と合計の超過は同じ種類なので、合計ではなく項目の重みの診断であることを確かめる
                REQUIRE(diagnostics.messages().size() == 1);
                CHECK(diagnostics.messages().front().find("rate '101'") != std::string::npos);
            }
        }
    }
}

TEST_CASE("DungeonReader rejects final-floor ID narrowing before reference lookup")
{
    auto session = DungeonReader::create_session();
    const std::vector<nlohmann::json> invalid_ids{
        -1,
        -32768,
        32767,
        32768,
        -32769,
        65536,
        -65536,
        65537,
        -65535,
        int64_t{ 4294967297 },
        int64_t{ -4294967295 },
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<uint64_t>::max(),
    };
    for (const auto *field : { "guardian", "artifact" }) {
        for (const auto &value : invalid_ids) {
            CAPTURE(std::string(field));
            CAPTURE(value);
            const FinalFloorStateGuard state;
            session = DungeonReader::create_session();
            const auto original_data = make_dungeon();
            REQUIRE(test::read_and_commit(DungeonReader(original_data, session), session).has_value());
            auto &dungeons = DungeonList::get_instance();
            const auto original = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
            auto data = make_dungeon(1);
            data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 } };
            data["final_floor"][field] = value;
            ScopedMessageCapture diagnostics;
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
            // 参照の検証は狭めた後の値を表示するため、元の値が表示されていれば狭める前に範囲外として拒否している
            REQUIRE(diagnostics.messages().size() == 1);
            CHECK(diagnostics.messages().front().find(fmt::format("'{}'", value.dump())) != std::string::npos);
            CHECK(dungeons.size() == 1);
            CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
            CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == original);
            CHECK(original->final_guardian == MonraceId::PLAYER);
            CHECK(original->final_artifact == FixedArtifactId::NONE);
        }
    }
}

TEST_CASE("DungeonReader preserves representable final-floor IDs and the no-artifact sentinel")
{
    auto session = DungeonReader::create_session();
    for (const auto &[guardian, artifact] : { std::pair{ 1, 0 }, std::pair{ 1, 1 }, std::pair{ 1, 32766 }, std::pair{ 32766, 1 } }) {
        CAPTURE(guardian);
        CAPTURE(artifact);
        const FinalFloorStateGuard state;
        session = DungeonReader::create_session();
        const auto guardian_id = static_cast<MonraceId>(guardian);
        const auto artifact_id = static_cast<FixedArtifactId>(artifact);
        if (guardian_id != MonraceId::FILTHY_URCHIN) {
            MonraceList::get_instance().emplace(guardian_id);
        }
        if (artifact_id != FixedArtifactId::NONE && artifact_id != FixedArtifactId::GALADRIEL_PHIAL) {
            ArtifactList::get_instance().emplace(artifact_id, {});
        }
        auto data = make_dungeon();
        data["final_floor"] = { { "guardian", guardian }, { "artifact", artifact } };
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.final_guardian == guardian_id);
        CHECK(dungeon.final_artifact == artifact_id);
    }
}

TEST_CASE("DungeonReader still rejects undefined representable final-floor IDs")
{
    auto session = DungeonReader::create_session();
    for (const auto *field : { "guardian", "artifact" }) {
        CAPTURE(std::string(field));
        const FinalFloorStateGuard state;
        session = DungeonReader::create_session();
        auto data = make_dungeon();
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 } };
        data["final_floor"][field] = 2;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader accepts final object IDs through the Baseitem reader limit")
{
    auto session = DungeonReader::create_session();
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    auto &items = BaseitemList::get_instance();
    const test::ScopedVectorWrapper saved_items(items);
    items.resize(32768);
    for (const auto id : { 1, 9999, 10000, 32767 }) {
        CAPTURE(id);
        auto data = make_dungeon(id == 1 ? 0 : id == 9999 ? 1
                                           : id == 10000  ? 2
                                                          : 3);
        data["final_floor"] = { { "object", id } };
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto dungeon_id = static_cast<DungeonId>(data["id"].get<int>());
        CHECK(DungeonList::get_instance().get_dungeon(dungeon_id).final_object == id);
    }
}

TEST_CASE("DungeonReader rejects final objects outside the Baseitem limit and missing IDs")
{
    auto session = DungeonReader::create_session();
    for (const auto id : { -1, 0, 1, 32768, std::numeric_limits<int>::max() }) {
        CAPTURE(id);
        DungeonReaderStateGuard guard;
        session = DungeonReader::create_session();
        auto &items = BaseitemList::get_instance();
        const test::ScopedVectorWrapper saved_items(items);
        auto data = make_dungeon();
        data["final_floor"] = { { "object", id } };
        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        CHECK(DungeonList::get_instance().empty());
        // 範囲外の ID はベースアイテムを引く前に拒否し、範囲内で未定義の ID だけ参照の診断を出す
        CHECK(diagnostics.messages().size() == (id == 1 ? 1U : 0U));
    }
}

TEST_CASE("DungeonReader rejects guardian zero as below the schema minimum before publication")
{
    auto session = DungeonReader::create_session();
    const FinalFloorStateGuard state;
    session = DungeonReader::create_session();
    auto data = make_dungeon();
    data["final_floor"] = { { "guardian", 0 } };
    CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader propagates nested parse errors without publishing partial definitions")
{
    auto session = DungeonReader::create_session();
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/description", 1, ParseErrorType::INVALID_TYPE },
        { "/generation", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/minDepth", "depth", ParseErrorType::INVALID_TYPE },
        { "/generation/pit", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/pit", { "ORC", "UNKNOWN" }, ParseErrorType::INVALID_FLAG },
        { "/generation/nest", { "UNDEAD", 1 }, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/nest", { "UNKNOWN" }, ParseErrorType::INVALID_FLAG },
        { "/floor", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/floor/tiles/0/rate", 99, ParseErrorType::INVALID_VALUE },
        { "/floor/tiles/0/rate", 101, ParseErrorType::INVALID_VALUE },
        { "/floor/tunnelRate", "rate", ParseErrorType::INVALID_TYPE },
        { "/wall", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/wall/tiles/0/rate", 99, ParseErrorType::INVALID_VALUE },
        { "/wall/tiles/0/rate", 101, ParseErrorType::INVALID_VALUE },
        { "/wall/outer", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/wall/outer", 1, ParseErrorType::INVALID_TYPE },
        { "/wall/outer", "UNKNOWN", ParseErrorType::UNDEFINED_TERRAIN_TAG },
        { "/wall/inner", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/wall/inner", 1, ParseErrorType::INVALID_TYPE },
        { "/wall/inner", "UNKNOWN", ParseErrorType::UNDEFINED_TERRAIN_TAG },
        { "/wall/streams", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/wall/streams/1/type", "UNKNOWN", ParseErrorType::UNDEFINED_TERRAIN_TAG },
        { "/wall/streams/1/count", 0, ParseErrorType::INVALID_VALUE },
        { "/wall/streams/1/chance", "chance", ParseErrorType::INVALID_TYPE },
        { "/wall/streams/1/priority", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/final_floor", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/final_floor/guardian", 65537, ParseErrorType::INVALID_VALUE },
        { "/final_floor/guardian", 2, ParseErrorType::INVALID_VALUE },
        { "/final_floor/object", 0, ParseErrorType::INVALID_VALUE },
        { "/final_floor/artifact", 2, ParseErrorType::INVALID_VALUE },
        { "/flags", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/flags", { "MAZE", "UNKNOWN" }, ParseErrorType::INVALID_FLAG },
        { "/monsters", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/monsters/additionalSpawnProbability", 0, ParseErrorType::INVALID_VALUE },
        { "/monsters/flagsMode", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/monsters/flagsMode", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/monsters/flags", { "UNIQUE", "UNKNOWN" }, ParseErrorType::INVALID_FLAG },
        { "/monsters/symbols", { "D", "XX" }, ParseErrorType::INVALID_VALUE },
        { "/monsters/symbols", { "D", 1 }, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/monsters/spells", { "HASTE", "UNKNOWN" }, ParseErrorType::INVALID_FLAG },
        { "/monsters/spells", { "HASTE", 1 }, ParseErrorType::TOO_FEW_ARGUMENTS },
    };
    for (const auto &field : invalid_fields) {
        CAPTURE(field.path);
        CAPTURE(field.value);
        FinalFloorStateGuard guard;
        session = DungeonReader::create_session();
        const auto original = make_dungeon();
        REQUIRE(test::read_and_commit(DungeonReader(original, session), session).has_value());
        const auto &dungeons = DungeonList::get_instance();
        const auto before = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
        auto data = make_dungeon(1);
        data["wall"]["streams"] = {
            { { "type", "FLOOR" }, { "count", 1 }, { "chance", 1 }, { "priority", 0 } },
            { { "type", "GRANITE" }, { "count", 2 }, { "chance", 2 }, { "priority", 1 } },
        };
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 0 } };
        data[nlohmann::json::json_pointer(field.path)] = field.value;

        ScopedMessageCapture diagnostics;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == field.error);
        if ((std::string_view(field.path) == "/floor/tiles/0/rate" || std::string_view(field.path) == "/wall/tiles/0/rate") && field.value == 101) {
            // 範囲外の重みと合計の不一致を、項目だけに出る診断で区別する。
            REQUIRE(diagnostics.messages().size() == 1);
            CHECK(diagnostics.messages().front().find("rate '") != std::string::npos);
        }
        CHECK(session.last_id().value_or(-1) == 0);
        CHECK(dungeons.size() == 1);
        CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
        CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == before);
        CHECK(before->name == "test dungeon");
        CHECK(before->streams.empty());
        CHECK(before->prob_table_floor.total_prob() == 100);
        CHECK(before->prob_table_wall.total_prob() == 100);
    }
}

TEST_CASE("DungeonReader preserves error priority and the last committed ID")
{
    auto session = DungeonReader::create_session();
    DungeonReaderStateGuard guard;
    session = DungeonReader::create_session();
    auto data = make_dungeon(1);
    data["generation"]["pit"] = { "UNKNOWN" };
    data["wall"]["inner"] = "UNKNOWN";
    data["monsters"]["symbols"] = { "XX" };
    SUBCASE("an invalid ID does not change the last committed ID")
    {
        data["id"] = "id";
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_TYPE);
        CHECK(session.last_id().value_or(-1) == -1);
    }
    SUBCASE("a nonsequential ID precedes field parsing")
    {
        session = DungeonReader::create_session(1);
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
        CHECK(session.last_id().value_or(-1) == 1);
    }
    SUBCASE("name errors do not commit the ID")
    {
        data["name"] = 1;
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_TYPE);
        CHECK_FALSE(session.last_id());
    }
    SUBCASE("generation errors precede later field errors")
    {
        CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == ParseErrorType::INVALID_FLAG);
        CHECK_FALSE(session.last_id());
    }
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader publishes complete nested definitions and accepts optional nulls")
{
    auto session = DungeonReader::create_session();
    FinalFloorStateGuard guard;
    session = DungeonReader::create_session();
    auto &baseitems = BaseitemList::get_instance();
    const auto restore_baseitems = test::ScopedVectorWrapper(baseitems);
    baseitems.resize(2);
    auto data = make_dungeon();
    SUBCASE("populated helpers preserve values and sort streams")
    {
        data["description"] = { { "ja", "description" }, { "en", "description" } };
        data["generation"]["pit"] = { "", "ORC" };
        data["generation"]["nest"] = { "", "UNDEAD" };
        data["wall"]["streams"] = {
            { { "type", "GRANITE" }, { "count", 255 }, { "chance", 65535 }, { "priority", 255 } },
            { { "type", "FLOOR" }, { "count", 1 }, { "chance", 1 }, { "priority", 0 } },
        };
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 }, { "object", 1 } };
        data["flags"] = { "", "MAZE" };
        data["monsters"]["flagsMode"] = "AND";
        data["monsters"]["flags"] = { "", "UNIQUE", "IM_ACID" };
        data["monsters"]["symbols"] = { "", "D" };
        data["monsters"]["spells"] = { "", "HASTE" };
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.text == "description");
        CHECK(dungeon.pit.has(PitKind::ORC));
        CHECK(dungeon.nest.has(NestKind::UNDEAD));
        REQUIRE(dungeon.streams.size() == 2);
        CHECK(dungeon.streams[0].terrain_id == 0);
        CHECK(dungeon.streams[0].count == 1);
        CHECK(dungeon.streams[0].chance == 1);
        CHECK(dungeon.streams[0].priority == 0);
        CHECK(dungeon.streams[1].terrain_id == 1);
        CHECK(dungeon.streams[1].count == 255);
        CHECK(dungeon.streams[1].chance == 65535);
        CHECK(dungeon.streams[1].priority == 255);
        CHECK(dungeon.final_guardian == MonraceId::FILTHY_URCHIN);
        CHECK(dungeon.final_artifact == FixedArtifactId::GALADRIEL_PHIAL);
        CHECK(dungeon.final_object == 1);
        CHECK(dungeon.flags.has(DungeonFeatureType::MAZE));
        CHECK(dungeon.mode == DungeonMode::AND);
        CHECK(dungeon.mon_kind_flags.has(MonsterKindType::UNIQUE));
        CHECK(dungeon.mon_resistance_flags.has(MonsterResistanceType::IMMUNE_ACID));
        CHECK(dungeon.mon_ability_flags.has(MonsterAbilityType::HASTE));
        CHECK(dungeon.r_chars == std::vector<char>{ 'D' });
    }
    SUBCASE("optional null fields keep defaults")
    {
        data["description"] = nullptr;
        data["wall"]["streams"] = nullptr;
        data["final_floor"] = nullptr;
        data["flags"] = nullptr;
        for (const auto *key : { "flags", "symbols", "spells" }) {
            data["monsters"][key] = nullptr;
        }
        REQUIRE(test::read_and_commit(DungeonReader(data, session), session).has_value());
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.text.empty());
        CHECK(dungeon.streams.empty());
        CHECK(dungeon.flags.none());
        CHECK(dungeon.r_chars.empty());
        CHECK(dungeon.mon_kind_flags.none());
        CHECK(dungeon.mon_ability_flags.none());
        CHECK(dungeon.final_guardian == MonraceId{});
        CHECK(dungeon.final_artifact == FixedArtifactId::NONE);
    }
    CHECK(session.last_id().value_or(-1) == 0);
    CHECK(DungeonList::get_instance().size() == 1);
}

TEST_CASE("DungeonReader pins flag array type errors and missing values")
{
    for (const auto *path : { "/flags", "/generation/pit", "/generation/nest", "/monsters/flags", "/monsters/spells" }) {
        for (const auto *input : { "42", "[1]", "null", "missing", "[]" }) {
            CAPTURE(path);
            CAPTURE(input);
            DungeonReaderStateGuard guard;
            auto session = DungeonReader::create_session();
            auto data = make_dungeon();
            const auto pointer = nlohmann::json::json_pointer(path);
            const std::string input_text = input;
            if (input_text == "missing") {
                data[pointer.parent_pointer()].erase(pointer.back());
            } else {
                data[pointer] = nlohmann::json::parse(input);
            }
            auto expected = ParseErrorType::NONE;
            if (input_text == "42") {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            } else if (input_text == "[1]") {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            if (std::string(path).starts_with("/generation/") && (input_text == "null" || input_text == "missing")) {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            CHECK(test::to_error_code(test::read_and_commit(DungeonReader(data, session), session)) == expected);
        }
    }
}
