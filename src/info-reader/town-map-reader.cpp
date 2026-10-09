#include "info-reader/town-map-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/character-encoding.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-list.h"
#include "util/string-processor.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>
#include <type_traits>
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
    return info_set_integer(value, parsed, true, Range(minimum, maximum)) == PARSE_ERROR_NONE;
}

// Numeric token grammar and bounds have already been checked by is_feature_token.
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

parse_error_type apply_town_building_rule(const TownMapBuildingRule &rule)
{
    auto directive = rule.directive;
    const auto convert = [](std::string &text) {
        // 建物のC文字列として表現できず、旧char*経路でも途中で切れていた入力は適用しない。
        if (text.find('\0') != std::string::npos) {
            return false;
        }
        const auto converted = utf8_to_sys(text);
        if (!converted) {
            return false;
        }
        text = *converted;
        return true;
    };
    const auto converted = std::visit([&convert](auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, TownMapBuildingNames>) {
            return convert(value.name) && convert(value.owner_name) && convert(value.owner_race);
        } else if constexpr (std::is_same_v<T, TownMapBuildingAction>) {
            return convert(value.name) && convert(value.letter);
        } else if constexpr (std::is_same_v<T, TownMapBuildingNoop>) {
            return std::all_of(value.fields.begin(), value.fields.end(), convert);
        } else {
            return true;
        }
    },
        directive);
    if (!converted) {
        return PARSE_ERROR_INVALID_VALUE;
    }
#ifdef JP
    if (rule.english) {
#else
    if (!rule.english) {
#endif
        return PARSE_ERROR_NONE;
    }
    if (rule.index < 0 || rule.index >= MAX_BUILDINGS) {
        return PARSE_ERROR_INVALID_VALUE;
    }
    auto &building = buildings[rule.index];
    return std::visit([&building](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, TownMapBuildingNames>) {
            angband_strcpy(building.name, value.name, sizeof(building.name));
            angband_strcpy(building.owner_name, value.owner_name, sizeof(building.owner_name));
            angband_strcpy(building.owner_race, value.owner_race, sizeof(building.owner_race));
        } else if constexpr (std::is_same_v<T, TownMapBuildingAction>) {
            if (value.index < 0 || value.index >= 8) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            angband_strcpy(building.act_names[value.index], value.name, sizeof(building.act_names[value.index]));
            building.member_costs[value.index] = value.member_cost;
            building.other_costs[value.index] = value.other_cost;
            building.letters[value.index] = value.letter.empty() ? '\0' : value.letter.front();
            building.actions[value.index] = static_cast<int16_t>(value.action);
            building.action_restr[value.index] = static_cast<int16_t>(value.restriction);
        } else if constexpr (std::is_same_v<T, TownMapBuildingMembership>) {
            const auto is_realm = value.kind == TownMapBuildingMembershipKind::REALM;
            auto &members = value.kind == TownMapBuildingMembershipKind::CLASS ? building.member_class : (is_realm ? building.member_realm : building.member_race);
            const auto count = value.kind == TownMapBuildingMembershipKind::CLASS ? static_cast<size_t>(PLAYER_CLASS_TYPE_MAX) : (is_realm ? static_cast<size_t>(MAX_MAGIC) : static_cast<size_t>(MAX_RACES));
            const auto offset = is_realm ? 1U : 0U;
            if (value.values.empty() || value.values.size() > count || members.size() < count + offset) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            for (size_t i = 0; i < count; ++i) {
                members[i + offset] = i < value.values.size() ? static_cast<int16_t>(value.values[i]) : 1;
            }
        }
        return PARSE_ERROR_NONE;
    },
        directive);
}

parse_error_type apply_town_map_feature(const FloorType &floor, const TownMapFeatureRule &feature)
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    if (!is_fixed_map_symbol(static_cast<unsigned char>(feature.symbol))) {
        return PARSE_ERROR_INVALID_VALUE;
    }

    auto grid = feature.cell.grid;
    grid.set_terrain_id(TerrainTag::NONE);
    grid.set_trap_id(TerrainTag::NONE);
    const auto &terrains = TerrainList::get_instance();
    try {
        if (feature.terrain == "*") {
            grid.random |= RANDOM_FEATURE;
        } else {
            grid.feature = terrains.get_terrain_id(feature.terrain);
        }
        if (feature.trap == "*") {
            grid.random |= RANDOM_TRAP;
        } else {
            grid.trap = terrains.get_terrain_id(feature.trap);
        }
    } catch (const std::exception &) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }

    try {
        if (floor.is_in_quest() && (feature.cell.object_is_quest_reward || feature.cell.artifact_is_quest_reward)) {
            const auto &quest = QuestList::get_instance().get_quest(floor.quest_number);
            if (feature.cell.object_is_quest_reward && quest.has_reward() && !quest.is_reward_instant_artifact()) {
                grid.object = quest.get_reward_bi_id();
            }
            if (feature.cell.artifact_is_quest_reward) {
                grid.artifact = quest.get_reward().value_or(FixedArtifactId::NONE);
            }
        }
    } catch (const std::runtime_error &) {
        // 報酬のベースアイテムが未定義なら、例外を漏らさず解析エラーとして返す。
        return PARSE_ERROR_INVALID_FLAG;
    } catch (const std::out_of_range &) {
        // クエストや報酬アーティファクトが未定義の場合も、適用前に解析エラーとして返す。
        return PARSE_ERROR_INVALID_FLAG;
    }
    fixed_map_letter_at(static_cast<unsigned char>(feature.symbol)) = grid;
    return PARSE_ERROR_NONE;
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
        if (symbol.size() != 1 || !is_fixed_map_symbol(static_cast<unsigned char>(symbol.front())) || symbol.front() == ':' || symbol.front() == '/' || symbol.front() == '\\') {
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
        feature.trap = fields["trap"].get<std::string>();
        read_feature_tokens(fields, feature.cell);
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
        std::vector<std::string> parsed_fields;
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
            if (!std::all_of(text.begin(), text.end(), is_fixed_map_symbol)) {
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
