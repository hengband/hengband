#pragma once

/*!
 * @file overview-map-cache.h
 * @brief ダンジョン全体図に使う、グリッドごとの表示のキャッシュ
 * @details
 * ダンジョン全体図 (display_map()) はフロアの全グリッドの表示を map_info() で求めるので重い。
 * 前回求めた結果を覚えておき、表示が変わったかもしれないグリッドだけ求め直す。
 * 表示が変わるグリッドは lite_spot() で、全体に関わる変化は print_map() で知らされる。
 * 自動拾いの対象を表示している間 (M コマンドの間) は、毎回全グリッドを求め直し、結果を後で使わない。
 */

#include "util/point-2d.h"
#include "view/display-symbol.h"
#include <tl/optional.hpp>
#include <vector>

class ItemEntity;
class PlayerType;

/*!
 * @brief グリッドに表示するアイテムが一致した、自動拾いの規則
 */
struct OverviewMapAutopick {
    int rule_index; //!< 自動拾いの規則の番号 (小さいほど優先)
    const ItemEntity *item; //!< 一致したアイテム
};

/*!
 * @brief 1 グリッド分の map_info() の結果
 */
struct OverviewMapGrid {
    DisplaySymbol symbol{};
    int priority = 0; //!< 縮小するときの優先度 (map_info() が設定した feat_priority)
    tl::optional<OverviewMapAutopick> autopick; //!< 自動拾いの規則に一致したアイテム (map_info() が設定した match_autopick と autopick_obj)
};

class OverviewMapCache {
public:
    OverviewMapCache(const OverviewMapCache &) = delete;
    OverviewMapCache(OverviewMapCache &&) = delete;
    OverviewMapCache &operator=(const OverviewMapCache &) = delete;
    OverviewMapCache &operator=(OverviewMapCache &&) = delete;

    static OverviewMapCache &get_instance();

    void mark_dirty(const Pos2D &pos);
    void mark_all_dirty();
    void update(PlayerType *player_ptr);
    const OverviewMapGrid &get_grid(const Pos2D &pos) const;

private:
    OverviewMapCache() = default;

    static OverviewMapCache instance;

    int height = 0;
    int width = 0;
    bool is_all_dirty = true; //!< 全グリッドを求め直す印
    std::vector<OverviewMapGrid> grids; //!< フロアの全グリッドの表示 (行優先)
    std::vector<bool> is_dirty; //!< 求め直す印 (grids と同じ並び)
    std::vector<Pos2D> dirty_positions; //!< 印を付けたグリッドの一覧 (全グリッドを走査せずに済むように持つ)

    int index_of(const Pos2D &pos) const;
};
