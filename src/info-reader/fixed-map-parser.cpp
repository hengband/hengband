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
#include "info-reader/town-map-condition.h"
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
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <utility>

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
            fixed_map_letter_at(symbol) = grid;
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
 * @return 変数の値。知らない変数ならtl::nullopt、数値不正・未定義Questなら解析エラー
 */
static ExpressionVariableResult resolve_town_map_variable(PlayerType *player_ptr, std::string_view name)
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
    const auto get_quest_after = [name](std::string_view prefix) -> const QuestType * {
        const auto number = parse_condition_expression_integer(name.substr(prefix.length()));
        if (!number || (*number < 0) || !std::in_range<std::underlying_type_t<QuestId>>(*number)) {
            return nullptr;
        }
        const auto id = i2enum<QuestId>(*number);
        const auto &quests = QuestList::get_instance();
        return quests.contains(id) ? &quests.get_quest(id) : nullptr;
    };
    constexpr std::string_view quest_type_prefix = "QUEST_TYPE";
    if (name.starts_with(quest_type_prefix)) {
        const auto *quest = get_quest_after(quest_type_prefix);
        if (quest == nullptr) {
            return tl::make_unexpected(ConditionExpressionError::INVALID_VARIABLE);
        }
        return std::to_string(enum2i(quest->type));
    }
    constexpr std::string_view quest_prefix = "QUEST";
    if (name.starts_with(quest_prefix)) {
        const auto *quest = get_quest_after(quest_prefix);
        if (quest == nullptr) {
            return tl::make_unexpected(ConditionExpressionError::INVALID_VARIABLE);
        }
        return std::to_string(enum2i(quest->status));
    }
    constexpr std::string_view random_prefix = "RANDOM";
    if (name.starts_with(random_prefix)) {
        const auto divisor = parse_condition_expression_integer(name.substr(random_prefix.length()));
        if (!divisor || (*divisor <= 0)) {
            return tl::make_unexpected(ConditionExpressionError::INVALID_VARIABLE);
        }
        const auto &system = AngbandSystem::get_instance();
        return std::to_string(system.get_seed_town() % static_cast<uint32_t>(*divisor));
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

/*!
 * @brief 町の条件を判定し、変数名の数値不正を条件不成立と区別する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param condition 条件式。省略時は成立
 * @return 条件の成立状態。町変数の数値不正・未定義Quest参照は解析エラー
 * @details 未知の変数や式自体のエラーは、従来どおり条件不成立とする。
 */
tl::expected<bool, ConditionExpressionFailure> is_town_map_condition_met(PlayerType *player_ptr, const std::optional<std::string> &condition)
{
    if (!condition) {
        return true;
    }
    const auto result = evaluate_condition_expression_detailed(*condition, [player_ptr](std::string_view name) {
        return resolve_town_map_variable(player_ptr, name);
    });
    if (!result) {
        if (result.error().cause == ConditionExpressionError::INVALID_VARIABLE) {
            return tl::make_unexpected(result.error());
        }
        return false;
    }
    return *result != "0";
}

/*! @brief ファイル・JSON項目・不正な式と変数名を含む町条件式の診断を作る。 */
std::string format_town_map_condition_error(std::string_view filename, std::string_view location, std::string_view expression, std::string_view variable_name)
{
    return fmt::format("Invalid condition in '{}', {}: '{}' (variable '${}').", filename, location, expression, variable_name);
}

/*! @brief 町の各規則の条件を評価し、不正な変数は位置を示して解析エラーにする。 */
template <typename Rules, typename Apply>
static parse_error_type apply_town_rules(PlayerType *player_ptr, std::string_view filename, std::string_view array_name, const Rules &rules, Apply apply)
{
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto &rule = rules[i];
        const auto condition = is_town_map_condition_met(player_ptr, rule.condition);
        if (!condition) {
            msg_print(format_town_map_condition_error(filename, fmt::format("{}[{}].when", array_name, i), *rule.condition, condition.error().variable));
            return PARSE_ERROR_INVALID_VALUE;
        }
        if (!*condition) {
            continue;
        }
        if (const auto err = apply(rule); err != PARSE_ERROR_NONE) {
            return err;
        }
    }
    return PARSE_ERROR_NONE;
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
        const auto apply_features = [player_ptr](const auto &feature) { return apply_town_map_feature(*player_ptr->current_floor_ptr, feature); };
        if (const auto err = apply_town_rules(player_ptr, name, "featureRules", definition.features, apply_features); err != PARSE_ERROR_NONE) {
            return err;
        }
        if (const auto err = apply_town_rules(player_ptr, name, "buildingRules", definition.buildings, apply_town_building_rule); err != PARSE_ERROR_NONE) {
            return err;
        }
        if (only_buildings) {
            return PARSE_ERROR_NONE;
        }
        const TownMapVariant *selected_map = nullptr;
        const auto select_map = [&selected_map](const auto &map) {
            if (selected_map != nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            selected_map = &map;
            return PARSE_ERROR_NONE;
        };
        if (const auto err = apply_town_rules(player_ptr, name, "mapVariants", definition.maps, select_map); err != PARSE_ERROR_NONE) {
            return err;
        }
        if (selected_map != nullptr) {
            for (const auto &row : selected_map->rows) {
                if (const auto err = apply_fixed_map_row(player_ptr, qg_ptr, row); err != PARSE_ERROR_NONE) {
                    return err;
                }
            }
        }
        const auto apply_start = [player_ptr, qg_ptr, selected_map](const auto &start) {
            if (selected_map == nullptr) {
                return PARSE_ERROR_INVALID_VALUE;
            }
            apply_fixed_map_start(player_ptr, qg_ptr, start.y, start.x);
            return PARSE_ERROR_NONE;
        };
        return apply_town_rules(player_ptr, name, "startingPositions", definition.starts, apply_start);
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
