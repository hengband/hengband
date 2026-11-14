/*!
 * @brief 町のマップの条件式の判定
 */

#include "info-reader/town-map-condition.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "io/pref-file-expressor.h"
#include "system/angband-system.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <cstdint>
#include <fmt/format.h>
#include <type_traits>
#include <utility>

namespace {
concptr variant = "ZANGBAND";

/*!
 * @brief 町のマップの条件式の変数の値を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。知らない変数ならtl::nullopt、数値不正・未定義Questなら解析エラー
 */
ExpressionVariableResult resolve_town_map_variable(PlayerType *player_ptr, std::string_view name)
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
}

/*!
 * @brief 町の条件を判定し、変数名の数値不正を条件不成立と区別する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param condition 条件式。省略時は成立
 * @return 条件の成立状態。町変数の数値不正・未定義Quest参照は解析エラー
 * @details 未知の変数や式自体のエラーは、従来どおり条件不成立とする。
 */
tl::expected<bool, ConditionExpressionFailure> is_town_map_condition_met(PlayerType *player_ptr, const tl::optional<std::string> &condition)
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
