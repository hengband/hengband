#include "info-reader/town-map-reader.h"
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
    if (!value.is_number_integer()) {
        return false;
    }
    if (value.is_number_unsigned()) {
        const auto integer = value.get<uint64_t>();
        return integer <= static_cast<uint64_t>(maximum) && (minimum <= 0 || integer >= static_cast<uint64_t>(minimum));
    }
    const auto integer = value.get<int64_t>();
    return integer >= minimum && integer <= maximum;
}

parse_error_type read_condition(const nlohmann::json &data, std::optional<std::string> &condition)
{
    if (!data.contains("when")) {
        return PARSE_ERROR_NONE;
    }
    if (!data["when"].is_string() || data["when"].get_ref<const std::string &>().empty()) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    condition = data["when"].get<std::string>();
    return PARSE_ERROR_NONE;
}
}

TownMapReader::TownMapReader(const nlohmann::json &data)
    : data(data)
{
}

parse_error_type TownMapReader::read(TownMapDefinition &definition, int maximum_height, int maximum_width, bool only_buildings) const
{
    if (!this->data.is_object() || !this->data.contains("version") || !this->data["version"].is_number_integer() || this->data["version"] != 2) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    for (const auto *field : { "featureRules", "mapVariants", "buildingRules", "startingPositions" }) {
        if (!this->data.contains(field) || !this->data[field].is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
    }
    if (this->data["mapVariants"].empty() || this->data["startingPositions"].empty()) {
        return PARSE_ERROR_INVALID_VALUE;
    }

    TownMapDefinition parsed;
    if (const auto err = this->read_features(parsed); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (const auto err = this->read_buildings(parsed); err != PARSE_ERROR_NONE) {
        return err;
    }
    if (!only_buildings) {
        if (const auto err = this->read_maps(parsed, maximum_height, maximum_width); err != PARSE_ERROR_NONE) {
            return err;
        }
        if (const auto err = this->read_starts(parsed); err != PARSE_ERROR_NONE) {
            return err;
        }
    }
    definition = std::move(parsed);
    return PARSE_ERROR_NONE;
}

parse_error_type TownMapReader::read_features(TownMapDefinition &definition) const
{
    for (const auto &rule : this->data["featureRules"]) {
        if (!rule.is_object() || !rule.contains("symbol") || !rule["symbol"].is_string() || !rule.contains("definition") || !rule["definition"].is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        const auto symbol = rule["symbol"].get<std::string>();
        if (symbol.size() != 1 || symbol.front() < ' ' || symbol.front() > '~' || symbol.front() == ':' || symbol.front() == '/' || symbol.front() == '\\') {
            return PARSE_ERROR_INVALID_VALUE;
        }
        TownMapFeatureRule feature;
        feature.symbol = symbol.front();
        if (const auto err = read_condition(rule, feature.condition); err != PARSE_ERROR_NONE) {
            return err;
        }
        const auto &fields = rule["definition"];
        for (const auto *field : { "terrain", "monster", "object", "ego", "artifact", "trap" }) {
            if (!fields.contains(field) || !fields[field].is_string()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            const auto &value = fields[field].get_ref<const std::string &>();
            if (value.find_first_of(":/\\\r\n") != std::string::npos ||
                std::any_of(value.begin(), value.end(), [](unsigned char character) { return character < ' ' || character > '~'; })) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            if (field != std::string_view("terrain") && field != std::string_view("trap") && !is_feature_token(field, value)) {
                return PARSE_ERROR_INVALID_VALUE;
            }
        }
        if (!fields.contains("caveInfo") || !is_json_integer_in_range(fields["caveInfo"], 0, std::numeric_limits<int>::max()) ||
            !fields.contains("special") || !is_json_integer_in_range(fields["special"], std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        feature.terrain = fields["terrain"].get<std::string>();
        feature.cave_info = fields["caveInfo"].get<int>();
        feature.monster = fields["monster"].get<std::string>();
        feature.object = fields["object"].get<std::string>();
        feature.ego = fields["ego"].get<std::string>();
        feature.artifact = fields["artifact"].get<std::string>();
        feature.trap = fields["trap"].get<std::string>();
        feature.special = fields["special"].get<int>();
        definition.features.push_back(std::move(feature));
    }
    return PARSE_ERROR_NONE;
}

parse_error_type TownMapReader::read_buildings(TownMapDefinition &definition) const
{
    for (const auto &rule : this->data["buildingRules"]) {
        if (!rule.is_object() || !rule.contains("index") || !rule["index"].is_number_integer() || !rule.contains("locale") ||
            !rule["locale"].is_string() || !rule.contains("command") || !rule["command"].is_string() || !rule.contains("fields") || !rule["fields"].is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        if (!is_json_integer_in_range(rule["index"], 0, MAX_BUILDINGS - 1)) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        TownMapBuildingRule building;
        if (const auto err = read_condition(rule, building.condition); err != PARSE_ERROR_NONE) {
            return err;
        }
        const auto locale = rule["locale"].get<std::string>();
        if (locale != "en" && locale != "ja") {
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto command = rule["command"].get<std::string>();
        if (command.size() != 1 || std::string_view("ACMNRZ").find(command.front()) == std::string_view::npos) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto &fields = rule["fields"];
        if ((command == "N" && fields.size() != 3) || (command == "A" && fields.size() != 7)) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        if (command == "A" && (!fields[0].is_string() || !is_integer_in_range(fields[0].get_ref<const std::string &>(), 0, 7))) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        if ((command == "C" || command == "M" || command == "R") && fields.empty()) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        if ((command == "C" && fields.size() > static_cast<size_t>(PLAYER_CLASS_TYPE_MAX)) ||
            (command == "M" && fields.size() > static_cast<size_t>(MAX_MAGIC)) ||
            (command == "R" && fields.size() > static_cast<size_t>(MAX_RACES))) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        building.index = rule["index"].get<int>();
        building.english = locale == "en";
        building.command = command.front();
        size_t field_index = 0;
        for (const auto &field : fields) {
            if (!field.is_string()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            const auto &value = field.get_ref<const std::string &>();
            if (value.find_first_of(":/\\\r\n") != std::string::npos) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            const auto is_numeric_field = (command == "A" && (field_index == 0 || field_index == 2 || field_index == 3 || field_index == 5 || field_index == 6)) ||
                                          command == "C" || command == "M" || command == "R";
            if (is_numeric_field && !is_integer_in_range(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max())) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            building.fields.push_back(value);
            ++field_index;
        }
        definition.buildings.push_back(std::move(building));
    }
    return PARSE_ERROR_NONE;
}

parse_error_type TownMapReader::read_maps(TownMapDefinition &definition, int maximum_height, int maximum_width) const
{
    for (const auto &variant : this->data["mapVariants"]) {
        if (!variant.is_object() || !variant.contains("rows") || !variant["rows"].is_array() || variant["rows"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        TownMapVariant map;
        if (const auto err = read_condition(variant, map.condition); err != PARSE_ERROR_NONE) {
            return err;
        }
        const auto &rows = variant["rows"];
        if (maximum_height <= 0 || maximum_width <= 0 || rows.size() > static_cast<size_t>(maximum_height) || !rows.front().is_string()) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto width = rows.front().get_ref<const std::string &>().size();
        if (width == 0 || width > static_cast<size_t>(maximum_width)) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        for (const auto &row : rows) {
            if (!row.is_string() || row.get_ref<const std::string &>().size() != width) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            const auto &text = row.get_ref<const std::string &>();
            if (std::any_of(text.begin(), text.end(), [](unsigned char cell) { return cell < ' ' || cell > '~'; })) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            map.rows.push_back(text);
        }
        definition.maps.push_back(std::move(map));
    }
    return PARSE_ERROR_NONE;
}

parse_error_type TownMapReader::read_starts(TownMapDefinition &definition) const
{
    for (const auto &start : this->data["startingPositions"]) {
        if (!start.is_object() || !start.contains("y") || !start["y"].is_number_integer() || !start.contains("x") || !start["x"].is_number_integer()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        TownMapStartingPosition position;
        if (const auto err = read_condition(start, position.condition); err != PARSE_ERROR_NONE) {
            return err;
        }
        for (const auto &map : definition.maps) {
            if (!is_json_integer_in_range(start["y"], 0, static_cast<int>(map.rows.size()) - 1) ||
                !is_json_integer_in_range(start["x"], 0, static_cast<int>(map.rows.front().size()) - 1)) {
                return PARSE_ERROR_INVALID_VALUE;
            }
        }
        position.y = start["y"].get<int>();
        position.x = start["x"].get<int>();
        definition.starts.push_back(std::move(position));
    }
    return PARSE_ERROR_NONE;
}
