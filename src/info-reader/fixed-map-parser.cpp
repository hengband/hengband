/*!
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2014 Deskull rearranged comment for Doxygen
 */

#include "info-reader/fixed-map-parser.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/fixed-map-generator.h"
#include "floor/floor-base-definitions.h"
#include "game-option/birth-options.h"
#include "info-reader/general-parser.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/parse-error-messages.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "info-reader/town-definition-list-reader.h"
#include "info-reader/town-map-condition.h"
#include "info-reader/town-map-reader.h"
#include "info-reader/town-preferences-reader.h"
#include "io/condition-expression.h"
#include "io/files-util.h"
#include "io/pref-file-expressor.h"
#include "locale/character-encoding.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/angband-system.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/building-type-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-list.h"
#include "term/z-util.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>
#include <tl/expected.hpp>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

static concptr variant = "ZANGBAND";

/*!
 * @brief 条件評価済みの建物宣言を文字コード変換し、対象言語の建物へ適用する。
 * @param rule 読込検証と条件評価を済ませた建物宣言
 * @return 成功時は空の値、失敗時は解析エラーコード。対象外言語の建物は、文字列を検証した後に
 * 何もせず成功とする。文字列変換の失敗、建物・行動番号の範囲外、会員配列の長さや
 * signed16格納値の不正はParseErrorType::INVALID_VALUE。
 * @details 言語判定より先に文字列変換を検証する。会員フラグは全件検証してから
 * 反映する。宣言順と条件評価は呼び出し側が管理する。
 */
tl::expected<void, ParseErrorType> apply_town_building_rule(const TownMapBuildingRule &rule)
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
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
#ifdef JP
    if (rule.english) {
#else
    if (!rule.english) {
#endif
        return {};
    }
    if (rule.index < 0 || rule.index >= MAX_BUILDINGS) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    auto &building = buildings[rule.index];
    return std::visit([&building](const auto &value) -> tl::expected<void, ParseErrorType> {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, TownMapBuildingNames>) {
            angband_strcpy(building.name, value.name, sizeof(building.name));
            angband_strcpy(building.owner_name, value.owner_name, sizeof(building.owner_name));
            angband_strcpy(building.owner_race, value.owner_race, sizeof(building.owner_race));
        } else if constexpr (std::is_same_v<T, TownMapBuildingAction>) {
            if (value.index < 0 || value.index >= 8 || !std::in_range<int16_t>(value.action) || !std::in_range<int16_t>(value.restriction)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
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
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            // 最後の値が不正でも、先頭の会員フラグを部分的に書き換えない。
            if (!std::all_of(value.values.begin(), value.values.end(), [](int number) { return std::in_range<int16_t>(number); })) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            for (size_t i = 0; i < count; ++i) {
                members[i + offset] = i < value.values.size() ? static_cast<int16_t>(value.values[i]) : 1;
            }
        }
        return {};
    },
        directive);
}

/*!
 * @brief 条件評価済みの町凡例の地形とクエスト報酬を解決し、letter[]へ適用する。
 * @param floor クエスト報酬を解決する生成対象フロア
 * @param feature 読込検証と条件評価を済ませた町凡例
 * @return 成功時は空の値、失敗時は解析エラーコード。建物専用モードでは何もせず成功とする。
 * 地形・トラップタグの解決失敗はParseErrorType::UNDEFINED_TERRAIN_TAG。
 * 無効な記号、未定義のクエスト・報酬参照、報酬解決失敗はParseErrorType::INVALID_VALUE。
 * @details 建物専用モードでは何もしない。地形や報酬の解決に失敗した場合は
 * この凡例の既存letter[]を保持する。宣言順と条件評価は呼び出し側が管理する。
 */
tl::expected<void, ParseErrorType> apply_town_map_feature(const FloorType &floor, const TownMapFeatureRule &feature)
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return {};
    }

    if (!is_fixed_map_symbol(static_cast<unsigned char>(feature.symbol))) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    auto grid = feature.cell.grid;
    grid.set_terrain_id(TerrainTag::NONE);
    grid.set_trap_id(TerrainTag::NONE);
    if (const auto result = resolve_legend_terrain(feature.terrain, grid.feature, grid.random, RANDOM_FEATURE); !result) {
        return result;
    }
    if (const auto result = resolve_legend_terrain(feature.trap, grid.trap, grid.random, RANDOM_TRAP); !result) {
        return result;
    }

    try {
        if (floor.is_in_quest() && (feature.cell.object_is_quest_reward || feature.cell.artifact_is_quest_reward)) {
            const auto &quests = QuestList::get_instance();
            if (!quests.contains(floor.quest_number)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            const auto &quest = quests.get_quest(floor.quest_number);
            const auto reward = quest.get_reward();
            if (reward && !ArtifactList::get_instance().contains(*reward)) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            resolve_legend_reward(grid, feature.cell, quest);
        }
    } catch (const std::exception &) {
        // 報酬解決に失敗した場合は、地形テンプレートを変更せず解析エラーとして返す。
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    fixed_map_letter_at(static_cast<unsigned char>(feature.symbol)) = grid;
    return {};
}

/*!
 * @brief JSONC ファイルを読み込めなかった理由を町の読み込みのエラーコードに変える
 * @param error 読み込めなかった理由
 * @return 開けない時は GENERIC、解析器の例外は INVALID_VALUE（例外の文言は使わない）
 */
static ParseErrorType to_town_load_error(const JsoncLoadError &error)
{
    return error.kind == JsoncLoadErrorKind::CANNOT_OPEN ? ParseErrorType::GENERIC : ParseErrorType::INVALID_VALUE;
}

static tl::expected<void, ParseErrorType> load_town_preferences()
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return {};
    }

    const auto data = JsoncDocumentLoader::load(path_build(ANGBAND_DIR_EDIT, TOWN_PREFERENCES));
    if (!data) {
        return tl::make_unexpected(to_town_load_error(data.error()));
    }

    try {
        TownPreferencesLegend legend;
        if (const auto result = TownPreferencesReader(*data).read(legend, parse_quest_legend_cell); !result) {
            return result;
        }

        for (const auto &[symbol, grid] : legend) {
            fixed_map_letter_at(symbol) = grid;
        }
        return {};
    } catch (const nlohmann::json::exception &) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
}

/*!
 * @brief 町の定義の一覧から、現在の町のマップのファイル名を読む
 * @return マップのファイル名（一覧に現在の町が無ければ空文字列）、またはエラーコード
 */
static tl::expected<std::string, ParseErrorType> load_town_definition_file()
{
    const auto data = JsoncDocumentLoader::load(path_build(ANGBAND_DIR_EDIT, TOWN_DEFINITION_LIST));
    if (!data) {
        return tl::make_unexpected(to_town_load_error(data.error()));
    }

    try {
        const auto mode = vanilla_town ? TownMapMode::NONE : (lite_town ? TownMapMode::LITE : TownMapMode::NORMAL);
        std::string map_file;
        if (const auto result = TownDefinitionListReader(*data).read(AngbandWorld::get_instance().get_town_index(), mode, map_file); !result) {
            return tl::make_unexpected(result.error());
        }
        return map_file;
    } catch (const nlohmann::json::exception &) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
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
static tl::expected<void, ParseErrorType> apply_town_rules(PlayerType *player_ptr, std::string_view filename, std::string_view array_name, const Rules &rules, Apply apply)
{
    for (size_t i = 0; i < rules.size(); ++i) {
        const auto &rule = rules[i];
        const auto condition = is_town_map_condition_met(player_ptr, rule.condition);
        if (!condition) {
            msg_print(format_town_map_condition_error(filename, fmt::format("{}[{}].when", array_name, i), *rule.condition, condition.error().variable));
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (!*condition) {
            continue;
        }
        if (const auto result = apply(rule); !result) {
            return result;
        }
    }
    return {};
}

static tl::expected<void, ParseErrorType> parse_town_map_jsonc(PlayerType *player_ptr, std::string_view name)
{
    const auto data = JsoncDocumentLoader::load(path_build(ANGBAND_DIR_EDIT, name));
    if (!data) {
        return tl::make_unexpected(to_town_load_error(data.error()));
    }
    try {
        TownMapDefinition definition;
        const bool only_buildings = (init_flags & INIT_ONLY_BUILDINGS) != 0;
        if (const auto result = TownMapReader(*data).read(definition, MAX_HGT, MAX_WID, only_buildings); !result) {
            return result;
        }
        auto y = 0;
        auto x = 0;
        qtwg_type qg;
        auto *qg_ptr = initialize_quest_generator_type(&qg, 0, 0, MAX_HGT, MAX_WID, &y, &x);
        const auto apply_features = [player_ptr](const auto &feature) { return apply_town_map_feature(*player_ptr->current_floor_ptr, feature); };
        if (const auto result = apply_town_rules(player_ptr, name, "featureRules", definition.features, apply_features); !result) {
            return result;
        }
        if (const auto result = apply_town_rules(player_ptr, name, "buildingRules", definition.buildings, apply_town_building_rule); !result) {
            return result;
        }
        if (only_buildings) {
            return {};
        }
        const TownMapVariant *selected_map = nullptr;
        const auto select_map = [&selected_map](const auto &map) -> tl::expected<void, ParseErrorType> {
            if (selected_map != nullptr) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            selected_map = &map;
            return {};
        };
        if (const auto result = apply_town_rules(player_ptr, name, "mapVariants", definition.maps, select_map); !result) {
            return result;
        }
        if (selected_map != nullptr) {
            for (const auto &row : selected_map->rows) {
                if (const auto result = apply_fixed_map_row(player_ptr, qg_ptr, row); !result) {
                    return result;
                }
            }
        }
        const auto apply_start = [player_ptr, qg_ptr, selected_map](const auto &start) -> tl::expected<void, ParseErrorType> {
            if (selected_map == nullptr) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            apply_fixed_map_start(player_ptr, qg_ptr, start.y, start.x);
            return {};
        };
        return apply_town_rules(player_ptr, name, "startingPositions", definition.starts, apply_start);
    } catch (const nlohmann::json::exception &) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
}

/*!
 * @brief 町の固定マップの読み込みエラーを捕捉先へ報告する
 * @param err エラーコード
 * @param filename 読み込んでいたファイルの名前
 */
static void report_load_error(ParseErrorType err, std::string_view filename)
{
    msg_print("Error {} ({}) loading '{}'.", enum2i(err), describe_parse_error(err), filename);
}

/*!
 * @brief 町の定義から現在の町の固定マップを読み込んで生成し、失敗はエラーコードで返す (load_town_map() の本体)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功、またはエラーコード。失敗した場合は診断をメッセージの出力先（捕捉中は捕捉先）へ出力済み
 */
static tl::expected<void, ParseErrorType> generate_town_map(PlayerType *player_ptr)
{
    if (const auto result = load_town_preferences(); !result) {
        report_load_error(result.error(), TOWN_PREFERENCES);
        return result;
    }

    const auto map_file = load_town_definition_file();
    if (!map_file) {
        report_load_error(map_file.error(), TOWN_DEFINITION_LIST);
        return tl::make_unexpected(map_file.error());
    }
    if (map_file->empty()) {
        return {};
    }
    const auto result = parse_town_map_jsonc(player_ptr, *map_file);
    if (!result) {
        report_load_error(result.error(), *map_file);
    }
    return result;
}

/*!
 * @brief 町の定義 (TownDefinitionList.jsonc) から現在の町の固定マップを読み込んでフロア全体に生成し、失敗したら終了する
 * @details 町を生成できないとゲームを続けられないので、読み込みに失敗した場合は、他のデータの読み込みと同じく終了する。
 * 起動時 (ゲーム開始前) はメッセージごとに入力を待つため、読み込みと生成の間のメッセージは捕捉する。
 * 失敗した場合は捕捉した診断を終了の理由にまとめて表示し、成功した場合は捕捉を終えてから表示し直す。
 * 失敗の報告の経路にメッセージの消去など入力を待つ処理を足さないこと。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void load_town_map(PlayerType *player_ptr)
{
    std::vector<std::string> messages;
    {
        // 起動時・町への移動時とも、致命的な診断で入力を待たず終了理由にまとめる。
        ScopedMessageCapture diagnostics;
        if (!generate_town_map(player_ptr)) {
            quit(format_definition_load_error(diagnostics.messages(), _("町の定義の読み込みに失敗しました", "Failed to load the town definition")));
        }
        messages = diagnostics.messages();
    }
    // 生成に成功した場合の通常メッセージは、捕捉を終えてから表示する。
    for (const auto &message : messages) {
        msg_print(message);
    }
}
