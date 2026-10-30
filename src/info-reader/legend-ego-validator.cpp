#include "info-reader/legend-ego-validator.h"
#include "info-reader/general-parser.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/language-switcher.h"
#include "object-enchant/object-ego.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include <algorithm>
#include <fmt/format.h>

/*!
 * @brief 凡例のエゴ指定と、実際に配置される品の組み合わせを検証する。
 * @param grid 共通パーサーで読んだ凡例
 * @param object_is_quest_reward 報酬を指すobject指定か
 * @return 成功時は空の値、失敗時は生成できない理由
 */
tl::expected<void, LegendEgoError> validate_legend_ego(const dungeon_grid &grid, bool object_is_quest_reward)
{
    const auto random_ego = (grid.random & RANDOM_EGO) != 0;
    if (grid.ego == EgoType::NONE && !random_ego) {
        return {};
    }
    if (grid.ego != EgoType::NONE) {
        const auto it = egos_info.find(grid.ego);
        if (it == egos_info.end()) {
            return tl::make_unexpected(LegendEgoError::UNDEFINED_ID);
        }
        if (is_worthless_ego(it->second)) {
            return tl::make_unexpected(LegendEgoError::WORTHLESS);
        }
    }
    if (grid.random & RANDOM_OBJECT) {
        return tl::make_unexpected(LegendEgoError::RANDOM_OBJECT);
    }
    if (object_is_quest_reward) {
        return tl::make_unexpected(LegendEgoError::QUEST_REWARD);
    }
    const auto &baseitems = BaseitemList::get_instance();
    if (grid.object == 0 || !baseitems.is_valid(grid.object)) {
        return tl::make_unexpected(LegendEgoError::NO_PLACED_OBJECT);
    }
    const auto &key = baseitems.get_baseitem(grid.object).bi_key;
    if (key.tval() == ItemKindType::GOLD) {
        return tl::make_unexpected(LegendEgoError::GOLD);
    }
    const auto compatible = random_ego
                                ? std::any_of(egos_info.begin(), egos_info.end(), [&key](const auto &entry) { return can_apply_ego(key, entry.first); })
                                : can_apply_ego(key, grid.ego);
    if (!compatible) {
        return tl::make_unexpected(LegendEgoError::INCOMPATIBLE);
    }
    return {};
}

// 検証結果を利用者向けの理由に変換し、ランダム指定でも原因を特定できるようにする。
std::string_view describe_legend_ego_error(LegendEgoError error)
{
    switch (error) {
    case LegendEgoError::UNDEFINED_ID:
        return _("未定義のエゴID", "undefined ego ID");
    case LegendEgoError::WORTHLESS:
        return _("悪いエゴ", "worthless ego");
    case LegendEgoError::NO_PLACED_OBJECT:
        return _("固定の配置対象がない", "no fixed object to place");
    case LegendEgoError::RANDOM_OBJECT:
        return _("ランダムな品には指定できない", "cannot apply to a random object");
    case LegendEgoError::QUEST_REWARD:
        return _("クエスト報酬には指定できない", "cannot apply to a quest reward");
    case LegendEgoError::GOLD:
        return _("財宝には指定できない", "cannot apply to gold");
    case LegendEgoError::INCOMPATIBLE:
        return _("品の種類と相性が合わない", "incompatible object kind");
    }
    return {};
}

/*!
 * @brief 町とクエストの診断で共通の形式により、エゴ指定と品の指定を表示する。
 * @param grid 共通パーサーで読んだ凡例
 * @param object_is_quest_reward 報酬を指すobject指定か
 * @return "ego 65 (object 54)" の形式の文字列
 * @details ランダムな品の object には生成階層の補正が、報酬の品には 0 が入っていて品IDに見えるため、
 * 定義ファイルと同じ綴りの random / questReward で表示する。
 */
std::string describe_legend_ego_spec(const dungeon_grid &grid, bool object_is_quest_reward)
{
    const auto ego = (grid.random & RANDOM_EGO) ? std::string("random") : std::to_string(static_cast<int>(grid.ego));
    std::string object;
    if (grid.random & RANDOM_OBJECT) {
        object = "random";
    } else if (object_is_quest_reward) {
        object = "questReward";
    } else {
        object = std::to_string(grid.object);
    }
    return fmt::format("ego {} (object {})", ego, object);
}
