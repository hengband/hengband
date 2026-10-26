#include "info-reader/wilderness-reader.h"
#include "floor/floor-base-definitions.h"
#include "game-option/birth-options.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/parse-error-messages.h"
#include "io/files-util.h"
#include "locale/language-switcher.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/floor/wilderness-grid.h"
#include "system/gamevalue.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace {
/*!
 * @brief 荒野の外周のマスが、外周の地形で町も道も無いものだけかを確かめる
 * @param map 荒野マップ
 * @param width 荒野全体の幅
 * @param height 荒野全体の高さ
 * @return 外周がすべて通れないなら true
 * @details 外周が通れると、荒野の端の区画に入ったときに外側の区画を作ろうとして荒野の範囲外を引き、例外で落ちる。
 * レイアウトに無いマスは外周の地形になるので確かめない。
 */
bool is_edge_closed(const WildernessMapDefinition &map, int width, int height)
{
    for (auto y = 0; y < std::min(height, static_cast<int>(map.layout.size())); ++y) {
        const auto &row = map.layout[y];
        for (auto x = 0; x < std::min(width, static_cast<int>(row.size())); ++x) {
            if ((y != 0) && (y != height - 1) && (x != 0) && (x != width - 1)) {
                continue;
            }
            const auto letter = std::find_if(map.letters.begin(), map.letters.end(), [symbol = row[x]](const auto &candidate) {
                return candidate.symbol == symbol;
            });
            if ((letter == map.letters.end()) || (letter->terrain != WildernessTerrain::EDGE) || (letter->town != 0) || (letter->road != 0)) {
                return false;
            }
        }
    }

    return true;
}

/*!
 * @brief 整数または選択言語の整数を、既存の範囲で検証して格納する
 * @param value 整数または言語別の値
 * @param destination 保存先
 * @param range 受理する整数範囲
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> read_localized_integer(const nlohmann::json &value, int &destination, const Range &range)
{
    if (!value.is_number_integer() && !value.is_object()) {
        return tl::make_unexpected(value.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    const auto &selected = value.is_object() ? get_json_value(value, _("ja", "en")) : value;
    if (const auto result = info_set_integer(selected, destination, true, range); !result) {
        return result;
    }
    return {};
}

//! 荒野の定義ファイルを読み込めなかった理由
struct WildernessLoadError {
    std::string message; //!< 起動時に表示する診断
    ParseErrorType code = ParseErrorType::GENERIC; //!< 定義の誤りならその種類、ファイルを開けない・構文エラーなら GENERIC
};

/*!
 * @brief 荒野の定義ファイルを読み込む
 * @return 読み込んだ定義、失敗時はファイルを開けない・構文エラー・定義の誤りを区別した理由
 */
tl::expected<WildernessDefinition, WildernessLoadError> load_wilderness_definition()
{
    const auto path = path_build(ANGBAND_DIR_EDIT, WILDERNESS_DEFINITION);
    JsoncDocumentLoader loader(path);
    if (!loader.is_open()) {
        return tl::make_unexpected(WildernessLoadError{ fmt::format(_("{}: ファイルを開けません", "{}: cannot open the file"), WILDERNESS_DEFINITION) });
    }

    try {
        const auto json_object = loader.parse();
        WildernessDefinition definition;
        if (const auto result = WildernessReader(json_object).read(definition); !result) {
            const auto code = result.error();
            return tl::make_unexpected(WildernessLoadError{
                fmt::format(_("{}: {} (エラー{})", "{}: {} (error {})"), WILDERNESS_DEFINITION, describe_parse_error(code), enum2i(code)), code });
        }

        return definition;
    } catch (const nlohmann::json::exception &e) {
        return tl::make_unexpected(WildernessLoadError{ format_json_syntax_error(WILDERNESS_DEFINITION, e.what()) });
    }
}
}

WildernessReader::WildernessReader(const nlohmann::json &data)
    : data(data)
{
}

/*!
 * @brief 荒野の定義(JSON Object)のパース関数
 * @param definition 読み込んだ荒野の定義の出力先
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details 全件の検証に成功した場合のみ出力を置き換える。
 */
tl::expected<void, ParseErrorType> WildernessReader::read(WildernessDefinition &definition) const
{
    if (!this->data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    WildernessDefinition parsed;
    int version = 0;
    if (const auto result = info_set_integer(get_json_value(this->data, "version"), version, true, Range(1, 1)); !result) {
        return result;
    }
    // 荒野全体図は荒野の1マスをフロアの1マスへ描くため、フロアの最大の大きさに収める。
    // 周囲8マスを生成できる内側のマスが要るため、幅・高さとも3以上とする。
    if (const auto result = info_set_integer(get_json_value(this->data, "width"), parsed.width, true, Range(3, MAX_WID)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->data, "height"), parsed.height, true, Range(3, MAX_HGT)); !result) {
        return result;
    }

    const auto &towns_data = get_json_value(this->data, "towns");
    if (!towns_data.is_array() || towns_data.empty()) {
        return tl::make_unexpected(towns_data.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    std::set<int> town_ids;
    for (const auto &town_data : towns_data) {
        if (!town_data.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        WildernessTownDefinition town;
        // 町の訪問記録は TownId::MAX までの町しか持たず、それより後の町に入ると例外で落ちるため、番号を限る。
        if (const auto result = info_set_integer(get_json_value(town_data, "id"), town.id, true, Range(1, enum2i(TownId::MAX))); !result) {
            return result;
        }
        if (!town_ids.insert(town.id).second) {
            return tl::make_unexpected(ParseErrorType::NON_SEQUENTIAL_RECORDS);
        }
        if (const auto result = info_set_string(get_json_value(town_data, "name"), town.name, true); !result) {
            return result;
        }
        const auto &alias_data = get_json_value(town_data, "alias");
        if (!alias_data.is_null()) {
            std::string alias;
            if (const auto result = info_set_string(alias_data, alias, true); !result) {
                return result;
            }
            town.alias = std::move(alias);
        }
        parsed.towns.push_back(std::move(town));
    }

    const auto &maps = get_json_value(this->data, "maps");
    if (!maps.is_object()) {
        return tl::make_unexpected(maps.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    if (const auto result = this->read_map(get_json_value(maps, "normal"), parsed.width, parsed.height, parsed.normal); !result) {
        return result;
    }
    if (parsed.normal.layout.size() != static_cast<size_t>(parsed.height)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    for (const auto &row : parsed.normal.layout) {
        if (row.size() != static_cast<size_t>(parsed.width)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
    }

    if (const auto result = this->read_map(get_json_value(maps, "compact"), parsed.width, parsed.height, parsed.compact); !result) {
        return result;
    }

    const auto has_undefined_town = [&town_ids](const WildernessMapDefinition &map) {
        return std::any_of(map.letters.begin(), map.letters.end(), [&town_ids](const auto &letter) {
            return letter.town != 0 && !town_ids.contains(letter.town);
        });
    };
    if (has_undefined_town(parsed.normal) || has_undefined_town(parsed.compact)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    if (!is_edge_closed(parsed.normal, parsed.width, parsed.height) || !is_edge_closed(parsed.compact, parsed.width, parsed.height)) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    definition = std::move(parsed);
    return {};
}

/*!
 * @brief 荒野マップ1件を検証し、公開前の解析結果へ格納する
 * @param map_data 読込元のマップ
 * @param width 荒野全体の幅
 * @param height 荒野全体の高さ
 * @param map 一時保存先
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> WildernessReader::read_map(const nlohmann::json &map_data, int width, int height, WildernessMapDefinition &map) const
{
    if (!map_data.is_object()) {
        return tl::make_unexpected(map_data.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    const auto &letters_data = get_json_value(map_data, "letters");
    if (!letters_data.is_array() || letters_data.empty()) {
        return tl::make_unexpected(letters_data.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    std::array<bool, 256> defined_symbols{};
    for (const auto &letter_data : letters_data) {
        if (!letter_data.is_object()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto &symbol_data = get_json_value(letter_data, "symbol");
        if (!symbol_data.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto &symbol = symbol_data.get_ref<const std::string &>();
        if (symbol.size() != 1 || symbol[0] < 0x20 || symbol[0] > 0x7e) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto symbol_index = static_cast<unsigned char>(symbol[0]);
        if (defined_symbols[symbol_index]) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        defined_symbols[symbol_index] = true;

        WildernessLetterDefinition letter{ symbol[0], WildernessTerrain::EDGE, 0, 0, 0 };
        if (const auto result = info_set_integer(get_json_value(letter_data, "terrain"), letter.terrain, true, Range(0, enum2i(WildernessTerrain::MAX) - 1)); !result) {
            return result;
        }
        const auto &level_data = get_json_value(letter_data, "level");
        if (!level_data.is_null()) {
            // 生成は階層を MAX_DEPTH - 1 で頭打ちにし、地上の階層はセーブで16ビットになるため、それより深い値は受け付けない。
            if (const auto result = read_localized_integer(level_data, letter.level, Range(0, MAX_DEPTH - 1)); !result) {
                return result;
            }
        }
        const auto &town_data = get_json_value(letter_data, "town");
        if (!town_data.is_null()) {
            if (const auto result = info_set_integer(town_data, letter.town, true, Range(0, enum2i(TownId::MAX))); !result) {
                return result;
            }
        }
        // road の値の大きさは使っていない (0 より大きければ道とみなし、1〜15 の違いはゲーム中で区別しない)。
        const auto &road_data = get_json_value(letter_data, "road");
        if (!road_data.is_null()) {
            if (const auto result = info_set_integer(road_data, letter.road, true, Range(0, 15)); !result) {
                return result;
            }
        }
        map.letters.push_back(letter);
    }

    const auto &layout_data = get_json_value(map_data, "layout");
    if (!layout_data.is_array() || layout_data.empty() || layout_data.size() > static_cast<size_t>(height)) {
        return tl::make_unexpected(layout_data.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_VALUE);
    }
    for (const auto &row_data : layout_data) {
        if (!row_data.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto &row = row_data.get_ref<const std::string &>();
        if (row.empty() || row.size() > static_cast<size_t>(width)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        for (const auto symbol : row) {
            if (!defined_symbols[static_cast<unsigned char>(symbol)]) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
        }
        map.layout.push_back(row);
    }

    const auto &position_data = get_json_value(map_data, "starting_position");
    if (!position_data.is_object()) {
        return tl::make_unexpected(position_data.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    // 地上の生成では現在地の周囲8マスを生成するため、開始位置は荒野の端を除いた内側に限る。
    if (const auto result = info_set_integer(get_json_value(position_data, "x"), map.starting_position.x, true, Range(1, width - 2)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(position_data, "y"), map.starting_position.y, true, Range(1, height - 2)); !result) {
        return result;
    }
    if (map.starting_position.y >= static_cast<int>(map.layout.size()) || map.starting_position.x >= static_cast<int>(map.layout[map.starting_position.y].size())) {
        return tl::make_unexpected(ParseErrorType::OUT_OF_BOUNDS);
    }

    return {};
}

tl::expected<void, std::string> initialize_wilderness_definition()
{
    const auto definition = load_wilderness_definition();
    if (!definition) {
        return tl::make_unexpected(definition.error().message);
    }

    if (!DungeonReader::validate_wilderness_positions(definition->height, definition->width)) {
        return tl::make_unexpected(std::string(_("荒野を初期化できません", "Cannot initialize wilderness")));
    }

    auto &towns = TownList::get_instance();
    for (const auto &town : definition->towns) {
        if (town.id >= static_cast<int>(towns.size())) {
            return tl::make_unexpected(fmt::format(_("{}: 町ID {} が範囲外です", "{}: town ID {} is out of range"), WILDERNESS_DEFINITION, town.id));
        }
        auto &destination = towns.get_town(town.id);
        destination.init_name(town.name);
        if (town.alias) {
            destination.init_name(*town.alias);
        }
    }
    if (!towns.is_all_initialized()) {
        return tl::make_unexpected(fmt::format(_("{}: 初期化されていない町があります", "{}: some towns are not initialized"), WILDERNESS_DEFINITION));
    }

    auto &wilderness = WildernessGrids::get_instance();
    wilderness.initialize_width(definition->width);
    wilderness.initialize_height(definition->height);
    wilderness.initialize_grids();
    wilderness.set_ambushes(false);
    return {};
}

tl::expected<void, ParseErrorType> apply_wilderness_definition()
{
    const auto definition = load_wilderness_definition();
    if (!definition) {
        return tl::make_unexpected(definition.error().code);
    }
    if (const auto result = DungeonReader::validate_wilderness_positions(definition->height, definition->width); !result) {
        return result;
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

    return {};
}
