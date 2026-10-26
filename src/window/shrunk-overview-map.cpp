#include "window/shrunk-overview-map.h"
#include "floor/geometry.h"
#include "term/z-term.h"
#include <algorithm>
#include <map>
#include <range/v3/algorithm.hpp>

namespace {
// 区画の周りがフロアの外のときは、フロアの外のマスと同じ空白のグリッドとして周り 8 マスの比較に数える (元の縮小マップの作り方と同じ)
constexpr auto SYMBOL_OUTSIDE_FLOOR = ShrunkOverviewMap::Cell{}.symbol;
}

/*!
 * @brief 縮小マップを作る。全てのマスは次の update() で作る
 * @param floor_height フロアの高さ
 * @param floor_width フロアの幅
 * @param hgt 縮小マップの高さ (枠を除く。1 以上)
 * @param wid 縮小マップの幅 (枠を除く。1 以上)
 */
ShrunkOverviewMap::ShrunkOverviewMap(int floor_height, int floor_width, int hgt, int wid)
    : floor_height(floor_height)
    , floor_width(floor_width)
    , hgt(hgt)
    , wid(wid)
    , yrat((floor_height + hgt - 1) / hgt)
    , xrat((floor_width + wid - 1) / wid)
    , cells(static_cast<size_t>(hgt * wid))
{
    // 初期化子の中で this を使うと MSVC が警告する (C4355) ので、ほかのメンバーから決まる大きさはここで確保する
    this->autopicks.resize(this->cells.size());
    this->is_dirty.resize(this->cells.size());
    const auto area_width = this->xrat + 2;
    this->area_grids.resize(static_cast<size_t>((this->yrat + 2) * area_width));
    this->priorities.resize(this->area_grids.size());
    ranges::transform(Direction::directions_8(), this->neighbor_offsets.begin(), [area_width](const auto &d) {
        const auto vec = d.vec();
        return vec.y * area_width + vec.x;
    });
}

/*!
 * @brief フロアと縮小マップの大きさが同じか
 */
bool ShrunkOverviewMap::matches(int floor_height_, int floor_width_, int hgt_, int wid_) const
{
    return (this->floor_height == floor_height_) && (this->floor_width == floor_width_) && (this->hgt == hgt_) && (this->wid == wid_);
}

/*!
 * @brief OverviewMapCache の表示に合わせて縮小マップを作り直す
 * @param cache 求め直した後のグリッドの表示
 * @details 世代が変わっていたら全てのマスを、そうでなければ前回の後に表示が変わったグリッドに関わるマスだけを作り直す
 */
void ShrunkOverviewMap::update(const OverviewMapCache &cache)
{
    const auto &changed_positions = cache.get_changed_positions();
    if (this->generation != cache.get_generation()) {
        for (auto y = 0; y < this->hgt; y++) {
            for (auto x = 0; x < this->wid; x++) {
                this->shrink_cell(cache, y, x);
            }
        }

        this->generation = cache.get_generation();
    } else {
        for (auto i = this->read_count; i < changed_positions.size(); i++) {
            this->mark_dirty_around(changed_positions[i]);
        }

        for (const auto &pos : this->dirty_cells) {
            this->shrink_cell(cache, pos.y, pos.x);
            this->is_dirty[this->index_of(pos.y, pos.x)] = false;
        }

        this->dirty_cells.clear();
    }

    this->read_count = changed_positions.size();
}

/*!
 * @brief 縮小マップの 1 マスを返す
 * @param y 縮小マップの行
 * @param x 縮小マップの列
 * @return マスの記号と優先度。区画がフロアの外のマスは空白
 */
const ShrunkOverviewMap::Cell &ShrunkOverviewMap::get_cell(int y, int x) const
{
    return this->cells[this->index_of(y, x)];
}

/*!
 * @brief マスの区画で最も優先する自動拾いの規則を返す
 * @param y 縮小マップの行
 * @param x 縮小マップの列
 * @return 自動拾いの規則に一致したアイテムが区画に無ければ tl::nullopt
 */
const tl::optional<OverviewMapAutopick> &ShrunkOverviewMap::get_autopick(int y, int x) const
{
    return this->autopicks[this->index_of(y, x)];
}

/*!
 * @brief 縮小マップの 1 行がフロアの何行に当たるか
 */
int ShrunkOverviewMap::get_yrat() const
{
    return this->yrat;
}

/*!
 * @brief 縮小マップの 1 列がフロアの何列に当たるか
 */
int ShrunkOverviewMap::get_xrat() const
{
    return this->xrat;
}

int ShrunkOverviewMap::index_of(int y, int x) const
{
    return y * this->wid + x;
}

/*!
 * @brief グリッドの表示が変わったので、そのグリッドか周り 8 マスを区画に含むマスに作り直す印を付ける
 * @param pos 表示が変わったグリッドの座標
 */
void ShrunkOverviewMap::mark_dirty_around(const Pos2D &pos)
{
    // 周り 1 グリッド (フロアの中だけ) が当たるマスの範囲。縦横とも高々 2 マスになる
    const auto cell_top = std::max(pos.y - 1, 0) / this->yrat;
    const auto cell_bottom = std::min(pos.y + 1, this->floor_height - 1) / this->yrat;
    const auto cell_left = std::max(pos.x - 1, 0) / this->xrat;
    const auto cell_right = std::min(pos.x + 1, this->floor_width - 1) / this->xrat;
    for (auto cell_y = cell_top; cell_y <= cell_bottom; cell_y++) {
        for (auto cell_x = cell_left; cell_x <= cell_right; cell_x++) {
            const auto index = this->index_of(cell_y, cell_x);
            if (this->is_dirty[index]) {
                continue;
            }

            this->is_dirty[index] = true;
            this->dirty_cells.emplace_back(cell_y, cell_x);
        }
    }
}

/*!
 * @brief 縮小マップの 1 マスを、区画のグリッドの表示から作る
 * @param cache グリッドの表示
 * @param y 縮小マップの行
 * @param x 縮小マップの列
 * @details
 * 区画で最も優先する自動拾いの規則に一致したグリッドは、優先度を最高 (0x7f) にする。
 * 同じ優先度のグリッドが並んでいたら、周り 8 マスに同じ記号が少ない (4 つ以下) ものを優先する。
 * 区画の外 (フロアの外) のマスは空白として数える。
 * どちらも区画のグリッドを見る順番で結果が変わるので、自動拾いは列ごとに上から、記号の選択は行ごとに左から見る。
 */
void ShrunkOverviewMap::shrink_cell(const OverviewMapCache &cache, int y, int x)
{
    const auto top = y * this->yrat;
    const auto bottom = std::min(top + this->yrat, this->floor_height);
    const auto left = x * this->xrat;
    const auto right = std::min(left + this->xrat, this->floor_width);
    const auto index = this->index_of(y, x);
    auto &autopick = this->autopicks[index];
    autopick = tl::nullopt;
    if ((top >= bottom) || (left >= right)) {
        this->cells[index] = {};
        return;
    }

    // 区画と周り 1 グリッドの表示を集めておき、周り 8 マスの比較で何度も引かずに済ませる。
    // 右端で区画が狭くても、ずらしを共通にするため幅は区画の幅いっぱいにとる
    const auto area_width = this->xrat + 2;
    const auto area_index_of = [top, left, area_width](int j, int i) { return (j - top + 1) * area_width + (i - left + 1); };
    for (auto j = top - 1; j <= bottom; ++j) {
        for (auto i = left - 1; i <= right; ++i) {
            const auto is_inside = (j >= 0) && (j < this->floor_height) && (i >= 0) && (i < this->floor_width);
            this->area_grids[area_index_of(j, i)] = is_inside ? &cache.get_grid({ j, i }) : nullptr;
        }
    }

    for (auto i = left; i < right; ++i) {
        for (auto j = top; j < bottom; ++j) {
            const auto &grid = *this->area_grids[area_index_of(j, i)];
            auto tp = static_cast<byte>(grid.priority);
            if (grid.autopick && (!autopick || (autopick->rule_index > grid.autopick->rule_index))) {
                autopick = grid.autopick;
                tp = 0x7f;
            }

            this->priorities[area_index_of(j, i)] = tp;
        }
    }

    Cell cell{};
    for (auto j = top; j < bottom; ++j) {
        for (auto i = left; i < right; ++i) {
            const auto area_index = area_index_of(j, i);
            const auto &symbol_foreground = this->area_grids[area_index]->symbol;
            auto tp = this->priorities[area_index];
            if (cell.priority == tp) {
                // 周りの 8 マスのうち同じ記号が 4 つ以下なら優先する。5 つ目が見つかった時点で結論が出る
                auto cnt = 0;
                for (const auto offset : this->neighbor_offsets) {
                    const auto *grid_neighbor = this->area_grids[area_index + offset];
                    const auto &symbol_neighbor = grid_neighbor ? grid_neighbor->symbol : SYMBOL_OUTSIDE_FLOOR;
                    if ((symbol_neighbor == symbol_foreground) && (++cnt > 4)) {
                        break;
                    }
                }

                if (cnt <= 4) {
                    tp++;
                }
            }

            if (cell.priority < tp) {
                cell = { symbol_foreground, tp };
            }
        }
    }

    this->cells[index] = cell;
}

/*!
 * @brief 今の端末に描く縮小マップを返す
 * @param floor_height フロアの高さ
 * @param floor_width フロアの幅
 * @param hgt 縮小マップの高さ (枠を除く)
 * @param wid 縮小マップの幅 (枠を除く)
 * @return 縮小マップ。無いとき、端末やフロアの大きさが変わったときは作る
 * @details サブウィンドウごとに大きさが違うことがあるので、端末ごとに持つ
 */
ShrunkOverviewMap &get_shrunk_overview_map(int floor_height, int floor_width, int hgt, int wid)
{
    static std::map<const term_type *, ShrunkOverviewMap> shrunk_maps;
    const auto it = shrunk_maps.find(game_term);
    if ((it != shrunk_maps.end()) && it->second.matches(floor_height, floor_width, hgt, wid)) {
        return it->second;
    }

    return shrunk_maps.insert_or_assign(game_term, ShrunkOverviewMap(floor_height, floor_width, hgt, wid)).first->second;
}
