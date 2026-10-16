#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/terrain-reader.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {
class TerrainStateGuard {
public:
    /*!
     * @brief 既存の地形定義を退避し、エラー位置とメッセージ出力をテスト用に設定する
     * @param initial_error_idx テストの間に設定しておく error_idx の値
     */
    explicit TerrainStateGuard(int initial_error_idx = -1)
        : reader_state(initial_error_idx)
        , terrains(TerrainList::get_instance())
        , saved_terrains(terrains.begin(), terrains.end())
    {
    }

    TerrainStateGuard(const TerrainStateGuard &) = delete;
    TerrainStateGuard &operator=(const TerrainStateGuard &) = delete;

    /*!
     * @brief 退避した地形定義を復元する。エラー位置は reader_state が復元する
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
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    CHECK(error_idx == 0);
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

    data["generation"]["changes"] = { { { "terrain", "FLOOR" }, { "probability", 100 } } };
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    REQUIRE(terrain.generation_changes.size() == 1);
    CHECK(terrain.generation_changes[0].result_tag == "FLOOR");
    CHECK(terrain.generation_changes[0].probability == 100);
}

TEST_CASE("TerrainReader leaves an existing terrain unchanged after a late parse error")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto before = TerrainList::get_instance().get_terrain(0);
    const auto size_before = TerrainList::get_instance().size();

    data["key"] = "FLOOR";
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    data["map_priority"] = 19;
    data["generation"]["changes"] = { { { "terrain", "WALL" }, { "probability", 90 } } };
    data["interactions"] = { { "DESTROYED", 42 } };
    CHECK(TerrainReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);

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
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(terrain.symbol_definitions.at(lighting).character == character.front());
    }
}

TEST_CASE("TerrainReader rejects invalid symbol lengths without publishing a terrain")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    auto &terrains = TerrainList::get_instance();
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();

    std::string character = "..";
    int expected = PARSE_ERROR_INVALID_VALUE;
    SUBCASE("multiple ASCII characters") {}
    SUBCASE("empty string keeps its existing error")
    {
        character.clear();
        expected = PARSE_ERROR_GENERIC;
    }
    SUBCASE("two-byte UTF-8 character")
    {
        character = "\xc3\xa9";
    }
    SUBCASE("four-byte UTF-8 character")
    {
        character = "\xf0\x9f\x8c\xb2";
    }
    data["symbol"]["character"] = character;
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    CHECK(TerrainReader(data).read() == expected);
    CHECK(terrains.size() == size_before);
    const auto &after = terrains.get_terrain(0);
    CHECK(after.name == before.name);
    CHECK(after.tag == before.tag);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(after.symbol_definitions.at(lighting) == before.symbol_definitions.at(lighting));
    }

    data["id"] = size_before + 1;
    CHECK(TerrainReader(data).read() == expected);
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
    CHECK(TerrainReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    CHECK(TerrainList::get_instance().size() == size_before);
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
    REQUIRE(TerrainReader(original).read() == PARSE_ERROR_NONE);
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        parse_error_type error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/symbol", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/symbol/character", "", PARSE_ERROR_GENERIC },
        { "/symbol/character", "..", PARSE_ERROR_INVALID_VALUE },
        { "/symbol/color", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/symbol/lit", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/door", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/door", nlohmann::json::object(), PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/door/power", false, PARSE_ERROR_INVALID_TYPE },
        { "/door/power", 256, PARSE_ERROR_INVALID_FLAG },
        { "/tunnel", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/tunnel/power", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/tunnel/power", -1, PARSE_ERROR_INVALID_FLAG },
        { "/trap", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/trap", { { "power", 1 } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/trap", { { "type", "PIT" } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/trap", { { "type", "UNKNOWN" }, { "power", 1 } }, PARSE_ERROR_INVALID_FLAG },
        { "/pattern", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/pattern/type", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/pattern/type", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/store", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/store/type", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/store/type", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/building", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/building/type", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/building/type", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/convert", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/convert/type", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/convert/type", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/convert", { { "type", "STREAM" } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/convert", { { "type", "STREAM" }, { "stream_index", 255 } }, PARSE_ERROR_INVALID_FLAG },
        { "/generation", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/changes", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/changes/0", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/changes/0/terrain", "", PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/changes/0/probability", false, PARSE_ERROR_INVALID_TYPE },
        { "/generation/changes/0/probability", 0, PARSE_ERROR_INVALID_FLAG },
        { "/generation/changes/1/probability", 100, PARSE_ERROR_INVALID_FLAG },
        { "/interactions", false, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/interactions/DESTROYED", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/interactions/OPEN", "", PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/interactions/DROP_GOLD", "bad", PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/interactions/UNKNOWN", "FLOOR", PARSE_ERROR_INVALID_FLAG },
    };
    for (const auto id : { 0, 3 }) {
        CAPTURE(id);
        for (const auto &invalid : invalid_fields) {
            CAPTURE(invalid.path);
            auto data = original;
            data["id"] = id;
            data["name"]["ja"] = "replacement";
            data["name"]["en"] = "replacement";
            data[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
            CHECK(TerrainReader(data).read() == invalid.error);
            CHECK(error_idx == id);
            CHECK(terrains.size() == size_before);
            check_existing_terrain(terrains.get_terrain(0), before);
        }
    }
}

/*!
 * @brief 公開境界のエラー位置更新とヘルパーの検証順を維持する
 */
TEST_CASE("TerrainReader keeps helper validation order and error index timing")
{
    TerrainStateGuard guard(73);
    auto &terrains = TerrainList::get_instance();
    terrains.resize(0);
    auto data = make_terrain();
    data["id"] = 10000;
    CHECK(TerrainReader(data).read() == PARSE_ERROR_INVALID_FLAG);
    CHECK(error_idx == 73);
    data["id"] = 0;
    data["key"] = "";
    CHECK(TerrainReader(data).read() == PARSE_ERROR_GENERIC);
    CHECK(error_idx == 73);
    data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto before = terrains.get_terrain(0);
    const auto check_invalid = [&](parse_error_type expected) {
        CHECK(TerrainReader(data).read() == expected);
        CHECK(error_idx == 0);
        CHECK(terrains.size() == 1);
        CHECK(terrains.get_terrain(0).flags == before.flags);
        CHECK(terrains.get_terrain(0).generation_changes.size() == 2);
    };
    data["symbol"]["character"] = "";
    data["door"] = { { "power", 256 } };
    data["tunnel"] = nlohmann::json::object();
    check_invalid(PARSE_ERROR_GENERIC);
    data["symbol"]["character"] = ".";
    check_invalid(PARSE_ERROR_INVALID_FLAG);
    data["door"]["power"] = 0;
    check_invalid(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["tunnel"]["power"] = 0;
    data["trap"] = { { "type", "UNKNOWN" }, { "power", 1 } };
    data["pattern"] = nlohmann::json::object();
    check_invalid(PARSE_ERROR_INVALID_FLAG);
    data["trap"]["type"] = "PIT";
    check_invalid(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["pattern"]["type"] = "START";
    data["store"] = { { "type", "UNKNOWN" } };
    data["building"] = nlohmann::json::object();
    check_invalid(PARSE_ERROR_INVALID_FLAG);
    data["store"]["type"] = "GENERAL";
    check_invalid(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["building"]["type"] = "BUILDING_00";
    data["convert"] = { { "type", "STREAM" } };
    data["generation"]["changes"][0]["probability"] = 101;
    data["interactions"] = { { "DESTROYED", 1 } };
    check_invalid(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["convert"]["stream_index"] = 0;
    check_invalid(PARSE_ERROR_INVALID_FLAG);
    data["generation"]["changes"][0]["probability"] = 25;
    check_invalid(PARSE_ERROR_TOO_FEW_ARGUMENTS);
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
        terrains.resize(0);
        auto data = make_terrain();
        data.erase("generation");
        for (const auto *key : { "door", "tunnel", "trap", "pattern", "store", "building", "convert", "generation", "interactions" }) {
            if (use_null) {
                data[key] = nullptr;
            }
        }
        REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
        CHECK(error_idx == 0);
        CHECK(terrains.get_terrain(0).flags.has(TerrainCharacteristics::LOS));
        CHECK(terrains.get_terrain(0).generation_changes.empty());
        CHECK(terrains.get_terrain(0).door_power == 0);
        CHECK(terrains.get_terrain(0).trap_type == TrapType::NOT_TRAP);
    }
    for (const auto power : { 0, 255 }) {
        CAPTURE(power);
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
        REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
        const auto &terrain = terrains.get_terrain(0);
        CHECK(error_idx == 0);
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
