/*!
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2014 Deskull rearranged comment for Doxygen
 */

#include "info-reader/fixed-map-parser.h"
#include "dungeon/quest.h"
#include "floor/fixed-map-generator.h"
#include "game-option/birth-options.h"
#include "game-option/runtime-arguments.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "io/files-util.h"
#include "locale/character-encoding.h"
#include "main/init-error-messages-table.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

static concptr variant = "ZANGBAND";

static bool is_valid_town_building_integer(std::string_view value)
{
    int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && error == std::errc{} && end == value.data() + value.size();
}

static bool is_town_map_integer_in_range(std::string_view value, int minimum, int maximum)
{
    int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && error == std::errc{} && end == value.data() + value.size() && parsed >= minimum && parsed <= maximum;
}

static bool is_town_map_unsigned_integer_in_range(std::string_view value, unsigned int maximum)
{
    unsigned int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && error == std::errc{} && end == value.data() + value.size() && parsed <= maximum;
}

static bool is_valid_town_map_feature_token(std::string_view field, std::string_view token)
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
        return is_town_map_unsigned_integer_in_range(token.substr(1), static_cast<unsigned int>(maximum));
    }
    if (token.starts_with('*')) {
        const auto random_maximum = short_field ? maximum : std::numeric_limits<int>::max();
        return token.size() == 1 || is_town_map_unsigned_integer_in_range(token.substr(1), static_cast<unsigned int>(random_maximum));
    }
    return is_town_map_integer_in_range(token, minimum, maximum);
}

static bool is_valid_town_special(const nlohmann::json &value)
{
    if (!value.is_number_integer()) {
        return false;
    }

    constexpr auto minimum = std::numeric_limits<int16_t>::min();
    constexpr auto maximum = std::numeric_limits<int16_t>::max();
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() <= static_cast<uint64_t>(maximum);
    }

    const auto special = value.get<int64_t>();
    return special >= minimum && special <= maximum;
}

static parse_error_type load_town_preferences()
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    std::ifstream ifs(path_build(ANGBAND_DIR_EDIT, TOWN_PREFERENCES));
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("legend") || !data["legend"].is_object() || data["legend"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        std::vector<std::pair<unsigned char, dungeon_grid>> legend;
        for (const auto &[symbol, cell_data] : data["legend"].items()) {
            if (symbol.size() != 1 || symbol.front() < '!' || symbol.front() > '~' || !cell_data.is_object() ||
                !cell_data.contains("terrain") || !cell_data["terrain"].is_string() || !cell_data.contains("caveInfo") || !cell_data["caveInfo"].is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            for (const auto &flag : cell_data["caveInfo"]) {
                if (!flag.is_string()) {
                    return PARSE_ERROR_INVALID_TYPE;
                }
            }
            if (cell_data.contains("special") && !is_valid_town_special(cell_data["special"])) {
                return PARSE_ERROR_INVALID_VALUE;
            }

            QuestLegendCell cell;
            if (const auto err = parse_quest_legend_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
                return err;
            }
            legend.emplace_back(static_cast<unsigned char>(symbol.front()), cell.grid);
        }

        for (const auto &[symbol, grid] : legend) {
            letter[symbol] = grid;
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

static parse_error_type load_town_definition_file(std::string &map_file)
{
    std::ifstream ifs(path_build(ANGBAND_DIR_EDIT, TOWN_DEFINITION_LIST));
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("towns") || !data["towns"].is_object() || data["towns"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        const auto &towns = data["towns"];
        const auto town = towns.find(std::to_string(AngbandWorld::get_instance().get_town_index()));
        if (town == towns.end()) {
            return PARSE_ERROR_NONE;
        }

        const nlohmann::json *selected = &*town;
        if (selected->is_object()) {
            const auto *mode = vanilla_town ? "none" : (lite_town ? "lite" : "normal");
            const auto file = selected->find(mode);
            if (file == selected->end()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            selected = &*file;
        }

        if (!selected->is_string()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        map_file = selected->get<std::string>();
        if (!map_file.starts_with("towns/") || !map_file.ends_with(".jsonc") || map_file.find("..") != std::string::npos || map_file.find('\\') != std::string::npos) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

static std::string parse_fixed_map_expression(PlayerType *player_ptr, char **sp, char *fp);

static bool is_town_map_condition_met(PlayerType *player_ptr, const nlohmann::json &when)
{
    if (when.is_null()) {
        return true;
    }
    if (!when.is_string()) {
        return false;
    }

    auto expression = when.get<std::string>();
    auto *source = expression.data();
    char flag;
    return parse_fixed_map_expression(player_ptr, &source, &flag) != "0";
}

static bool is_valid_town_map_coordinate(const nlohmann::json &value, size_t maximum)
{
    if (!value.is_number_integer()) {
        return false;
    }
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() < maximum;
    }

    const auto coordinate = value.get<int64_t>();
    return coordinate >= 0 && static_cast<uint64_t>(coordinate) < maximum;
}

static bool is_valid_town_map_integer(const nlohmann::json &value, int minimum, int maximum)
{
    if (!value.is_number_integer()) {
        return false;
    }
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() <= static_cast<uint64_t>(maximum);
    }

    const auto integer = value.get<int64_t>();
    return integer >= minimum && integer <= maximum;
}

static parse_error_type parse_town_map_jsonc(PlayerType *player_ptr, std::string_view name, int ymin, int xmin, int ymax, int xmax)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, name);
    std::ifstream ifs(path);
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 2) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto *field : { "featureRules", "mapVariants", "buildingRules", "startingPositions" }) {
            if (!data.contains(field) || !data[field].is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
        }
        if (data["mapVariants"].empty() || data["startingPositions"].empty()) {
            return PARSE_ERROR_INVALID_VALUE;
        }

        auto y = ymin;
        auto x = xmin;
        qtwg_type qg;
        auto *qg_ptr = initialize_quest_generator_type(&qg, ymin, xmin, ymax, xmax, &y, &x);

        for (const auto &rule : data["featureRules"]) {
            if (!rule.is_object() || !rule.contains("symbol") || !rule["symbol"].is_string() || !rule.contains("definition") || !rule["definition"].is_object()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            const auto symbol = rule["symbol"].get<std::string>();
            if (symbol.size() != 1 || symbol.front() < ' ' || symbol.front() > '~' || symbol.front() == ':' || symbol.front() == '/' || symbol.front() == '\\') {
                return PARSE_ERROR_INVALID_VALUE;
            }
            if (rule.contains("when") && (!rule["when"].is_string() || rule["when"].get_ref<const std::string &>().empty())) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            const auto &definition = rule["definition"];
            for (const auto *field : { "terrain", "monster", "object", "ego", "artifact", "trap" }) {
                if (!definition.contains(field) || !definition[field].is_string()) {
                    return PARSE_ERROR_INVALID_TYPE;
                }
                const auto &value = definition[field].get_ref<const std::string &>();
                if (value.find_first_of(":/\\\r\n") != std::string::npos ||
                    std::any_of(value.begin(), value.end(), [](unsigned char character) { return character < ' ' || character > '~'; })) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
                if (field != std::string_view("terrain") && field != std::string_view("trap") && !is_valid_town_map_feature_token(field, value)) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
            }
            if (!definition.contains("caveInfo") || !is_valid_town_map_integer(definition["caveInfo"], 0, std::numeric_limits<int>::max()) ||
                !definition.contains("special") || !is_valid_town_map_integer(definition["special"], std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            if (rule.contains("when") && !is_town_map_condition_met(player_ptr, rule["when"])) {
                continue;
            }
            auto line = "F:" + symbol;
            line += ":" + definition["terrain"].get<std::string>();
            line += ":" + std::to_string(definition["caveInfo"].get<int>());
            for (const auto *field : { "monster", "object", "ego", "artifact", "trap" }) {
                line += ":" + definition[field].get<std::string>();
            }
            line += ":" + std::to_string(definition["special"].get<int>());
            qg_ptr->buf = line.data();
            if (const auto err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map); err != PARSE_ERROR_NONE) {
                return err;
            }
        }

        for (const auto &rule : data["buildingRules"]) {
            if (!rule.is_object() || !rule.contains("index") || !rule["index"].is_number_integer() || !rule.contains("locale") ||
                !rule["locale"].is_string() || !rule.contains("command") || !rule["command"].is_string() || !rule.contains("fields") || !rule["fields"].is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            if (!is_valid_town_map_coordinate(rule["index"], MAX_BUILDINGS)) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            if (rule.contains("when") && (!rule["when"].is_string() || rule["when"].get_ref<const std::string &>().empty())) {
                return PARSE_ERROR_INVALID_TYPE;
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
            if (command == "A" && (!fields[0].is_string() || !is_valid_town_building_integer(fields[0].get_ref<const std::string &>()) ||
                                      std::stoi(fields[0].get<std::string>()) < 0 || std::stoi(fields[0].get<std::string>()) >= 8)) {
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
            std::string line = "B:";
            if (locale == "en") {
                line += '$';
            }
            line += std::to_string(rule["index"].get<int>()) + ":" + command;
            size_t field_index = 0;
            for (const auto &field : fields) {
                if (!field.is_string()) {
                    return PARSE_ERROR_INVALID_TYPE;
                }
                const auto value = field.get<std::string>();
                if (value.find_first_of(":/\\\r\n") != std::string::npos) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
                const auto is_numeric_field = (command == "A" && (field_index == 0 || field_index == 2 || field_index == 3 || field_index == 5 || field_index == 6)) ||
                                              ((command == "C" || command == "M" || command == "R"));
                if (is_numeric_field && !is_valid_town_building_integer(value)) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
                line += ':';
                line += value;
                ++field_index;
            }
            if (rule.contains("when") && !is_town_map_condition_met(player_ptr, rule["when"])) {
                continue;
            }
            line = utf8_to_local(line);
            qg_ptr->buf = line.data();
            if (const auto err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map); err != PARSE_ERROR_NONE) {
                return err;
            }
        }

        if (init_flags & INIT_ONLY_BUILDINGS) {
            return PARSE_ERROR_NONE;
        }

        const nlohmann::json *selected_map = nullptr;
        for (const auto &map_variant : data["mapVariants"]) {
            if (!map_variant.is_object() || !map_variant.contains("rows") || !map_variant["rows"].is_array() || map_variant["rows"].empty()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            if (map_variant.contains("when") && (!map_variant["when"].is_string() || map_variant["when"].get_ref<const std::string &>().empty())) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            const auto &rows = map_variant["rows"];
            const auto map_height = ymax - ymin;
            const auto map_width = xmax - xmin;
            if (map_height <= 0 || map_width <= 0 || rows.size() > static_cast<size_t>(map_height) || !rows.front().is_string()) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            const auto width = rows.front().get_ref<const std::string &>().size();
            if (width == 0 || width > static_cast<size_t>(map_width)) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            for (const auto &row : rows) {
                if (!row.is_string() || row.get_ref<const std::string &>().size() != width) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
                const auto &row_text = row.get_ref<const std::string &>();
                if (std::any_of(row_text.begin(), row_text.end(), [](unsigned char cell) { return cell < ' ' || cell > '~'; })) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
            }
            if (map_variant.contains("when") && !is_town_map_condition_met(player_ptr, map_variant["when"])) {
                continue;
            }
            if (selected_map != nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            selected_map = &map_variant;
        }

        if (selected_map != nullptr) {
            const auto &rows = (*selected_map)["rows"];
            for (const auto &row : rows) {
                const auto &row_text = row.get_ref<const std::string &>();
                auto line = "D:" + row_text;
                qg_ptr->buf = line.data();
                if (const auto err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map); err != PARSE_ERROR_NONE) {
                    return err;
                }
            }
        }

        for (const auto &start : data["startingPositions"]) {
            if (!start.is_object() || !start.contains("y") || !start["y"].is_number_integer() || !start.contains("x") || !start["x"].is_number_integer()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            if (start.contains("when") && (!start["when"].is_string() || start["when"].get_ref<const std::string &>().empty())) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            for (const auto &map_variant : data["mapVariants"]) {
                const auto &rows = map_variant["rows"];
                const auto width = rows.front().get_ref<const std::string &>().size();
                if (!is_valid_town_map_coordinate(start["y"], rows.size()) || !is_valid_town_map_coordinate(start["x"], width)) {
                    return PARSE_ERROR_INVALID_VALUE;
                }
            }
            if (start.contains("when") && !is_town_map_condition_met(player_ptr, start["when"])) {
                continue;
            }
            if (selected_map == nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            auto line = "P:" + std::to_string(start["y"].get<int>()) + ":" + std::to_string(start["x"].get<int>());
            qg_ptr->buf = line.data();
            if (const auto err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)生成時の分岐処理
 * Helper function for "parse_fixed_map()"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sp
 * @param fp
 * @return エラーコード
 */
static std::string parse_fixed_map_expression(PlayerType *player_ptr, char **sp, char *fp)
{
    constexpr char b1 = '[';
    constexpr char b2 = ']';

    char f = ' ';

    char *s = (*sp);

    while (iswspace(*s)) {
        s++;
    }

    char *b = s;
    std::string v = "?o?o?";
    if (*s == b1) {
        std::string t;
        s++;
        t = parse_fixed_map_expression(player_ptr, &s, &f);
        if (t.empty()) {
            /* Nothing */
        } else if (t == "IOR") {
            v = "0";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t != "0") {
                    v = "1";
                }
            }
        } else if (t == "AND") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t == "0") {
                    v = "0";
                }
            }
        } else if (t == "NOT") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t == "1") {
                    v = "0";
                }
            }
        } else if (t == "EQU") {
            v = "0";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (t == p) {
                    v = "1";
                }
            }
        } else if (t == "LEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!p.empty() && atoi(t.data()) > atoi(p.data())) {
                    v = "0";
                }
            }
        } else if (t == "GEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!p.empty() && atoi(t.data()) < atoi(p.data())) {
                    v = "0";
                }
            }
        } else {
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }
        }

        if (f != b2) {
            v = "?x?x?";
        }
        if ((f = *s) != '\0') {
            *s++ = '\0';
        }

        (*fp) = f;
        (*sp) = s;
        return v;
    }

#ifdef JP
    while (iskanji(*s) || (isprint(*s) && !angband_strchr(" []", *s))) {
        if (iskanji(*s)) {
            s++;
        }
        s++;
    }
#else
    while (isprint(*s) && !angband_strchr(" []", *s)) {
        ++s;
    }
#endif
    if ((f = *s) != '\0') {
        *s++ = '\0';
    }

    if (*b != '$') {
        v = b;
        (*fp) = f;
        (*sp) = s;
        return v;
    }

    if (streq(b + 1, "SYS")) {
        v = ANGBAND_SYS;
    } else if (streq(b + 1, "GRAF")) {
        v = ANGBAND_GRAF;
    } else if (streq(b + 1, "MONOCHROME")) {
        if (arg_monochrome) {
            v = "ON";
        } else {
            v = "OFF";
        }
    } else if (streq(b + 1, "RACE")) {
        v = rp_ptr->title.en_string();
    } else if (streq(b + 1, "CLASS")) {
        v = cp_ptr->title.en_string();
    } else if (streq(b + 1, "REALM1")) {
        v = PlayerRealm(player_ptr).realm1().get_name().en_string();
    } else if (streq(b + 1, "REALM2")) {
        v = PlayerRealm(player_ptr).realm2().get_name().en_string();
    } else if (streq(b + 1, "PLAYER")) {
        char tmp_player_name[32]{};
        char *pn, *tpn;
        for (pn = player_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++) {
#ifdef JP
            if (iskanji(*pn)) {
                *(tpn++) = *(pn++);
                *tpn = *pn;
                continue;
            }
#endif
            *tpn = angband_strchr(" []", *pn) ? '_' : *pn;
        }

        *tpn = '\0';
        v = tmp_player_name;
    } else if (streq(b + 1, "TOWN")) {
        v = std::to_string(AngbandWorld::get_instance().get_town_index());
    } else if (streq(b + 1, "LEVEL")) {
        v = std::to_string(player_ptr->lev);
    } else if (streq(b + 1, "QUEST_NUMBER")) {
        v = std::to_string(enum2i(player_ptr->current_floor_ptr->quest_number));
    } else if (streq(b + 1, "LEAVING_QUEST")) {
        v = std::to_string(enum2i(leaving_quest));
    } else if (prefix(b + 1, "QUEST_TYPE")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 11))).type));
    } else if (prefix(b + 1, "QUEST")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 6))).status));
    } else if (prefix(b + 1, "RANDOM")) {
        const auto &system = AngbandSystem::get_instance();
        v = std::to_string((static_cast<int>(system.get_seed_town()) % std::stoi(b + 7)));
    } else if (streq(b + 1, "VARIANT")) {
        v = variant;
    } else if (streq(b + 1, "WILDERNESS")) {
        if (vanilla_town) {
            v = "NONE";
        } else if (lite_town) {
            v = "LITE";
        } else {
            v = "NORMAL";
        }
    } else if (streq(b + 1, "IRONMAN_DOWNWARD")) {
        v = (ironman_downward ? "1" : "0");
    }

    (*fp) = f;
    (*sp) = s;
    return v;
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)をq_info、t_info、w_infoから読み込んでパースする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name ファイル名
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @return エラーコード
 */
parse_error_type parse_fixed_map(PlayerType *player_ptr, std::string_view name, int ymin, int xmin, int ymax, int xmax)
{
    if (name == TOWN_DEFINITION_LIST) {
        if (const auto err = load_town_preferences(); err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) loading '{}'.", enum2i(err), oops, TOWN_PREFERENCES);
            msg_erase();
            return err;
        }

        std::string map_file;
        if (const auto err = load_town_definition_file(map_file); err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) loading '{}'.", enum2i(err), oops, TOWN_DEFINITION_LIST);
            msg_erase();
            return err;
        }
        if (map_file.empty()) {
            return PARSE_ERROR_NONE;
        }
        const auto err = parse_town_map_jsonc(player_ptr, map_file, ymin, xmin, ymax, xmax);
        if (err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) loading '{}'.", enum2i(err), oops, map_file);
            msg_erase();
        }
        return err;
    }

    const auto path = path_build(ANGBAND_DIR_EDIT, name);
    std::ifstream ifs(path);
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    auto num = 0;
    parse_error_type err = PARSE_ERROR_NONE;
    bool bypass = false;
    auto x = xmin;
    auto y = ymin;
    qtwg_type tmp_qg;
    qtwg_type *qg_ptr = initialize_quest_generator_type(&tmp_qg, ymin, xmin, ymax, xmax, &y, &x);
    std::string line;
    while (std::getline(ifs, line)) {
        num++;
        line = utf8_to_local(line);
        if (line.empty() || (std::isspace(static_cast<unsigned char>(line.front())) != 0) || line.starts_with('#')) {
            continue;
        }

        if (line.starts_with("?:")) {
            char f;
            auto *s = line.data() + 2;
            auto v = parse_fixed_map_expression(player_ptr, &s, &f);
            bypass = v == "0";
            continue;
        }

        if (bypass) {
            continue;
        }

        qg_ptr->buf = line.data();
        err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map);
        if (err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) at line {} of '{}'.", enum2i(err), oops, num, name);
            msg_print(_("'{}'を解析中。", "Parsing '{}'."), line);
            msg_erase();
            break;
        }
    }

    if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
        constexpr auto fmt = _("ファイルの読み込みに失敗しました ({})", "Failed to read file ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }

    return err;
}
