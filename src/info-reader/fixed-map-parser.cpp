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
#include "floor/floor-base-definitions.h"
#include "game-option/birth-options.h"
#include "info-reader/general-parser.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "info-reader/town-definition-list-reader.h"
#include "info-reader/town-map-reader.h"
#include "info-reader/town-preferences-reader.h"
#include "io/condition-expression.h"
#include "io/files-util.h"
#include "io/pref-file-expressor.h"
#include "locale/character-encoding.h"
#include "main/init-error-messages-table.h"
#include "system/angband-system.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "term/z-util.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <nlohmann/json.hpp>
#include <string>

static concptr variant = "ZANGBAND";

static parse_error_type load_town_preferences()
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    JsoncDocumentLoader loader(path_build(ANGBAND_DIR_EDIT, TOWN_PREFERENCES));
    if (!loader.is_open()) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = loader.parse();
        TownPreferencesLegend legend;
        if (const auto err = TownPreferencesReader(data).read(legend, parse_quest_legend_cell); err != PARSE_ERROR_NONE) {
            return err;
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
    JsoncDocumentLoader loader(path_build(ANGBAND_DIR_EDIT, TOWN_DEFINITION_LIST));
    if (!loader.is_open()) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = loader.parse();
        const auto mode = vanilla_town ? TownMapMode::NONE : (lite_town ? TownMapMode::LITE : TownMapMode::NORMAL);
        return TownDefinitionListReader(data).read(AngbandWorld::get_instance().get_town_index(), mode, map_file);
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 町のマップの条件式の変数の値を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。知らない変数なら tl::nullopt
 */
static tl::optional<std::string> resolve_town_map_variable(PlayerType *player_ptr, std::string_view name)
{
    if (name == "TOWN") {
        return std::to_string(AngbandWorld::get_instance().get_town_index());
    }
    if (name == "LEVEL") {
        return std::to_string(player_ptr->lev);
    }
    if (name == "QUEST_NUMBER") {
        return std::to_string(enum2i(player_ptr->current_floor_ptr->quest_number));
    }
    if (name == "LEAVING_QUEST") {
        return std::to_string(enum2i(leaving_quest));
    }
    // 接頭辞の後ろの番号のクエストを返す
    const auto get_quest_after = [name](std::string_view prefix) -> const QuestType & {
        const std::string number(name.substr(prefix.length()));
        return QuestList::get_instance().get_quest(i2enum<QuestId>(atoi(number.data())));
    };
    constexpr std::string_view quest_type_prefix = "QUEST_TYPE";
    if (name.starts_with(quest_type_prefix)) {
        return std::to_string(enum2i(get_quest_after(quest_type_prefix).type));
    }
    constexpr std::string_view quest_prefix = "QUEST";
    if (name.starts_with(quest_prefix)) {
        return std::to_string(enum2i(get_quest_after(quest_prefix).status));
    }
    constexpr std::string_view random_prefix = "RANDOM";
    if (name.starts_with(random_prefix)) {
        const auto &system = AngbandSystem::get_instance();
        return std::to_string(static_cast<int>(system.get_seed_town()) % std::stoi(std::string(name.substr(random_prefix.length()))));
    }
    if (name == "VARIANT") {
        return variant;
    }
    if (name == "WILDERNESS") {
        if (vanilla_town) {
            return "NONE";
        }
        if (lite_town) {
            return "LITE";
        }
        return "NORMAL";
    }
    if (name == "IRONMAN_DOWNWARD") {
        return ironman_downward ? "1" : "0";
    }

    return resolve_common_expression_variable(player_ptr, name);
}

static bool is_town_map_condition_met(PlayerType *player_ptr, const std::optional<std::string> &condition)
{
    if (!condition) {
        return true;
    }

    const auto resolve = [player_ptr](std::string_view name) { return resolve_town_map_variable(player_ptr, name); };
    return evaluate_condition_expression(*condition, resolve) != "0";
}

static parse_error_type parse_town_map_jsonc(PlayerType *player_ptr, std::string_view name)
{
    JsoncDocumentLoader loader(path_build(ANGBAND_DIR_EDIT, name));
    if (!loader.is_open()) {
        return PARSE_ERROR_GENERIC;
    }
    try {
        const auto data = loader.parse();
        TownMapDefinition definition;
        const bool only_buildings = (init_flags & INIT_ONLY_BUILDINGS) != 0;
        if (const auto err = TownMapReader(data).read(definition, MAX_HGT, MAX_WID, only_buildings); err != PARSE_ERROR_NONE) {
            return err;
        }
        auto y = 0;
        auto x = 0;
        qtwg_type qg;
        auto *qg_ptr = initialize_quest_generator_type(&qg, 0, 0, MAX_HGT, MAX_WID, &y, &x);
        for (const auto &feature : definition.features) {
            if (!is_town_map_condition_met(player_ptr, feature.condition)) {
                continue;
            }
            if (const auto err = apply_town_map_feature(*player_ptr->current_floor_ptr, feature); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
        for (const auto &building : definition.buildings) {
            if (!is_town_map_condition_met(player_ptr, building.condition)) {
                continue;
            }
            if (const auto err = apply_town_building_rule(building); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
        if (only_buildings) {
            return PARSE_ERROR_NONE;
        }
        const TownMapVariant *selected_map = nullptr;
        for (const auto &map : definition.maps) {
            if (!is_town_map_condition_met(player_ptr, map.condition)) {
                continue;
            }
            if (selected_map != nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            selected_map = &map;
        }
        if (selected_map != nullptr) {
            for (const auto &row : selected_map->rows) {
                if (const auto err = apply_fixed_map_row(player_ptr, qg_ptr, row); err != PARSE_ERROR_NONE) {
                    return err;
                }
            }
        }
        for (const auto &start : definition.starts) {
            if (!is_town_map_condition_met(player_ptr, start.condition)) {
                continue;
            }
            if (selected_map == nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            apply_fixed_map_start(player_ptr, qg_ptr, start.y, start.x);
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 町の固定マップの読み込みエラーを表示する
 * @param err エラーコード
 * @param filename 読み込んでいたファイルの名前
 */
static void report_load_error(parse_error_type err, std::string_view filename)
{
    const auto oops = ((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown";
    msg_print("Error {} ({}) loading '{}'.", enum2i(err), oops, filename);
    msg_erase();
}

/*!
 * @brief 町の定義から現在の町の固定マップを読み込んで生成し、エラーコードを返す (load_town_map() の本体)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード。失敗した場合はエラーを表示済み
 */
static parse_error_type generate_town_map(PlayerType *player_ptr)
{
    if (const auto err = load_town_preferences(); err != PARSE_ERROR_NONE) {
        report_load_error(err, TOWN_PREFERENCES);
        return err;
    }

    std::string map_file;
    if (const auto err = load_town_definition_file(map_file); err != PARSE_ERROR_NONE) {
        report_load_error(err, TOWN_DEFINITION_LIST);
        return err;
    }
    if (map_file.empty()) {
        return PARSE_ERROR_NONE;
    }
    const auto err = parse_town_map_jsonc(player_ptr, map_file);
    if (err != PARSE_ERROR_NONE) {
        report_load_error(err, map_file);
    }
    return err;
}

/*!
 * @brief 町の定義 (TownDefinitionList.jsonc) から現在の町の固定マップを読み込んでフロア全体に生成し、失敗したら終了する
 * @details 町を生成できないとゲームを続けられないので、読み込みに失敗した場合は、他のデータの読み込みと同じく終了する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void load_town_map(PlayerType *player_ptr)
{
    if (generate_town_map(player_ptr) != PARSE_ERROR_NONE) {
        quit(_("町の定義の読み込みに失敗しました", "Failed to load the town definition"));
    }
}
