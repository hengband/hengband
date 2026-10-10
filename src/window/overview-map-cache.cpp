#include "window/overview-map-cache.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-map.h"
#include "window/main-window-util.h"

namespace {
/*!
 * @brief グリッドの表示を map_info() で求める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos グリッドの座標
 * @return グリッドの表示
 */
OverviewMapGrid make_grid(PlayerType *player_ptr, const Pos2D &pos)
{
    match_autopick = -1;
    autopick_obj = nullptr;
    feat_priority = -1;
    const auto symbol_pair = map_info(player_ptr, pos);
    tl::optional<OverviewMapAutopick> autopick;
    if (match_autopick != -1) {
        autopick = OverviewMapAutopick{ match_autopick, autopick_obj };
    }

    return { symbol_pair.symbol_foreground, feat_priority, autopick };
}
}

OverviewMapCache OverviewMapCache::instance{};

OverviewMapCache &OverviewMapCache::get_instance()
{
    return instance;
}

/*!
 * @brief グリッドの表示が変わったかもしれないので、次に全体図を描くときに求め直す
 * @param pos グリッドの座標
 */
void OverviewMapCache::mark_dirty(const Pos2D &pos)
{
    if (this->is_all_dirty || (pos.y < 0) || (pos.y >= this->height) || (pos.x < 0) || (pos.x >= this->width)) {
        return;
    }

    const auto index = this->index_of(pos);
    if (this->is_dirty[index]) {
        return;
    }

    this->is_dirty[index] = true;
    this->dirty_positions.push_back(pos);
}

/*!
 * @brief フロア全体の表示が変わったかもしれないので、次に全体図を描くときに全グリッドを求め直す
 */
void OverviewMapCache::mark_all_dirty()
{
    this->is_all_dirty = true;
}

/*!
 * @brief 印を付けたグリッドの表示を求め直す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * map_info() で求めるので、呼び出し側で表示のオプションを全体図用に切り替えてから呼ぶこと。
 * フロアの切り替えは、マップ全体の描き直しの要求で知らされる。
 * 次の場合は全グリッドを求め直す。
 * - マップ全体の描き直しの要求が残っている (画面の保存中は print_map() が呼ばれず、印が付かないため)
 * - 自動拾いの対象を表示している (display_autopick、M コマンドの間だけ有効)。
 *   キャッシュにアイテムへのポインタを残さないよう、この結果は次に使わない
 * - フロアの大きさが前回と違う (配列を確保し直す)
 */
void OverviewMapCache::update(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if ((this->height != floor.height) || (this->width != floor.width)) {
        this->height = floor.height;
        this->width = floor.width;
        this->grids.resize(static_cast<size_t>(this->height * this->width));
        this->is_dirty.assign(this->grids.size(), false);
        this->dirty_positions.clear();
        this->is_all_dirty = true;
    }

    const auto is_autopick_displayed = display_autopick.any();
    if (RedrawingFlagsUpdater::get_instance().has(MainWindowRedrawingFlag::MAP) || is_autopick_displayed) {
        this->is_all_dirty = true;
    }

    if (this->is_all_dirty) {
        for (auto y = 0; y < this->height; y++) {
            for (auto x = 0; x < this->width; x++) {
                this->grids[this->index_of({ y, x })] = make_grid(player_ptr, { y, x });
            }
        }

        this->is_dirty.assign(this->grids.size(), false);
        this->dirty_positions.clear();
        this->is_all_dirty = is_autopick_displayed;
        return;
    }

    for (const auto &pos : this->dirty_positions) {
        const auto index = this->index_of(pos);
        this->grids[index] = make_grid(player_ptr, pos);
        this->is_dirty[index] = false;
    }

    this->dirty_positions.clear();
}

/*!
 * @brief グリッドの表示を返す
 * @param pos グリッドの座標
 * @return 最後に update() で求めたグリッドの表示
 */
const OverviewMapGrid &OverviewMapCache::get_grid(const Pos2D &pos) const
{
    return this->grids[this->index_of(pos)];
}

int OverviewMapCache::index_of(const Pos2D &pos) const
{
    return pos.y * this->width + pos.x;
}
