/*!
 * @file shrunk-overview-map.h
 * @brief 端末の大きさに縮小したダンジョン全体図
 * @details
 * 縮小マップの 1 マスは、フロアの xrat × yrat グリッドの区画から優先度で 1 つの記号を選ぶ。
 * 選ぶときに区画の各グリッドの周り 8 マスも見るので、1 マスの結果は区画とその周り 1 グリッドだけで決まる。
 * OverviewMapCache で表示が変わったグリッドを読み、そのグリッドの周りを含む区画のマスだけを作り直す。
 */
#pragma once

#include "term/term-color-types.h"
#include "util/point-2d.h"
#include "view/display-symbol.h"
#include "window/overview-map-cache.h"
#include <array>
#include <tl/optional.hpp>
#include <vector>

/*!
 * @brief 縮小した全体図
 * @details 縮小の比率が変わると作り直しになるので、端末ごとに、端末とフロアの大きさが変わったら作り直す (get_shrunk_overview_map())。
 * マスの座標は枠を含まず、左上を (0, 0) とする
 */
class ShrunkOverviewMap {
public:
    /*!
     * @brief 縮小マップの 1 マス分の記号と、縮めるときの優先度
     */
    struct Cell {
        DisplaySymbol symbol = { TERM_WHITE, ' ' };
        byte priority = 0;

        bool operator==(const Cell &other) const = default;
    };

    ShrunkOverviewMap(int floor_height, int floor_width, int hgt, int wid);

    bool matches(int floor_height_, int floor_width_, int hgt_, int wid_) const;
    void update(const OverviewMapCache &cache);
    const Cell &get_cell(int y, int x) const;
    const tl::optional<OverviewMapAutopick> &get_autopick(int y, int x) const;
    int get_yrat() const;
    int get_xrat() const;

private:
    int floor_height;
    int floor_width;
    int hgt;
    int wid;
    int yrat;
    int xrat;
    std::vector<Cell> cells; //!< 縮小マップの各マス (行優先)
    std::vector<tl::optional<OverviewMapAutopick>> autopicks; //!< 各マスの区画で最も優先する自動拾いの規則 (cells と同じ並び)
    std::vector<bool> is_dirty; //!< 作り直す印 (cells と同じ並び。update() の中だけで使う作業領域)
    std::vector<Pos2D> dirty_cells; //!< 作り直すマスの一覧 (update() の中だけで使う作業領域)
    tl::optional<int> generation; //!< 最後に読んだ OverviewMapCache の世代
    size_t read_count = 0; //!< 今の世代で、表示が変わったグリッドの一覧をどこまで読んだか
    std::vector<const OverviewMapGrid *> area_grids; //!< 1 マス分の区画と周り 1 グリッドの表示。フロアの外は nullptr (作業領域)
    std::array<int, 8> neighbor_offsets{}; //!< area_grids の中で周り 8 マスを指すずらし
    std::vector<byte> priorities; //!< 1 マス分の区画の各グリッドの優先度 (area_grids と同じ並び。作業領域)

    int index_of(int y, int x) const;
    void mark_dirty_around(const Pos2D &pos);
    void shrink_cell(const OverviewMapCache &cache, int y, int x);
};

ShrunkOverviewMap &get_shrunk_overview_map(int floor_height, int floor_width, int hgt, int wid);
