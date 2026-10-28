#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/terrain-reader.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {
class TerrainStateGuard {
public:
    /*!
     * @brief 既存の地形定義を退避し、メッセージ出力をテスト用に設定する
     */
    TerrainStateGuard()
        : terrains(TerrainList::get_instance())
        , saved_terrains(terrains.begin(), terrains.end())
    {
    }

    TerrainReader::Session session;

    TerrainStateGuard(const TerrainStateGuard &) = delete;
    TerrainStateGuard &operator=(const TerrainStateGuard &) = delete;

    /*!
     * @brief 退避した地形定義を復元する
     */
    ~TerrainStateGuard()
    {
        terrains.resize(saved_terrains.size());
        std::copy(saved_terrains.begin(), saved_terrains.end(), terrains.begin());
    }

private:
    test::ScopedReaderState reader_state;
    TerrainList &terrains;
    std::vector<TerrainType> saved_terrains;
};

nlohmann::json make_terrain()
{
    return {
        { "id", 0 },
        { "key", "NONE" },
        { "name", { { "ja", "test terrain" }, { "en", "test terrain" } } },
        { "symbol", { { "character", "." }, { "color", "White" }, { "lit", false } } },
        { "map_priority", 7 },
        { "flags", { "LOS" } },
        { "generation", { { "changes", { { { "terrain", "FLOOR" }, { "probability", 25 } }, { { "terrain", "WALL" }, { "probability", 40 } } } } } },
    };
}

/*!
 * @brief 読込失敗時に既存地形の各解析結果が一切公開されていないことを確かめる
 */
void check_existing_terrain(const TerrainType &after, const TerrainType &before)
{
    CHECK(after.idx == before.idx);
    CHECK(after.tag == before.tag);
    CHECK(after.tag_enum == before.tag_enum);
    CHECK(after.name == before.name);
    CHECK(after.text == before.text);
    CHECK(after.priority == before.priority);
    CHECK(after.flags == before.flags);
    CHECK(after.mimic == before.mimic);
    CHECK(after.mimic_tag == before.mimic_tag);
    CHECK(after.destroyed == before.destroyed);
    CHECK(after.destroyed_tag == before.destroyed_tag);
    CHECK(after.door_power == before.door_power);
    CHECK(after.trap_power == before.trap_power);
    CHECK(after.tunnel_power == before.tunnel_power);
    CHECK(after.trap_type == before.trap_type);
    CHECK(after.pattern_tile_type == before.pattern_tile_type);
    CHECK(after.store_sale_type == before.store_sale_type);
    CHECK(after.building_type == before.building_type);
    CHECK(after.conversion_type == before.conversion_type);
    CHECK(after.stream_index == before.stream_index);
    CHECK(after.gold_drop == before.gold_drop);
    CHECK(after.symbol_definitions == before.symbol_definitions);
    CHECK(after.symbol_configs == before.symbol_configs);
    for (int i = 0; i < MAX_FEAT_STATES; ++i) {
        CHECK(after.state[i].action == before.state[i].action);
        CHECK(after.state[i].result_tag == before.state[i].result_tag);
        CHECK(after.state[i].result == before.state[i].result);
    }
    REQUIRE(after.generation_changes.size() == before.generation_changes.size());
    for (size_t i = 0; i < before.generation_changes.size(); ++i) {
        CHECK(after.generation_changes[i].result_tag == before.generation_changes[i].result_tag);
        CHECK(after.generation_changes[i].result == before.generation_changes[i].result);
        CHECK(after.generation_changes[i].probability == before.generation_changes[i].probability);
    }
}
}

TEST_CASE("TerrainReader reads a valid terrain and replaces generation changes on reload")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    CHECK(terrain.idx == 0);
    CHECK(terrain.tag == "NONE");
    CHECK(terrain.name == "test terrain");
    CHECK(terrain.priority == 7);
    CHECK(terrain.has(TerrainCharacteristics::LOS));
    REQUIRE(terrain.generation_changes.size() == 2);
    CHECK(terrain.generation_changes[0].result_tag == "FLOOR");
    CHECK(terrain.generation_changes[0].probability == 25);
    CHECK(terrain.generation_changes[1].result_tag == "WALL");
    CHECK(terrain.generation_changes[1].probability == 40);

    // 再読み込みは新しいロードセッションとして実行する。
    guard.session = {};
    data["generation"]["changes"] = { { { "terrain", "FLOOR" }, { "probability", 100 } } };
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    REQUIRE(terrain.generation_changes.size() == 1);
    CHECK(terrain.generation_changes[0].result_tag == "FLOOR");
    CHECK(terrain.generation_changes[0].probability == 100);
}

TEST_CASE("TerrainReader accepts map priority from 0 to 125 and rejects values outside")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    for (const auto priority : { 0, 125 }) {
        CAPTURE(priority);
        terrains.resize(0);
        auto data = make_terrain();
        data["map_priority"] = priority;
        guard.session = {};
        REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
        CHECK(terrains.get_terrain(0).priority == priority);
    }

    terrains.resize(0);
    auto data = make_terrain();
    data["map_priority"] = nullptr;
    guard.session = {};
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    CHECK(terrains.get_terrain(0).priority == 0);

    data = make_terrain();
    guard.session = {};
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();
    for (const auto priority : { -1, 126 }) {
        CAPTURE(priority);
        for (const auto id : { 0, 3 }) {
            CAPTURE(id);
            data["id"] = id;
            data["name"]["ja"] = "replacement";
            data["name"]["en"] = "replacement";
            data["map_priority"] = priority;
            guard.session = {};
            CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
            CHECK(terrains.size() == size_before);
            check_existing_terrain(terrains.get_terrain(0), before);
        }
    }
}

TEST_CASE("TerrainReader leaves an existing terrain unchanged after a late parse error")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const auto before = TerrainList::get_instance().get_terrain(0);
    const auto size_before = TerrainList::get_instance().size();

    guard.session = {};
    data["key"] = "FLOOR";
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    data["map_priority"] = 19;
    data["generation"]["changes"] = { { { "terrain", "WALL" }, { "probability", 90 } } };
    data["interactions"] = { { "DESTROYED", 42 } };
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::TOO_FEW_ARGUMENTS);

    const auto &after = TerrainList::get_instance().get_terrain(0);
    CHECK(TerrainList::get_instance().size() == size_before);
    CHECK(after.tag == before.tag);
    CHECK(after.name == before.name);
    CHECK(after.priority == before.priority);
    CHECK(after.flags == before.flags);
    REQUIRE(after.generation_changes.size() == before.generation_changes.size());
    CHECK(after.generation_changes[0].result_tag == before.generation_changes[0].result_tag);
    CHECK(after.generation_changes[0].probability == before.generation_changes[0].probability);
}

TEST_CASE("TerrainReader accepts single-byte symbol characters")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    std::string character = ".";
    SUBCASE("visible character") {}
    SUBCASE("space")
    {
        character = " ";
    }
    data["symbol"]["character"] = character;
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(terrain.symbol_definitions.at(lighting).character == character.front());
    }
}

TEST_CASE("TerrainReader rejects invalid symbol lengths without publishing a terrain")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    auto &terrains = TerrainList::get_instance();
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();

    std::string character = "..";
    auto expected = ParseErrorType::INVALID_VALUE;
    SUBCASE("multiple ASCII characters") {}
    SUBCASE("empty string")
    {
        character.clear();
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("two-byte UTF-8 character")
    {
        character = "\xc3\xa9";
    }
    SUBCASE("four-byte UTF-8 character")
    {
        character = "\xf0\x9f\x8c\xb2";
    }
    guard.session = {};
    data["symbol"]["character"] = character;
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == expected);
    CHECK(terrains.size() == size_before);
    const auto &after = terrains.get_terrain(0);
    CHECK(after.name == before.name);
    CHECK(after.tag == before.tag);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(after.symbol_definitions.at(lighting) == before.symbol_definitions.at(lighting));
    }

    data["id"] = size_before + 1;
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == expected);
    CHECK(terrains.size() == size_before);
    CHECK(terrains.get_terrain(0).name == before.name);
}

TEST_CASE("TerrainReader does not grow the terrain list after a failed new entry")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    const auto size_before = TerrainList::get_instance().size();
    data["id"] = size_before + 1;
    data["interactions"] = { { "DESTROYED", 42 } };
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(TerrainList::get_instance().size() == size_before);
}

/*!
 * @brief 状態遷移の枠 (MAX_FEAT_STATES) を超える相互作用は不正な値として拒否する
 */
TEST_CASE("TerrainReader reports interactions beyond the state slots as invalid values")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    const auto new_id = static_cast<int>(TerrainList::get_instance().size());
    data["id"] = new_id;
    constexpr auto slots = static_cast<size_t>(MAX_FEAT_STATES);
    const std::vector<std::string> actions = { "OPEN", "CLOSE", "BASH", "SPIKE", "DISARM", "TUNNEL", "LOS", "MOVE", "PLACE" };
    REQUIRE(actions.size() == slots + 1);
    data["interactions"] = nlohmann::json::object();
    for (size_t i = 0; i < slots; ++i) {
        data["interactions"][actions[i]] = "FLOOR";
    }
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());

    // 新しいIDにして、前の読み込みで埋まった状態を引き継がない地形で枠を超えさせる
    guard.session = {};
    data["id"] = new_id + 1;
    data["interactions"][actions.back()] = "FLOOR";
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
}

/*!
 * @brief 全ヘルパーの解析エラーを伝播し、既存地形と新規IDの件数を保持する
 */
TEST_CASE("TerrainReader propagates helper errors without publishing staged terrain")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    const auto original = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(original, guard.session), guard.session).has_value());
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/symbol", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/symbol/character", "", ParseErrorType::INVALID_VALUE },
        { "/symbol/character", "..", ParseErrorType::INVALID_VALUE },
        { "/symbol/color", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/symbol/lit", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/door", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/door", nlohmann::json::object(), ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/door/power", false, ParseErrorType::INVALID_TYPE },
        { "/door/power", 256, ParseErrorType::INVALID_VALUE },
        { "/tunnel", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/tunnel/power", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/tunnel/power", -1, ParseErrorType::INVALID_VALUE },
        { "/trap", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/trap", { { "power", 1 } }, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/trap", { { "type", "PIT" } }, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/trap", { { "type", "UNKNOWN" }, { "power", 1 } }, ParseErrorType::INVALID_FLAG },
        { "/pattern", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/pattern/type", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/pattern/type", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/store", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/store/type", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/store/type", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/building", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/building/type", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/building/type", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/convert", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/convert/type", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/convert/type", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/convert", { { "type", "STREAM" } }, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/convert", { { "type", "STREAM" }, { "stream_index", 255 } }, ParseErrorType::INVALID_VALUE },
        { "/generation", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/changes", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/changes/0", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/changes/0/terrain", "", ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/generation/changes/0/probability", false, ParseErrorType::INVALID_TYPE },
        { "/generation/changes/0/probability", 0, ParseErrorType::INVALID_VALUE },
        { "/generation/changes/1/probability", 100, ParseErrorType::INVALID_VALUE },
        { "/interactions", false, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/interactions/DESTROYED", 1, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/interactions/OPEN", "", ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/interactions/DROP_GOLD", "bad", ParseErrorType::INVALID_VALUE },
        { "/interactions/UNKNOWN", "FLOOR", ParseErrorType::INVALID_FLAG },
    };
    for (const auto id : { 0, 3 }) {
        CAPTURE(id);
        for (const auto &invalid : invalid_fields) {
            CAPTURE(invalid.path);
            guard.session = {};
            auto data = original;
            data["id"] = id;
            data["name"]["ja"] = "replacement";
            data["name"]["en"] = "replacement";
            data[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
            CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == invalid.error);
            CHECK(terrains.size() == size_before);
            check_existing_terrain(terrains.get_terrain(0), before);
        }
    }
}

TEST_CASE("TerrainReader accepts IDs up to one below the FEAT_IDX storage maximum")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    // 地形IDの上限は、IDを格納する型(FEAT_IDX)の上限−1とする。
    constexpr short max_id = 32766;
    static_assert(max_id == MAX_TERRAIN_ID);
    auto data = make_terrain();
    data["id"] = max_id;
    // 上限のIDを読むため、地形の一覧を 32,767 件に広げるのは意図どおり。
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    CHECK(terrains.size() == static_cast<size_t>(max_id) + 1);
    CHECK(terrains.get_terrain(max_id).idx == max_id);

    data["id"] = max_id + 1;
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
    CHECK(terrains.size() == static_cast<size_t>(max_id) + 1);
}

/*!
 * @brief 公開境界でのヘルパーの検証順を維持する
 */
TEST_CASE("TerrainReader keeps helper validation order and error index timing")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    auto data = make_terrain();
    data["id"] = 32767;
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
    data["id"] = 0;
    data["key"] = "";
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
    data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const auto before = terrains.get_terrain(0);
    const auto check_invalid = [&](ParseErrorType expected) {
        guard.session = {};
        CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == expected);
        CHECK(terrains.size() == 1);
        CHECK(terrains.get_terrain(0).flags == before.flags);
        CHECK(terrains.get_terrain(0).generation_changes.size() == 2);
    };
    data["symbol"]["character"] = "";
    data["door"] = { { "power", false } };
    data["tunnel"] = nlohmann::json::object();
    check_invalid(ParseErrorType::INVALID_VALUE);
    data["symbol"]["character"] = ".";
    check_invalid(ParseErrorType::INVALID_TYPE);
    data["door"]["power"] = 0;
    check_invalid(ParseErrorType::TOO_FEW_ARGUMENTS);
    data["tunnel"]["power"] = 0;
    data["trap"] = { { "type", "UNKNOWN" }, { "power", 1 } };
    data["pattern"] = nlohmann::json::object();
    check_invalid(ParseErrorType::INVALID_FLAG);
    data["trap"]["type"] = "PIT";
    check_invalid(ParseErrorType::TOO_FEW_ARGUMENTS);
    data["pattern"]["type"] = "START";
    data["store"] = { { "type", "UNKNOWN" } };
    data["building"] = nlohmann::json::object();
    check_invalid(ParseErrorType::INVALID_FLAG);
    data["store"]["type"] = "GENERAL";
    check_invalid(ParseErrorType::TOO_FEW_ARGUMENTS);
    data["building"]["type"] = "BUILDING_00";
    data["convert"] = { { "type", "STREAM" } };
    data["generation"]["changes"][0]["probability"] = 101;
    data["interactions"] = { { "DESTROYED", 1 } };
    check_invalid(ParseErrorType::TOO_FEW_ARGUMENTS);
    data["convert"]["stream_index"] = 0;
    check_invalid(ParseErrorType::INVALID_VALUE);
    data["generation"]["changes"][0]["probability"] = 25;
    check_invalid(ParseErrorType::TOO_FEW_ARGUMENTS);
}

/*!
 * @brief 省略・null・各用途のゼロ値と上限値を正常なヘルパー結果として公開する
 */
TEST_CASE("TerrainReader preserves optional helpers and successful endpoint values")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    for (const auto use_null : { false, true }) {
        CAPTURE(use_null);
        guard.session = {};
        terrains.resize(0);
        auto data = make_terrain();
        data.erase("generation");
        for (const auto *key : { "door", "tunnel", "trap", "pattern", "store", "building", "convert", "generation", "interactions" }) {
            if (use_null) {
                data[key] = nullptr;
            }
        }
        REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
        CHECK(terrains.get_terrain(0).flags.has(TerrainCharacteristics::LOS));
        CHECK(terrains.get_terrain(0).generation_changes.empty());
        CHECK(terrains.get_terrain(0).door_power == 0);
        CHECK(terrains.get_terrain(0).trap_type == TrapType::NOT_TRAP);
    }
    for (const auto power : { 0, 255 }) {
        CAPTURE(power);
        guard.session = {};
        terrains.resize(0);
        auto data = make_terrain();
        data["door"] = { { "power", power } };
        data["tunnel"] = { { "power", power } };
        data["trap"] = { { "type", "TRAPDOOR" }, { "power", power } };
        data["pattern"] = { { "type", "START" } };
        data["store"] = { { "type", "GENERAL" } };
        data["building"] = { { "type", "BUILDING_00" } };
        data["symbol"]["lit"] = power != 0;
        const auto stream_index = power == 0 ? 0 : 254;
        data["convert"] = { { "type", "STREAM" }, { "stream_index", stream_index } };
        data["interactions"] = { { "DESTROYED", "WALL" }, { "DROP_GOLD", "1d2" }, { "OPEN", "FLOOR" } };
        REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
        const auto &terrain = terrains.get_terrain(0);
        CHECK(terrain.symbol_definitions[F_LIT_STANDARD].character == '.');
        CHECK(terrain.door_power == power);
        CHECK(terrain.tunnel_power == power);
        CHECK(terrain.trap_power == power);
        CHECK(terrain.trap_type == TrapType::TRAPDOOR);
        CHECK(terrain.pattern_tile_type == PatternTileType::START);
        CHECK(terrain.store_sale_type == StoreSaleType::GENERAL);
        CHECK(terrain.building_type == BuildingType::BUILDING_00);
        CHECK(terrain.conversion_type == TerrainConversionType::STREAM);
        CHECK(terrain.stream_index == stream_index);
        CHECK(terrain.destroyed_tag == "WALL");
        CHECK(terrain.gold_drop == Dice(1, 2));
        CHECK(terrain.flags.has(TerrainCharacteristics::HAS_GOLD));
        CHECK(terrain.state[0].action == TerrainCharacteristics::OPEN);
        CHECK(terrain.state[0].result_tag == "FLOOR");
        REQUIRE(terrain.generation_changes.size() == 2);
        CHECK(terrain.generation_changes[1].probability == 40);
    }
}

TEST_CASE("TerrainReader rejects duplicate IDs and keys within one session without publication")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    auto data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    data["id"] = 2;
    data["key"] = "WALL";
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    const std::vector<TerrainType> before(terrains.begin(), terrains.end());

    auto expected = ParseErrorType::NON_SEQUENTIAL_RECORDS;
    SUBCASE("duplicate ID after another record")
    {
        data["id"] = 0;
        data["key"] = "FLOOR";
    }
    SUBCASE("duplicate key with a new ID")
    {
        data["id"] = 4;
        data["key"] = "NONE";
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("duplicate key with an existing unused ID")
    {
        data["id"] = 1;
        data["key"] = "NONE";
        expected = ParseErrorType::INVALID_VALUE;
    }
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == expected);
    REQUIRE(terrains.size() == before.size());
    for (size_t i = 0; i < before.size(); ++i) {
        check_existing_terrain(terrains.get_terrain(static_cast<short>(i)), before[i]);
    }
}

TEST_CASE("TerrainReader accepts distinct IDs and keys in any order within a session")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    auto data = make_terrain();
    data["id"] = 2;
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    data["id"] = 0;
    data["key"] = "FLOOR";
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    data["id"] = 1;
    data["key"] = "WALL";
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    CHECK(terrains.size() == 3);
    CHECK(terrains.get_terrain(0).tag == "FLOOR");
    CHECK(terrains.get_terrain(1).tag == "WALL");
    CHECK(terrains.get_terrain(2).tag == "NONE");
}

TEST_CASE("TerrainReader reloads existing IDs and keys in a new session")
{
    TerrainStateGuard guard;
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    auto data = make_terrain();
    REQUIRE(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    data["generation"]["changes"] = { { { "terrain", "WALL" }, { "probability", 100 } } };
    TerrainReader::Session reload_session;
    REQUIRE(test::read_and_commit(TerrainReader(data, reload_session), reload_session).has_value());
    CHECK(terrains.size() == 1);
    const auto &terrain = terrains.get_terrain(0);
    CHECK(terrain.tag == "NONE");
    CHECK(terrain.name == "replacement");
    REQUIRE(terrain.generation_changes.size() == 1);
    CHECK(terrain.generation_changes[0].probability == 100);
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, reload_session), reload_session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
}

TEST_CASE("TerrainReader pins flag array type errors and missing values")
{
    for (const auto *path : { "/flags" }) {
        for (const auto *input : { "42", "[1]", "null", "missing", "[]" }) {
            CAPTURE(path);
            CAPTURE(input);
            TerrainStateGuard guard;
            auto session = TerrainReader::create_session();
            auto data = make_terrain();
            const auto pointer = nlohmann::json::json_pointer(path);
            const std::string input_text = input;
            if (input_text == "missing") {
                data[pointer.parent_pointer()].erase(pointer.back());
            } else {
                data[pointer] = nlohmann::json::parse(input);
            }
            auto expected = ParseErrorType::NONE;
            if (input_text == "42" || input_text == "[1]") {
                expected = ParseErrorType::INVALID_TYPE;
            }
            if (input_text == "null" || input_text == "missing") {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, session), session)) == expected);
        }
    }
}

TEST_CASE("TerrainReader preserves symbol error codes and validation precedence")
{
    TerrainStateGuard guard;
    auto session = TerrainReader::create_session();
    struct InvalidSymbol {
        const char *json;
        ParseErrorType error;
    };
    const InvalidSymbol cases[] = {
        { R"(null)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(42)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"([])", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(true)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"("bad")", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": null, "color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": 42, "color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": true, "color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": [], "color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": {}, "color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": null, "character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": 42, "character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": true, "character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": [], "character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": {}, "character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": "!", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color": "White", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": "", "color": "White", "lit": false})", ParseErrorType::INVALID_VALUE },
        { R"({"character": "!!", "color": "White", "lit": false})", ParseErrorType::INVALID_VALUE },
        { R"({"character": "\u3042", "color": "White", "lit": false})", ParseErrorType::INVALID_VALUE },
        { R"({"character": "!", "color": "UNKNOWN", "lit": false})", ParseErrorType::INVALID_FLAG },
        { R"({"character": "", "color": "UNKNOWN", "lit": false})", ParseErrorType::INVALID_VALUE },
        { R"({"character": "", "color": 42, "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character": "", "lit": false})", ParseErrorType::TOO_FEW_ARGUMENTS },
    };
    for (const auto &invalid : cases) {
        CAPTURE(invalid.json);
        auto data = make_terrain();
        data["symbol"] = nlohmann::json::parse(invalid.json);
        CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, session), session)) == invalid.error);
    }
    auto data = make_terrain();
    data.erase("symbol");
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
}

TEST_CASE("TerrainReader checks lit type before symbol length and color")
{
    TerrainStateGuard guard;
    for (const auto *invalid_lit : { "null", "1", "[]", "{}", "\"bad\"" }) {
        CAPTURE(invalid_lit);
        auto session = TerrainReader::create_session();
        auto data = make_terrain();
        data["symbol"]["character"] = "";
        data["symbol"]["color"] = "UNKNOWN";
        data["symbol"]["lit"] = nlohmann::json::parse(invalid_lit);
        CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    auto session = TerrainReader::create_session();
    auto data = make_terrain();
    data["symbol"].erase("lit");
    CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, session), session)) == ParseErrorType::TOO_FEW_ARGUMENTS);
}

TEST_CASE("TerrainReader rejects unusable gold dice and preserves valid dice")
{
    for (const auto *dice : { "0d0", "0d1", "1d0", "-1d2", "1d-2", "1d1", "2d6" }) {
        CAPTURE(dice);
        TerrainStateGuard guard;
        auto &terrains = TerrainList::get_instance();
        terrains.resize(0);
        const auto original = make_terrain();
        REQUIRE(test::read_and_commit(TerrainReader(original, guard.session), guard.session).has_value());
        const auto before = terrains.get_terrain(0);
        guard.session = {};
        auto data = original;
        data["interactions"] = { { "DROP_GOLD", dice } };
        const auto parsed = Dice::parse(dice);
        const auto accepted = parsed.is_valid();
        const auto result = test::read_and_commit(TerrainReader(data, guard.session), guard.session);
        CHECK(test::to_error_code(result) == (accepted ? ParseErrorType::NONE : ParseErrorType::INVALID_VALUE));
        REQUIRE(terrains.size() == 1);
        if (accepted) {
            CHECK(terrains.get_terrain(0).gold_drop == parsed);
            CHECK(terrains.get_terrain(0).flags.has(TerrainCharacteristics::HAS_GOLD));
        } else {
            check_existing_terrain(terrains.get_terrain(0), before);
            CHECK(terrains.get_terrain(0).gold_drop == before.gold_drop);
            data["id"] = 3;
            CHECK(test::to_error_code(test::read_and_commit(TerrainReader(data, guard.session), guard.session)) == ParseErrorType::INVALID_VALUE);
            CHECK(terrains.size() == 1);
            data["id"] = 0;
            data["interactions"]["DROP_GOLD"] = "1d1";
            CHECK(test::read_and_commit(TerrainReader(data, guard.session), guard.session).has_value());
        }
    }
}
