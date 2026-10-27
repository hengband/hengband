#include "info-reader/legend-ego-validator.h"
#include "info-reader/general-parser.h"
#include "info-reader/random-grid-effect-types.h"
#include "object-enchant/object-ego.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include <algorithm>

/*!
 * @brief 凡例のエゴ指定と、実際に配置される品の組み合わせを検証する。
 * @param grid 共通パーサーで読んだ凡例
 * @param object_is_quest_reward 報酬を指すobject指定か
 * @param has_trap 罠の指定があるか（町の未解決タグも呼び出し側で判定する）
 * @return 成功時は空の値、失敗時は生成できない理由
 */
tl::expected<void, LegendEgoError> validate_legend_ego(const dungeon_grid &grid, bool object_is_quest_reward, bool has_trap)
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
    if (has_trap) {
        return tl::make_unexpected(LegendEgoError::WITH_TRAP);
    }
    const auto compatible = random_ego
                                ? std::any_of(egos_info.begin(), egos_info.end(), [&key](const auto &entry) { return can_apply_ego(key, entry.first); })
                                : can_apply_ego(key, grid.ego);
    if (!compatible) {
        return tl::make_unexpected(LegendEgoError::INCOMPATIBLE);
    }
    return {};
}
