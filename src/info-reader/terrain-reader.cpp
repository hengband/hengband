#include "info-reader/terrain-reader.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "util/dice.h"
#include "view/display-messages.h"
#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {
// 縮小マップの byte 変換で折り返さず、同順位の +1 後も自動拾い印の 127 未満にする。
constexpr int MAX_TERRAIN_MAP_PRIORITY = 125;
}

TerrainReader::TerrainReader(const nlohmann::json &terrain_data, Session &session)
    : terrain_data(terrain_data)
    , session(session)
{
}

/*!
 * @brief 地形定義(TerrainDefinitions)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::read() const
{
    const auto &element = this->terrain_data;
    if (element.is_null() || !element.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    int id = 0;
    if (const auto result = info_set_integer(get_json_value(element, "id"), id, true, Range(0, MAX_TERRAIN_ID)); !result) {
        return result;
    }

    const auto &key_obj = get_json_value(element, "key");
    if (!key_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    const auto key = key_obj.get<std::string>();
    if (key.empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    auto &terrains = TerrainList::get_instance();
    error_idx = id;

    if (this->session.ids.contains(id)) {
        msg_print(_("地形ID '{}' が重複しています。", "Duplicate terrain ID '{}'."), id);
        return tl::make_unexpected(ParseErrorType::NON_SEQUENTIAL_RECORDS);
    }
    if (this->session.keys.contains(key)) {
        msg_print(_("地形タグ '{}' が重複しています。", "Duplicate terrain tag '{}'."), key);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto s = static_cast<short>(id);
    // Preserve fields that are not supplied by the JSON when reloading a terrain.
    // A failed parse must leave both the existing entry and the list size intact.
    auto terrain = id < static_cast<int>(terrains.size()) ? terrains.get_terrain(s) : TerrainType{};

    terrain.idx = s;
    terrain.tag = key;

    const auto tag_it = terrain_tags.find(key);
    if (tag_it != terrain_tags.end()) {
        terrain.tag_enum = tag_it->second;
    }

    terrain.mimic = s;
    terrain.destroyed = s;
    terrain.gold_drop = Dice{};
    terrain.generation_changes.clear();
    for (auto j = 0; j < MAX_FEAT_STATES; j++) {
        terrain.state[j].action = TerrainCharacteristics::MAX;
        terrain.state[j].result_tag.clear();
    }

    if (const auto result = info_set_string(get_json_value(element, "name"), terrain.name, true); !result) {
        return result;
    }

    if (const auto result = this->set_terrain_symbol(terrain); !result) {
        return result;
    }

    const auto &mimic_obj = get_json_value(element, "mimic");
    if (!mimic_obj.is_null()) {
        if (!mimic_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        terrain.mimic_tag = mimic_obj.get<std::string>();
    }

    const auto &prio_obj = get_json_value(element, "map_priority");
    if (!prio_obj.is_null()) {
        if (const auto result = info_set_integer(prio_obj, terrain.priority, true, Range(0, MAX_TERRAIN_MAP_PRIORITY)); !result) {
            return result;
        }
    }

    const auto &flags_obj = get_json_value(element, "flags");
    if (flags_obj.is_null() || !flags_obj.is_array()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        if (!this->grab_one_feat_flag(terrain, f)) {
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }
    }
    if (const auto result = this->set_terrain_door(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_tunnel(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_trap(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_pattern(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_store(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_building(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_conversion(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_generation(terrain); !result) {
        return result;
    }
    if (const auto result = this->set_terrain_interactions(terrain); !result) {
        return result;
    }

    if (id >= static_cast<int>(terrains.size())) {
        terrains.resize(id + 1);
    }
    terrains.get_terrain(s) = std::move(terrain);
    this->session.ids.insert(id);
    this->session.keys.insert(key);

    return {};
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
bool TerrainReader::grab_one_feat_flag(TerrainType &terrain, std::string_view what) const
{
    if (EnumClassFlagGroup<TerrainCharacteristics>::grab_one_flag(terrain.flags, f_info_flags, what)) {
        return true;
    }

    msg_print(_("未知の地形フラグ '{}'。", "Unknown feature flag '{}'."), what);
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
static std::optional<TerrainCharacteristics> parse_terrain_action(std::string_view what)
{
    if (auto it = f_info_flags.find(what); it != f_info_flags.end()) {
        return it->second;
    }

    return std::nullopt;
}

/*!
 * @brief JSON Objectから地形シンボルをセットする
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_symbol(TerrainType &terrain) const
{
    const auto &symbol_obj = get_json_value(this->terrain_data, "symbol");
    if (symbol_obj.is_null() || !symbol_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &ch_obj = get_json_value(symbol_obj, "character");
    const auto &color_obj = get_json_value(symbol_obj, "color");
    const auto &lit_obj = get_json_value(symbol_obj, "lit");

    if (!ch_obj.is_string() || !color_obj.is_string() || !lit_obj.is_boolean()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto ch_str = ch_obj.get<std::string>();
    if (ch_str.size() != 1) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto color_name = color_obj.get<std::string>();
    const auto it = color_list.find(color_name);
    if (it == color_list.end()) {
        msg_print(_("未知の色 '{}'。", "Unknown color '{}'."), color_name);
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    if (it->second > 127) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto character = ch_str.front();
    const auto color = it->second;

    terrain.symbol_definitions[F_LIT_STANDARD] = { color, character };

    const bool lit = lit_obj.get<bool>();
    if (lit) {
        terrain.reset_lighting(false);
        return {};
    }

    for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
        terrain.symbol_definitions[j] = { color, character };
    }

    return {};
}

/*!
 * @brief 地形の変換先定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_conversion(TerrainType &terrain) const
{
    const auto &convert_obj = get_json_value(this->terrain_data, "convert");
    if (convert_obj.is_null()) {
        return {};
    }
    if (!convert_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &type_obj = get_json_value(convert_obj, "type");
    if (!type_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    static const std::unordered_map<std::string_view, TerrainConversionType> conversion_types = {
        { "FLOOR", TerrainConversionType::FLOOR },
        { "WALL", TerrainConversionType::WALL },
        { "INNER", TerrainConversionType::INNER },
        { "OUTER", TerrainConversionType::OUTER },
        { "SOLID", TerrainConversionType::SOLID },
        { "STREAM", TerrainConversionType::STREAM },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = conversion_types.find(type);
    if (it == conversion_types.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }

    auto stream_index = -1;
    if (it->second == TerrainConversionType::STREAM) {
        // 上限 254 は、1バイトだった旧形式の SUBTYPE を stream_index に移したとき (51228d3210) に引き継いだもの。
        // ダンジョンの streams の数には上限がなく、数を超える位置は DungeonDefinition::select_stream_terrain_id() で元の地形のままになる。
        if (const auto result = info_set_integer(get_json_value(convert_obj, "stream_index"), stream_index, true, Range(0, 254)); !result) {
            return result;
        }
    }

    terrain.flags.set(TerrainCharacteristics::CONVERT);
    if (!terrain.init_conversion_type(it->second, stream_index)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief 地形の生成時変化定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_generation(TerrainType &terrain) const
{
    const auto &generation_obj = get_json_value(this->terrain_data, "generation");
    if (generation_obj.is_null()) {
        return {};
    }
    if (!generation_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &changes_obj = get_json_value(generation_obj, "changes");
    if (!changes_obj.is_array()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    auto probability_sum = 0;
    for (const auto &change_obj : changes_obj) {
        if (!change_obj.is_object()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        const auto &terrain_obj = get_json_value(change_obj, "terrain");
        if (!terrain_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        auto change = TerrainGenerationChange{};
        change.result_tag = terrain_obj.get<std::string>();
        if (change.result_tag.empty()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        if (const auto result = info_set_integer(get_json_value(change_obj, "probability"), change.probability, true, Range(1, 100)); !result) {
            return result;
        }

        probability_sum += change.probability;
        if (probability_sum > 100) {
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }

        terrain.generation_changes.push_back(change);
    }

    return {};
}

/*!
 * @brief 地形の用途別強度をJSON定義から設定する
 * @param power_obj 強度値を表すJSON値
 * @param power 設定先の強度
 * @param is_required trueなら未定義時にエラーとする
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
static tl::expected<void, ParseErrorType> set_terrain_power(const nlohmann::json &power_obj, uint8_t &power, bool is_required = false)
{
    if (const auto result = info_set_integer(power_obj, power, is_required, Range(0, 255)); !result) {
        return result;
    }
    return {};
}

/*!
 * @brief 地形の罠定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_trap(TerrainType &terrain) const
{
    const auto &trap_obj = get_json_value(this->terrain_data, "trap");
    if (trap_obj.is_null()) {
        return {};
    }
    if (!trap_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &type_obj = get_json_value(trap_obj, "type");
    if (!type_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (const auto result = set_terrain_power(get_json_value(trap_obj, "power"), terrain.trap_power, true); !result) {
        return result;
    }

    static const std::unordered_map<std::string_view, TrapType> trap_types = {
        { "TRAPDOOR", TrapType::TRAPDOOR },
        { "PIT", TrapType::PIT },
        { "SPIKED_PIT", TrapType::SPIKED_PIT },
        { "POISON_PIT", TrapType::POISON_PIT },
        { "TY_CURSE", TrapType::TY_CURSE },
        { "TELEPORT", TrapType::TELEPORT },
        { "FIRE", TrapType::FIRE },
        { "ACID", TrapType::ACID },
        { "SLOW", TrapType::SLOW },
        { "LOSE_STR", TrapType::LOSE_STR },
        { "LOSE_DEX", TrapType::LOSE_DEX },
        { "LOSE_CON", TrapType::LOSE_CON },
        { "BLIND", TrapType::BLIND },
        { "CONFUSE", TrapType::CONFUSE },
        { "POISON", TrapType::POISON },
        { "SLEEP", TrapType::SLEEP },
        { "TRAPS", TrapType::TRAPS },
        { "ALARM", TrapType::ALARM },
        { "OPEN", TrapType::OPEN },
        { "ARMAGEDDON", TrapType::ARMAGEDDON },
        { "PIRANHA", TrapType::PIRANHA },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = trap_types.find(type);
    if (it == trap_types.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }

    terrain.flags.set(TerrainCharacteristics::TRAP);
    if (!terrain.init_trap_type(it->second)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief 扉定義を地形情報へ反映する
 * @param terrain 設定先の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_door(TerrainType &terrain) const
{
    const auto &door_obj = get_json_value(this->terrain_data, "door");
    if (door_obj.is_null()) {
        return {};
    }
    if (!door_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    terrain.flags.set(TerrainCharacteristics::DOOR);
    return set_terrain_power(get_json_value(door_obj, "power"), terrain.door_power, true);
}

/*!
 * @brief 掘削定義を地形情報へ反映する
 * @param terrain 設定先の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_tunnel(TerrainType &terrain) const
{
    const auto &tunnel_obj = get_json_value(this->terrain_data, "tunnel");
    if (tunnel_obj.is_null()) {
        return {};
    }
    if (!tunnel_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    terrain.flags.set(TerrainCharacteristics::TUNNEL);
    return set_terrain_power(get_json_value(tunnel_obj, "power"), terrain.tunnel_power, true);
}

/*!
 * @brief 地形のパターン定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_pattern(TerrainType &terrain) const
{
    const auto &pattern_obj = get_json_value(this->terrain_data, "pattern");
    if (pattern_obj.is_null()) {
        return {};
    }
    if (!pattern_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &type_obj = get_json_value(pattern_obj, "type");
    if (!type_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    static const std::unordered_map<std::string_view, PatternTileType> pattern_types = {
        { "START", PatternTileType::START },
        { "TILE_1", PatternTileType::TILE_1 },
        { "TILE_2", PatternTileType::TILE_2 },
        { "TILE_3", PatternTileType::TILE_3 },
        { "TILE_4", PatternTileType::TILE_4 },
        { "END", PatternTileType::END },
        { "OLD", PatternTileType::OLD },
        { "TELEPORT", PatternTileType::TELEPORT },
        { "WRECKED", PatternTileType::WRECKED },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = pattern_types.find(type);
    if (it == pattern_types.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }

    terrain.flags.set(TerrainCharacteristics::PATTERN);
    if (!terrain.init_pattern_tile_type(it->second)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief 地形の店舗定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_store(TerrainType &terrain) const
{
    const auto &store_obj = get_json_value(this->terrain_data, "store");
    if (store_obj.is_null()) {
        return {};
    }
    if (!store_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &type_obj = get_json_value(store_obj, "type");
    if (!type_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    static const std::unordered_map<std::string_view, StoreSaleType> store_types = {
        { "GENERAL", StoreSaleType::GENERAL },
        { "ARMOURY", StoreSaleType::ARMOURY },
        { "WEAPON", StoreSaleType::WEAPON },
        { "TEMPLE", StoreSaleType::TEMPLE },
        { "ALCHEMIST", StoreSaleType::ALCHEMIST },
        { "MAGIC", StoreSaleType::MAGIC },
        { "BLACK", StoreSaleType::BLACK },
        { "HOME", StoreSaleType::HOME },
        { "BOOK", StoreSaleType::BOOK },
        { "MUSEUM", StoreSaleType::MUSEUM },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = store_types.find(type);
    if (it == store_types.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }

    terrain.flags.set(TerrainCharacteristics::STORE);
    if (!terrain.init_store_sale_type(it->second)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief 地形の施設定義を検証し、公開前の情報へ格納する
 * @param terrain 公開前の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_building(TerrainType &terrain) const
{
    const auto &building_obj = get_json_value(this->terrain_data, "building");
    if (building_obj.is_null()) {
        return {};
    }
    if (!building_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &type_obj = get_json_value(building_obj, "type");
    if (!type_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    static const std::unordered_map<std::string_view, BuildingType> building_types = {
        { "BUILDING_00", BuildingType::BUILDING_00 },
        { "BUILDING_01", BuildingType::BUILDING_01 },
        { "BUILDING_02", BuildingType::BUILDING_02 },
        { "BUILDING_03", BuildingType::BUILDING_03 },
        { "BUILDING_04", BuildingType::BUILDING_04 },
        { "BUILDING_05", BuildingType::BUILDING_05 },
        { "BUILDING_06", BuildingType::BUILDING_06 },
        { "BUILDING_07", BuildingType::BUILDING_07 },
        { "BUILDING_08", BuildingType::BUILDING_08 },
        { "BUILDING_09", BuildingType::BUILDING_09 },
        { "BUILDING_10", BuildingType::BUILDING_10 },
        { "BUILDING_11", BuildingType::BUILDING_11 },
        { "BUILDING_12", BuildingType::BUILDING_12 },
        { "BUILDING_13", BuildingType::BUILDING_13 },
        { "BUILDING_14", BuildingType::BUILDING_14 },
        { "BUILDING_15", BuildingType::BUILDING_15 },
        { "BUILDING_16", BuildingType::BUILDING_16 },
        { "BUILDING_17", BuildingType::BUILDING_17 },
        { "BUILDING_18", BuildingType::BUILDING_18 },
        { "BUILDING_19", BuildingType::BUILDING_19 },
        { "BUILDING_20", BuildingType::BUILDING_20 },
        { "BUILDING_21", BuildingType::BUILDING_21 },
        { "BUILDING_22", BuildingType::BUILDING_22 },
        { "BUILDING_23", BuildingType::BUILDING_23 },
        { "BUILDING_24", BuildingType::BUILDING_24 },
        { "BUILDING_25", BuildingType::BUILDING_25 },
        { "BUILDING_26", BuildingType::BUILDING_26 },
        { "BUILDING_27", BuildingType::BUILDING_27 },
        { "BUILDING_28", BuildingType::BUILDING_28 },
        { "BUILDING_29", BuildingType::BUILDING_29 },
        { "BUILDING_30", BuildingType::BUILDING_30 },
        { "BUILDING_31", BuildingType::BUILDING_31 },
    };
    const auto type = type_obj.get<std::string>();
    const auto it = building_types.find(type);
    if (it == building_types.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }

    terrain.flags.set(TerrainCharacteristics::BLDG);
    if (!terrain.init_building_type(it->second)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief 地形変化定義を地形情報へ反映する
 * @param terrain 設定先の地形情報
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> TerrainReader::set_terrain_interactions(TerrainType &terrain) const
{
    const auto &interactions_obj = get_json_value(this->terrain_data, "interactions");
    if (interactions_obj.is_null()) {
        return {};
    }
    if (!interactions_obj.is_object()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (auto it = interactions_obj.begin(); it != interactions_obj.end(); ++it) {
        const auto action = it.key();
        const auto &value_obj = it.value();
        if (!value_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        const auto result_tag = value_obj.get<std::string>();
        if (result_tag.empty()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        if (action == "DROP_GOLD") {
            Dice dice;
            if (const auto result = info_set_dice(value_obj, dice, true); !result) {
                return result;
            }
            if (dice.is_valid()) {
                terrain.gold_drop = dice;
                terrain.flags.set(TerrainCharacteristics::HAS_GOLD);
            }
            continue;
        }

        if (action == "DESTROYED") {
            terrain.destroyed_tag = result_tag;
            continue;
        }

        const auto state_it = std::find_if(std::begin(terrain.state), std::end(terrain.state),
            [](const auto &s) { return s.action == TerrainCharacteristics::MAX; });
        if (state_it == std::end(terrain.state)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        const auto idx_state = static_cast<int>(std::distance(std::begin(terrain.state), state_it));

        if (auto val = parse_terrain_action(action)) {
            terrain.state[idx_state].action = *val;
            if (*val == TerrainCharacteristics::MAY_HAVE_GOLD) {
                terrain.flags.set(TerrainCharacteristics::MAY_HAVE_GOLD);
            }
        } else {
            msg_print(_("未知の地形アクション '{}'。", "Unknown feature action '{}'."), action);
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }
        terrain.state[idx_state].result_tag = result_tag;
    }

    return {};
}
