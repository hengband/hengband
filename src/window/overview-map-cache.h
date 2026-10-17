#pragma once

/*!
 * @file overview-map-cache.h
 * @brief ダンジョン全体図に使う、グリッドごとの表示のキャッシュ
 * @details
 * ダンジョン全体図 (display_map()) はフロアの全グリッドの表示を map_info() で求めるので重い。
 * 前回求めた結果を覚えておき、表示が変わったかもしれないグリッドだけ求め直す。
 * 表示が変わるグリッドは lite_spot() で、全体に関わる変化は print_map() で知らされる。
 * lite_spot() は印を付けるだけで、サブウィンドウを描く前に印を付けたグリッドを求め直し、
 * サブウィンドウの全体図を最後に描いたときから表示が変わっていたら描き直す。
 * 自動拾いの対象を表示している間 (M コマンドの間) は、毎回全グリッドを求め直し、結果を後で使わない。
 * 縮小した全体図を変わったグリッドの周りだけ作り直せるよう、表示が変わったグリッドを世代ごとに記録する。
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

    bool operator==(const OverviewMapAutopick &other) const = default;
};

/*!
 * @brief 1 グリッド分の map_info() の結果
 */
struct OverviewMapGrid {
    DisplaySymbol symbol{};
    int priority = 0; //!< 縮小するときの優先度 (map_info() が設定した feat_priority)
    tl::optional<OverviewMapAutopick> autopick; //!< 自動拾いの規則に一致したアイテム (map_info() が設定した match_autopick と autopick_obj)

    bool operator==(const OverviewMapGrid &other) const = default;
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
    bool has_dirty_grids() const;
    bool has_changed() const;
    void reset_changed();
    const OverviewMapGrid &get_grid(const Pos2D &pos) const;
    int get_generation() const;
    const std::vector<Pos2D> &get_changed_positions() const;

private:
    OverviewMapCache() = default;

    static OverviewMapCache instance;

    int height = 0;
    int width = 0;
    bool is_all_dirty = true; //!< 全グリッドを求め直す印
    bool is_changed = false; //!< サブウィンドウの全体図を描いてから、求め直した表示が変わったか
    std::vector<OverviewMapGrid> grids; //!< フロアの全グリッドの表示 (行優先)
    std::vector<bool> is_dirty; //!< 求め直す印 (grids と同じ並び)
    std::vector<Pos2D> dirty_positions; //!< 印を付けたグリッドの一覧 (全グリッドを走査せずに済むように持つ)
    int generation = 0; //!< 表示が変わったグリッドの記録の番号。全グリッドを求め直したときと、記録が長くなりすぎて捨てたときに変わる
    std::vector<Pos2D> changed_positions; //!< 今の世代で表示が変わったグリッドの一覧 (変わった順)

    void start_new_generation();

    int index_of(const Pos2D &pos) const;
};
