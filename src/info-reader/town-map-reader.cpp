#include "info-reader/town-map-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/random-grid-effect-types.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <string_view>
#include <utility>

namespace {
bool is_integer_in_range(std::string_view value, int minimum, int maximum)
{
    int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && error == std::errc{} && end == value.data() + value.size() && parsed >= minimum && parsed <= maximum;
}

bool is_unsigned_integer_in_range(std::string_view value, unsigned int maximum)
{
    unsigned int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && error == std::errc{} && end == value.data() + value.size() && parsed <= maximum;
}

bool is_feature_token(std::string_view field, std::string_view token)
{
    const bool short_field = (field == "monster") || (field == "object") || (field == "artifact");
    const auto minimum = field == "monster" ? -std::numeric_limits<int16_t>::max() : (short_field ? std::numeric_limits<int16_t>::min() : std::numeric_limits<int>::min());
    const auto maximum = short_field ? std::numeric_limits<int16_t>::max() : std::numeric_limits<int>::max();
    if ((field == "object" || field == "artifact") && token == "!") {
        return true;
    }
    if (token == "*") {
        return true;
    }
    if (field == "monster" && token.starts_with('c')) {
        return is_unsigned_integer_in_range(token.substr(1), static_cast<unsigned int>(maximum));
    }
    if (token.starts_with('*')) {
        return is_unsigned_integer_in_range(token.substr(1), static_cast<unsigned int>(maximum));
    }
    return is_integer_in_range(token, minimum, maximum);
}

bool is_json_integer_in_range(const nlohmann::json &value, int minimum, int maximum)
{
    int parsed = 0;
    return info_set_integer(value, parsed, true, Range(minimum, maximum)).has_value();
}

// 数値トークンの文法と範囲はis_feature_tokenで検証済み。
int feature_token_value(std::string_view token)
{
    if (token.starts_with('*') || token.starts_with('c')) {
        token.remove_prefix(1);
    }
    int value = 0;
    if (!token.empty()) {
        std::from_chars(token.data(), token.data() + token.size(), value);
    }
    return value;
}

void read_feature_tokens(const nlohmann::json &fields, QuestLegendCell &cell)
{
    auto &grid = cell.grid;
    const auto &monster = fields["monster"].get_ref<const std::string &>();
    const auto &object = fields["object"].get_ref<const std::string &>();
    const auto &ego = fields["ego"].get_ref<const std::string &>();
    const auto &artifact = fields["artifact"].get_ref<const std::string &>();
    grid.monster = static_cast<MONSTER_IDX>(feature_token_value(monster) * (monster.starts_with('c') ? -1 : 1));
    cell.object_is_quest_reward = object == "!";
    cell.artifact_is_quest_reward = artifact == "!";
    grid.object = cell.object_is_quest_reward ? 0 : static_cast<OBJECT_IDX>(feature_token_value(object));
    grid.ego = i2enum<EgoType>(feature_token_value(ego));
    grid.artifact = cell.artifact_is_quest_reward ? FixedArtifactId{} : i2enum<FixedArtifactId>(feature_token_value(artifact));
    grid.cave_info = fields["caveInfo"].get<BIT_FLAGS>();
    grid.special = fields["special"].get<int16_t>();
    for (const auto &[token, flag] : { std::pair{ monster, RANDOM_MONSTER }, std::pair{ object, RANDOM_OBJECT },
             std::pair{ ego, RANDOM_EGO }, std::pair{ artifact, RANDOM_ARTIFACT } }) {
        if (token.starts_with('*')) {
            grid.random |= flag;
        }
    }
}

tl::expected<std::optional<std::string>, ParseErrorType> read_condition(const nlohmann::json &data)
{
    if (!data.contains("when")) {
        return std::optional<std::string>{};
    }
    if (!data["when"].is_string() || data["when"].get_ref<const std::string &>().empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    return data["when"].get<std::string>();
}
}

TownMapReader::TownMapReader(const nlohmann::json &data)
    : data(data)
{
}

/*!
 * @brief 町の宣言全体を検証し、成功時のみ型付き定義へ置き換える。
 * @details 宣言の配列内順序と条件式・UTF-8文字列を保持する。条件の評価、報酬解決、
 * 地形・建物への適用はfixed-map-parser側で行う。建物専用モードでも凡例と建物は
 * 全件検証し、マップと開始位置の読み取りだけを省略する。
 */
tl::expected<void, ParseErrorType> TownMapReader::read(TownMapDefinition &definition, int maximum_height, int maximum_width, bool only_buildings) const
{
    if (!this->data.is_object() || !this->data.contains("version") || !this->data["version"].is_number_integer() || this->data["version"] != 2) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    for (const auto *field : { "featureRules", "mapVariants", "buildingRules", "startingPositions" }) {
        if (!this->data.contains(field) || !this->data[field].is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
    }
    if (this->data["mapVariants"].empty() || this->data["startingPositions"].empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    TownMapDefinition parsed;
    if (const auto result = this->read_features(parsed); !result) {
        return result;
    }
    if (const auto result = this->read_buildings(parsed); !result) {
        return result;
    }
    if (!only_buildings) {
        if (const auto result = this->read_maps(parsed, maximum_height, maximum_width); !result) {
            return result;
        }
        if (const auto result = this->read_starts(parsed); !result) {
            return result;
        }
    }
    definition = std::move(parsed);
    return {};
}

tl::expected<void, ParseErrorType> TownMapReader::read_features(TownMapDefinition &definition) const
{
    for (const auto &rule : this->data["featureRules"]) {
        if (!rule.is_object() || !rule.contains("symbol") || !rule["symbol"].is_string() || !rule.contains("definition") || !rule["definition"].is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto symbol = rule["symbol"].get<std::string>();
        if (symbol.size() != 1 || !is_fixed_map_symbol(static_cast<unsigned char>(symbol.front())) || symbol.front() == ':' || symbol.front() == '/' || symbol.front() == '\\') {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        TownMapFeatureRule feature;
        feature.symbol = symbol.front();
        const auto condition = read_condition(rule);
        if (!condition) {
            return tl::make_unexpected(condition.error());
        }
        feature.condition = *condition;
        const auto &fields = rule["definition"];
        for (const auto *field : { "terrain", "monster", "object", "ego", "artifact", "trap" }) {
            if (!fields.contains(field) || !fields[field].is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            const auto &value = fields[field].get_ref<const std::string &>();
            if (value.find_first_of(":/\\\r\n") != std::string::npos ||
                std::any_of(value.begin(), value.end(), [](unsigned char character) { return character < ' ' || character > '~'; })) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            if (field != std::string_view("terrain") && field != std::string_view("trap") && !is_feature_token(field, value)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
        }
        if (!fields.contains("caveInfo") || !is_json_integer_in_range(fields["caveInfo"], 0, std::numeric_limits<int>::max()) ||
            !fields.contains("special") || !is_json_integer_in_range(fields["special"], std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        feature.terrain = fields["terrain"].get<std::string>();
        feature.trap = fields["trap"].get<std::string>();
        read_feature_tokens(fields, feature.cell);
        definition.features.push_back(std::move(feature));
    }
    return {};
}

/*!
 * @brief 建物宣言を型付きデータへ解析する。
 * @details action・restriction・会員フラグは保存先のsigned16範囲に限定する。
 * 費用はint範囲を維持し、対象言語や条件の成立にかかわらず宣言全体を検証する。
 */
tl::expected<void, ParseErrorType> TownMapReader::read_buildings(TownMapDefinition &definition) const
{
    for (const auto &rule : this->data["buildingRules"]) {
        if (!rule.is_object() || !rule.contains("index") || !rule["index"].is_number_integer() || !rule.contains("locale") ||
            !rule["locale"].is_string() || !rule.contains("command") || !rule["command"].is_string() || !rule.contains("fields") || !rule["fields"].is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        if (!is_json_integer_in_range(rule["index"], 0, MAX_BUILDINGS - 1)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        TownMapBuildingRule building;
        const auto condition = read_condition(rule);
        if (!condition) {
            return tl::make_unexpected(condition.error());
        }
        building.condition = *condition;
        const auto locale = rule["locale"].get<std::string>();
        if (locale != "en" && locale != "ja") {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto command = rule["command"].get<std::string>();
        if (command.size() != 1 || std::string_view("ACMNRZ").find(command.front()) == std::string_view::npos) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto &fields = rule["fields"];
        if ((command == "N" && fields.size() != 3) || (command == "A" && fields.size() != 7)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (command == "A" && (!fields[0].is_string() || !is_integer_in_range(fields[0].get_ref<const std::string &>(), 0, 7))) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if ((command == "C" || command == "M" || command == "R") && fields.empty()) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if ((command == "C" && fields.size() > static_cast<size_t>(PLAYER_CLASS_TYPE_MAX)) ||
            (command == "M" && fields.size() > static_cast<size_t>(MAX_MAGIC)) ||
            (command == "R" && fields.size() > static_cast<size_t>(MAX_RACES))) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        building.index = rule["index"].get<int>();
        building.english = locale == "en";
        std::vector<std::string> parsed_fields;
        size_t field_index = 0;
        for (const auto &field : fields) {
            if (!field.is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            const auto &value = field.get_ref<const std::string &>();
            if (value.find_first_of(":/\\\r\n") != std::string::npos) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            const auto is_numeric_field = (command == "A" && (field_index == 0 || field_index == 2 || field_index == 3 || field_index == 5 || field_index == 6)) ||
                                          command == "C" || command == "M" || command == "R";
            if (is_numeric_field) {
                const auto short_field = command != "A" || field_index == 5 || field_index == 6;
                const auto minimum = short_field ? std::numeric_limits<int16_t>::min() : std::numeric_limits<int>::min();
                const auto maximum = short_field ? std::numeric_limits<int16_t>::max() : std::numeric_limits<int>::max();
                if (!is_integer_in_range(value, minimum, maximum)) {
                    return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
                }
            }
            parsed_fields.push_back(value);
            ++field_index;
        }
        const auto integer = [&parsed_fields](size_t index) {
            const auto &text = parsed_fields[index];
            int value = 0;
            std::from_chars(text.data(), text.data() + text.size(), value);
            return value;
        };
        if (command == "N") {
            building.directive = TownMapBuildingNames{ parsed_fields[0], parsed_fields[1], parsed_fields[2] };
        } else if (command == "A") {
            building.directive = TownMapBuildingAction{ integer(0), parsed_fields[1], integer(2), integer(3), parsed_fields[4], integer(5), integer(6) };
        } else if (command == "Z") {
            building.directive = TownMapBuildingNoop{ std::move(parsed_fields) };
        } else {
            TownMapBuildingMembership membership;
            membership.kind = command == "C" ? TownMapBuildingMembershipKind::CLASS : (command == "R" ? TownMapBuildingMembershipKind::RACE : TownMapBuildingMembershipKind::REALM);
            for (size_t i = 0; i < parsed_fields.size(); ++i) {
                membership.values.push_back(integer(i));
            }
            building.directive = std::move(membership);
        }
        definition.buildings.push_back(std::move(building));
    }
    return {};
}

tl::expected<void, ParseErrorType> TownMapReader::read_maps(TownMapDefinition &definition, int maximum_height, int maximum_width) const
{
    for (const auto &variant : this->data["mapVariants"]) {
        if (!variant.is_object() || !variant.contains("rows") || !variant["rows"].is_array() || variant["rows"].empty()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        TownMapVariant map;
        const auto condition = read_condition(variant);
        if (!condition) {
            return tl::make_unexpected(condition.error());
        }
        map.condition = *condition;
        const auto &rows = variant["rows"];
        if (maximum_height <= 0 || maximum_width <= 0 || rows.size() > static_cast<size_t>(maximum_height) || !rows.front().is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto width = rows.front().get_ref<const std::string &>().size();
        if (width == 0 || width > static_cast<size_t>(maximum_width)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        for (const auto &row : rows) {
            if (!row.is_string() || row.get_ref<const std::string &>().size() != width) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            const auto &text = row.get_ref<const std::string &>();
            if (!std::all_of(text.begin(), text.end(), is_fixed_map_symbol)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            map.rows.push_back(text);
        }
        definition.maps.push_back(std::move(map));
    }
    return {};
}

tl::expected<void, ParseErrorType> TownMapReader::read_starts(TownMapDefinition &definition) const
{
    for (const auto &start : this->data["startingPositions"]) {
        if (!start.is_object() || !start.contains("y") || !start["y"].is_number_integer() || !start.contains("x") || !start["x"].is_number_integer()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        TownMapStartingPosition position;
        const auto condition = read_condition(start);
        if (!condition) {
            return tl::make_unexpected(condition.error());
        }
        position.condition = *condition;
        for (const auto &map : definition.maps) {
            if (!is_json_integer_in_range(start["y"], 0, static_cast<int>(map.rows.size()) - 1) ||
                !is_json_integer_in_range(start["x"], 0, static_cast<int>(map.rows.front().size()) - 1)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
        }
        position.y = start["y"].get<int>();
        position.x = start["x"].get<int>();
        definition.starts.push_back(std::move(position));
    }
    return {};
}
