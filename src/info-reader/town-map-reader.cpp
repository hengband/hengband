#include "info-reader/town-map-reader.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/legend-ego-validator.h"
#include "info-reader/quest-reader.h"
#include "locale/language-switcher.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include "view/display-messages.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <fmt/format.h>
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

bool is_json_integer_in_range(const nlohmann::json &value, int minimum, int maximum)
{
    int parsed = 0;
    return info_set_integer(value, parsed, true, Range(minimum, maximum)).has_value();
}

bool is_cell_key(std::string_view key)
{
    return key == "terrain" || key == "trap" || key == "monster" || key == "object" ||
           key == "ego" || key == "artifact" || key == "caveInfo" || key == "special";
}

bool is_spec_field(std::string_view field)
{
    return field == "monster" || field == "object" || field == "ego" || field == "artifact" || field == "trap";
}

bool is_spec_key(std::string_view field, std::string_view key)
{
    return key == "random" || (key == "cloneOf" && field == "monster") ||
           (key == "oodLevel" && (field == "monster" || field == "object")) ||
           (key == "questReward" && (field == "object" || field == "artifact")) ||
           (key == "id" && field == "artifact");
}

/*!
 * @brief 共通解析の前に町v3の形・値だけを検証する。地形の存在は適用時に確認する。
 * @details 未知キー、空の指定、型と必須構造、値の順で検査し、Questの受理範囲は変えない。
 */
ParseErrorType validate_town_cell(const nlohmann::json &fields)
{
    for (const auto &[field, value] : fields.items()) {
        if (!is_cell_key(field)) {
            return ParseErrorType::INVALID_VALUE;
        }
        if (is_spec_field(field) && value.is_object()) {
            for (const auto &[key, ignored] : value.items()) {
                if (!is_spec_key(field, key)) {
                    return ParseErrorType::INVALID_VALUE;
                }
            }
        }
    }
    for (const auto &[field, value] : fields.items()) {
        if (is_spec_field(field) && value.is_object() && value.empty()) {
            return ParseErrorType::INVALID_VALUE;
        }
    }
    for (const auto &[field, value] : fields.items()) {
        if (field == "terrain" || (field == "trap" && !value.is_object())) {
            if (!value.is_string()) {
                return ParseErrorType::INVALID_TYPE;
            }
        } else if (field == "caveInfo") {
            if (!value.is_array() || !std::all_of(value.begin(), value.end(), [](const auto &flag) { return flag.is_string(); })) {
                return ParseErrorType::INVALID_TYPE;
            }
        } else if (field == "special" || !value.is_object()) {
            if (!value.is_number_integer()) {
                return ParseErrorType::INVALID_TYPE;
            }
        } else {
            if (!value.contains("random") && !value.contains("questReward") && !value.contains("cloneOf")) {
                return ParseErrorType::INVALID_TYPE;
            }
            for (const auto &[key, item] : value.items()) {
                if ((key == "random" || key == "questReward") ? !item.is_boolean() : !item.is_number_integer()) {
                    return ParseErrorType::INVALID_TYPE;
                }
            }
        }
    }
    for (const auto &[field, value] : fields.items()) {
        if (value.is_string()) {
            const auto &tag = value.get_ref<const std::string &>();
            if (tag.empty() || tag.find_first_of(":/\\") != std::string::npos ||
                std::any_of(tag.begin(), tag.end(), [](unsigned char c) { return c < ' ' || c > '~'; }) ||
                (field == "trap" && tag == "*")) {
                return ParseErrorType::INVALID_VALUE;
            }
        } else if (field == "caveInfo") {
            std::vector<std::string> seen;
            for (const auto &flag : value) {
                const auto name = flag.get<std::string>();
                if (std::find(seen.begin(), seen.end(), name) != seen.end()) {
                    return ParseErrorType::INVALID_VALUE;
                }
                if (!is_legend_cave_flag(name)) {
                    return ParseErrorType::INVALID_FLAG;
                }
                seen.push_back(name);
            }
        } else if (value.is_number_integer()) {
            const auto &range = field == "ego" ? LEGEND_EGO_RANGE : field == "monster" ? LEGEND_MONSTER_RANGE
                                                                                       : LEGEND_SHORT_RANGE;
            // 町の固定IDは正数に限定し、specialとegoは共通範囲全体を受理する。
            const auto minimum = field == "special" || field == "ego" ? range.first : 1;
            if (!is_json_integer_in_range(value, minimum, range.second)) {
                return ParseErrorType::INVALID_VALUE;
            }
        } else if (value.is_object()) {
            const auto modes = static_cast<int>(value.contains("random")) + static_cast<int>(value.contains("cloneOf")) + static_cast<int>(value.contains("questReward"));
            if (modes != 1 || (value.contains("oodLevel") && !value.contains("random")) ||
                (value.contains("id") && !value.contains("random"))) {
                return ParseErrorType::INVALID_VALUE;
            }
            for (const auto &[key, item] : value.items()) {
                if (key == "random" || key == "questReward") {
                    if (!item.get<bool>()) {
                        return ParseErrorType::INVALID_VALUE;
                    }
                } else {
                    const auto &range = key == "cloneOf" ? LEGEND_MONSTER_RANGE : LEGEND_SHORT_RANGE;
                    // 町の階層補正は非負、構造化指定のIDは正数に限定する。
                    if (!is_json_integer_in_range(item, key == "oodLevel" ? 0 : 1, range.second)) {
                        return ParseErrorType::INVALID_VALUE;
                    }
                }
            }
        }
    }
    return ParseErrorType::NONE;
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
    if (const auto result = info_validate_version(this->data, 3); !result) {
        return result;
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
    for (const auto &[index, rule] : this->data["featureRules"].items()) {
        if (!rule.is_object() || !rule.contains("symbol") || !rule["symbol"].is_string() || !rule.contains("definition") || !rule["definition"].is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto symbol = rule["symbol"].get<std::string>();
        if (!parse_fixed_map_symbol(symbol) || symbol.front() == ':' || symbol.front() == '/' || symbol.front() == '\\') {
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
        if (const auto error = validate_town_cell(fields); error != ParseErrorType::NONE) {
            return tl::make_unexpected(error);
        }
        DeferredQuestLegendCell parsed;
        std::string reason;
        if (const auto result = parse_quest_legend_cell_deferred(fields, parsed, reason); !result) {
            if (!reason.empty()) {
                if (fields.contains("trap") && fields["trap"].is_string()) {
                    const auto &tag = fields["trap"].get_ref<const std::string &>();
                    if (tag != "NONE" && tag != "*") {
                        reason += fmt::format(_(" (trap '{}' は固定の罠として扱われます。未知のタグや誤記も固定の罠になります)",
                                                  " (trap '{}' is treated as a fixed trap; unknown tags and typos are treated the same way)"),
                            tag);
                    }
                }
                msg_print(_("町の配置指定が不正です: featureRules[{}] (記号 '{}').trap: {}",
                              "Invalid town placement: featureRules[{}] (symbol '{}').trap: {}"),
                    index, symbol, reason);
            }
            return tl::make_unexpected(result.error());
        }
        if (const auto result = validate_legend_ego(parsed.cell.grid, parsed.cell.object_is_quest_reward); !result) {
            constexpr auto fmt = _("町のエゴ指定が不正です: featureRules[{}] (記号 '{}').{}: {}",
                "Invalid town ego: featureRules[{}] (symbol '{}').{}: {}");
            msg_print(fmt, index, symbol, describe_legend_ego_spec(parsed.cell.grid), describe_legend_ego_error(result.error()));
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        feature.terrain = std::move(parsed.terrain_tag);
        feature.trap = std::move(parsed.trap_tag);
        feature.cell = std::move(parsed.cell);
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
        if (maximum_height <= 0 || maximum_width <= 0 || rows.size() > static_cast<size_t>(maximum_height)) {
            return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
        }
        if (!rows.front().is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto width = rows.front().get_ref<const std::string &>().size();
        if (width > static_cast<size_t>(maximum_width)) {
            return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
        }
        if (width == 0) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        for (const auto &row : rows) {
            if (!row.is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            const auto &text = row.get_ref<const std::string &>();
            const FixedMapRowLimits limits{ static_cast<size_t>(maximum_height), static_cast<size_t>(maximum_width), ParseErrorType::OUT_OF_BOUNDS, false, width };
            if (const auto result = validate_fixed_map_row(text, map.rows.size(), limits); !result) {
                return result;
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
