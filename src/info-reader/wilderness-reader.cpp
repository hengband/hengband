#include "info-reader/wilderness-reader.h"
#include "game-option/birth-options.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "io/files-util.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/town-list.h"
#include "system/floor/wilderness-grid.h"
#include "system/gamevalue.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace {
int read_localized_integer(const nlohmann::json &value, int &destination, const Range &range)
{
    if (value.is_number_integer()) {
        return info_set_integer(value, destination, true, range);
    }
    if (!value.is_object()) {
        return value.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }

#ifdef JP
    return info_set_integer(get_json_value(value, "ja"), destination, true, range);
#else
    return info_set_integer(get_json_value(value, "en"), destination, true, range);
#endif
}

std::optional<WildernessDefinition> load_wilderness_definition()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, WILDERNESS_DEFINITION);
    std::ifstream ifs(path);
    if (!ifs) {
        return std::nullopt;
    }

    try {
        std::istreambuf_iterator<char> ifs_iter(ifs);
        std::istreambuf_iterator<char> ifs_end;
        const auto json_object = nlohmann::json::parse(ifs_iter, ifs_end, nullptr, true, true, true);
        WildernessDefinition definition;
        if (WildernessReader(json_object).read(definition) != PARSE_ERROR_NONE) {
            return std::nullopt;
        }

        return definition;
    } catch (const nlohmann::json::exception &) {
        return std::nullopt;
    }
}
}

WildernessReader::WildernessReader(const nlohmann::json &data)
    : data(data)
{
}

int WildernessReader::read(WildernessDefinition &definition) const
{
    if (!this->data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    int version;
    if (const auto err = info_set_integer(get_json_value(this->data, "version"), version, true, Range(1, 1))) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(this->data, "width"), definition.width, true, Range(2, 32767))) {
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(this->data, "height"), definition.height, true, Range(2, 32767))) {
        return err;
    }

    const auto &towns_data = get_json_value(this->data, "towns");
    if (!towns_data.is_array() || towns_data.empty()) {
        return towns_data.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }

    std::set<int> town_ids;
    for (const auto &town_data : towns_data) {
        if (!town_data.is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        WildernessTownDefinition town;
        if (const auto err = info_set_integer(get_json_value(town_data, "id"), town.id, true, Range(1, 32767))) {
            return err;
        }
        if (!town_ids.insert(town.id).second) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
        if (const auto err = info_set_string(get_json_value(town_data, "name"), town.name, true)) {
            return err;
        }
        const auto &alias_data = get_json_value(town_data, "alias");
        if (!alias_data.is_null()) {
            std::string alias;
            if (const auto err = info_set_string(alias_data, alias, true)) {
                return err;
            }
            town.alias = std::move(alias);
        }
        definition.towns.push_back(std::move(town));
    }

    const auto &maps = get_json_value(this->data, "maps");
    if (!maps.is_object()) {
        return maps.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }
    if (const auto err = this->read_map(get_json_value(maps, "normal"), definition.width, definition.height, definition.normal)) {
        return err;
    }
    if (definition.normal.layout.size() != static_cast<size_t>(definition.height)) {
        return PARSE_ERROR_INVALID_VALUE;
    }
    for (const auto &row : definition.normal.layout) {
        if (row.size() != static_cast<size_t>(definition.width)) {
            return PARSE_ERROR_INVALID_VALUE;
        }
    }

    return this->read_map(get_json_value(maps, "compact"), definition.width, definition.height, definition.compact);
}

int WildernessReader::read_map(const nlohmann::json &map_data, int width, int height, WildernessMapDefinition &map) const
{
    if (!map_data.is_object()) {
        return map_data.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }

    const auto &letters_data = get_json_value(map_data, "letters");
    if (!letters_data.is_array() || letters_data.empty()) {
        return letters_data.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }

    std::array<bool, 256> defined_symbols{};
    for (const auto &letter_data : letters_data) {
        if (!letter_data.is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        const auto &symbol_data = get_json_value(letter_data, "symbol");
        if (!symbol_data.is_string()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        const auto &symbol = symbol_data.get_ref<const std::string &>();
        if (symbol.size() != 1 || symbol[0] < 0x20 || symbol[0] > 0x7e) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto symbol_index = static_cast<unsigned char>(symbol[0]);
        if (defined_symbols[symbol_index]) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
        defined_symbols[symbol_index] = true;

        WildernessLetterDefinition letter{ symbol[0], WildernessTerrain::EDGE, 0, 0, 0 };
        if (const auto err = info_set_integer(get_json_value(letter_data, "terrain"), letter.terrain, true, Range(0, enum2i(WildernessTerrain::MAX) - 1))) {
            return err;
        }
        const auto &level_data = get_json_value(letter_data, "level");
        if (!level_data.is_null()) {
            if (const auto err = read_localized_integer(level_data, letter.level, Range(0, std::numeric_limits<int>::max()))) {
                return err;
            }
        }
        const auto &town_data = get_json_value(letter_data, "town");
        if (!town_data.is_null()) {
            if (const auto err = info_set_integer(town_data, letter.town, true, Range(0, std::numeric_limits<short>::max()))) {
                return err;
            }
        }
        const auto &road_data = get_json_value(letter_data, "road");
        if (!road_data.is_null()) {
            if (const auto err = info_set_integer(road_data, letter.road, true, Range(0, 15))) {
                return err;
            }
        }
        map.letters.push_back(letter);
    }

    const auto &layout_data = get_json_value(map_data, "layout");
    if (!layout_data.is_array() || layout_data.empty() || layout_data.size() > static_cast<size_t>(height)) {
        return layout_data.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_VALUE;
    }
    for (const auto &row_data : layout_data) {
        if (!row_data.is_string()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        const auto &row = row_data.get_ref<const std::string &>();
        if (row.empty() || row.size() > static_cast<size_t>(width)) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        for (const auto symbol : row) {
            if (!defined_symbols[static_cast<unsigned char>(symbol)]) {
                return PARSE_ERROR_INVALID_VALUE;
            }
        }
        map.layout.push_back(row);
    }

    const auto &position_data = get_json_value(map_data, "starting_position");
    if (!position_data.is_object()) {
        return position_data.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }
    if (auto err = info_set_integer(get_json_value(position_data, "x"), map.starting_position.x, true, Range(1, width - 1))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(position_data, "y"), map.starting_position.y, true, Range(1, height - 1))) {
        return err;
    }
    if (map.starting_position.y >= static_cast<int>(map.layout.size()) || map.starting_position.x >= static_cast<int>(map.layout[map.starting_position.y].size())) {
        return PARSE_ERROR_OUT_OF_BOUNDS;
    }

    return PARSE_ERROR_NONE;
}

bool initialize_wilderness_definition()
{
    const auto definition = load_wilderness_definition();
    if (!definition) {
        return false;
    }

    auto &towns = TownList::get_instance();
    for (const auto &town : definition->towns) {
        if (town.id >= static_cast<int>(towns.size())) {
            return false;
        }
        auto &destination = towns.get_town(town.id);
        destination.init_name(town.name);
        if (town.alias) {
            destination.init_name(*town.alias);
        }
    }
    if (!towns.is_all_initialized()) {
        return false;
    }

    auto &wilderness = WildernessGrids::get_instance();
    wilderness.initialize_width(definition->width);
    wilderness.initialize_height(definition->height);
    wilderness.initialize_grids();
    wilderness.set_ambushes(false);
    return true;
}

parse_error_type apply_wilderness_definition()
{
    const auto definition = load_wilderness_definition();
    if (!definition) {
        return PARSE_ERROR_GENERIC;
    }
    const auto &map = (vanilla_town || lite_town) ? definition->compact : definition->normal;

    auto &letters = WildernessLetters::get_instance();
    letters.initialize();
    for (const auto &letter_definition : map.letters) {
        auto &letter = letters.get_grid(static_cast<unsigned char>(letter_definition.symbol));
        letter.set_terrain(letter_definition.terrain);
        letter.set_level(letter_definition.level);
        letter.set_town(letter_definition.town);
        letter.set_road(letter_definition.road);
    }

    auto &wilderness = WildernessGrids::get_instance();
    for (size_t y = 0; y < map.layout.size(); ++y) {
        const auto &row = map.layout[y];
        for (size_t x = 0; x < row.size(); ++x) {
            const auto &letter = letters.get_grid(static_cast<unsigned char>(row[x]));
            wilderness.get_grid({ static_cast<int>(y), static_cast<int>(x) }).initialize(letter);
        }
    }

    if (!wilderness.has_player_located()) {
        wilderness.set_starting_player_position(map.starting_position);
        wilderness.initialize_position();
    }

    for (const auto &[dungeon_id, dungeon] : DungeonList::get_instance()) {
        if (dungeon_id == DungeonId::WILDERNESS) {
            continue;
        }
        auto &grid = wilderness.get_grid(dungeon->get_position());
        grid.set_entrance(dungeon_id);
        if (!grid.has_town()) {
            grid.set_level(dungeon->mindepth);
        }
    }

    return PARSE_ERROR_NONE;
}
