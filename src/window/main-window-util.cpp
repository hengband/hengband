#include "window/main-window-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "player/player-status.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/timed-effects.h"
#include "view/display-map.h"
#include "view/display-symbol.h"
#include "world/world.h"
#include <array>
#include <range/v3/algorithm.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*
 * Dungeon size info
 */
POSITION panel_row_min;
POSITION panel_row_max;
POSITION panel_col_min;
POSITION panel_col_max;
POSITION panel_col_prt;
POSITION panel_row_prt;

int match_autopick;
const ItemEntity *autopick_obj; /*!< 各種自動拾い処理時に使うオブジェクトポインタ */
int feat_priority; /*!< マップ縮小表示時に表示すべき地形の優先度を保管する */

static const std::vector<std::pair<std::string_view, std::string_view>> simplify_list = {
#ifdef JP
    { "の魔法書", "" }
#else
    { "^Ring of ", "=" }, { "^Amulet of ", "\"" }, { "^Scroll of ", "?" }, { "^Scroll titled ", "?" }, { "^Wand of ", "-" }, { "^Rod of ", "-" },
    { "^Staff of ", "_" }, { "^Potion of ", "!" }, { " Spellbook ", "" }, { "^Book of ", "" }, { " Magic [", "[" }, { " Book [", "[" }, { " Arts [", "[" },
    { "^Set of ", "" }, { "^Pair of ", "" }
#endif
};

/*!
 * @brief 画面左の能力値表示を行うために指定位置から13キャラ分を空白消去後指定のメッセージを明るい青で描画する /
 * Print character info at given row, column in a 13 char field
 * @param info 表示文字列
 * @param row 描画列
 * @param col 描画行
 */
void print_field(std::string_view info, TERM_LEN row, TERM_LEN col)
{
    c_put_str(TERM_WHITE, "             ", row, col);
    c_put_str(TERM_L_BLUE, info, row, col);
}

/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
void print_map(PlayerType *player_ptr)
{
    auto [wid, hgt] = term_get_size();
    wid -= COL_MAP + 2;
    hgt -= ROW_MAP + 2;

    const auto v = term_get_cursor();
    term_set_cursor(false);

    const auto &floor = *player_ptr->current_floor_ptr;
    POSITION xmin = (0 < panel_col_min) ? panel_col_min : 0;
    POSITION xmax = (floor.width - 1 > panel_col_max) ? panel_col_max : floor.width - 1;
    POSITION ymin = (0 < panel_row_min) ? panel_row_min : 0;
    POSITION ymax = (floor.height - 1 > panel_row_max) ? panel_row_max : floor.height - 1;

    for (auto y = 1; y <= ymin - panel_row_prt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (auto y = ymax - panel_row_prt; y <= hgt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    const auto monochrome_color = get_monochrome_display_color(player_ptr);
    for (auto y = ymin; y <= ymax; y++) {
        for (auto x = xmin; x <= xmax; x++) {
            auto symbol_pair = map_info(player_ptr, { y, x });
            symbol_pair.symbol_foreground.color = monochrome_color.value_or(symbol_pair.symbol_foreground.color);

            term_queue_bigchar(panel_col_of(x), y - panel_row_prt, symbol_pair);
        }
    }

    // 各マスは lite_spot() を通らないので、ここで地図のサブウィンドウの再描画を要求する
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);

    lite_spot(player_ptr, player_ptr->get_position());
    term_set_cursor(v != 0);
}

namespace {
/*!
 * @brief 縮小マップの作業領域に使う、1 次元の配列に詰めた 2 次元の配列
 */
template <typename T>
class FlatArray2D {
public:
    FlatArray2D(int height, int width)
        : width(width)
        , cells(static_cast<size_t>(height * width))
    {
    }

    int index_of(int y, int x) const
    {
        return y * this->width + x;
    }

    T &operator()(int y, int x)
    {
        return this->cells[this->index_of(y, x)];
    }

    const T &operator()(int y, int x) const
    {
        return this->cells[this->index_of(y, x)];
    }

    T &operator[](int index)
    {
        return this->cells[index];
    }

    const T &operator[](int index) const
    {
        return this->cells[index];
    }

private:
    int width;
    std::vector<T> cells;
};

/*!
 * @brief 縮小マップの 1 マス分の記号と、縮めるときの優先度
 */
struct MapCell {
    DisplaySymbol symbol = { TERM_WHITE, ' ' };
    byte priority = 0;
};

/*!
 * @brief 縮小マップの 1 マス分の、自動拾いの対象になるアイテム
 */
struct AutopickCell {
    int match = -1;
    const ItemEntity *item = nullptr;
};
}

/*!
 * @brief 短縮マップにおける自動拾い対象のアイテムを短縮表記する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item アイテムへの参照
 * @param y 表示する行番号
 */
static void display_shortened_item_name(PlayerType *player_ptr, const ItemEntity &item, int y)
{
    auto item_name = describe_flavor(player_ptr, item, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NAME_ONLY));
    auto attr = tval_to_attr[enum2i(item.bi_key.tval()) % 128];
    if (player_ptr->effects()->hallucination().is_active()) {
        attr = TERM_WHITE;
        item_name = _("何か奇妙な物", "something strange");
    }

    for (const auto &simplified_str : simplify_list) {
        const auto &replacing = simplified_str.first;
#ifndef JP
        if (replacing.starts_with('^')) {
            const auto replacing_without_caret = replacing.substr(1);
            if (item_name.starts_with(replacing_without_caret)) {
                item_name.replace(0, replacing_without_caret.length(), simplified_str.second);
                break;
            }
        }
#endif

        const auto pos = item_name.find(replacing);
        if (pos != std::string::npos) {
            item_name.replace(pos, replacing.length(), simplified_str.second);
            break;
        }
    }

    constexpr auto max_shortened_name = 12;
    term_putstr(0, y, max_shortened_name, attr, item_name);
}

/*!
 * @brief 縮小マップ表示 / Display a "small-scale" map of the dungeon in the active Term
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param cy 縮小マップ上のプレイヤーのy座標
 * @param cx 縮小マップ上のプレイヤーのx座標
 * @details
 * メインウィンドウ('M'コマンド)、サブウィンドウ兼(縮小図)用。
 * use_bigtile時に横の描画列数は1/2になる。
 */
void display_map(PlayerType *player_ptr, int *cy, int *cx)
{
    int i, j, x, y;

    byte tp;

    auto border_width = use_bigtile ? 2 : 1; //!< @note 枠線幅
    auto [wid, hgt] = term_get_size();
    hgt -= 2;
    wid -= 12 + border_width * 2; //!< @note 描画桁数(枠線抜)
    if (use_bigtile) {
        wid = wid / 2 - 1;
    }

    // 枠の内側に 1 マスも描けないほど狭い端末には描かない (2 倍幅のタイルで幅 15〜19 桁のサブウィンドウなど)
    if ((wid < 1) || (hgt < 1)) {
        *cy = ROW_MAP;
        *cx = COL_MAP;
        return;
    }

    const auto old_view_special_lite = view_special_lite;
    const auto old_view_granite_lite = view_granite_lite;

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto yrat = (floor.height + hgt - 1) / hgt;
    const auto xrat = (floor.width + wid - 1) / wid;
    view_special_lite = false;
    view_granite_lite = false;

    FlatArray2D<MapCell> small_map(hgt + 2, wid + 2);
    FlatArray2D<AutopickCell> autopicks(hgt + 2, wid + 2);
    FlatArray2D<MapCell> big_map(floor.height + 2, floor.width + 2);
    for (i = 0; i < floor.width; ++i) {
        for (j = 0; j < floor.height; ++j) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            match_autopick = -1;
            autopick_obj = nullptr;
            feat_priority = -1;
            const auto symbol_pair = map_info(player_ptr, { j, i });
            tp = (byte)feat_priority;
            auto &autopick = autopicks(y, x);
            if (match_autopick != -1 && (autopick.match == -1 || autopick.match > match_autopick)) {
                autopick = { match_autopick, autopick_obj };
                tp = 0x7f;
            }

            big_map(j + 1, i + 1) = { symbol_pair.symbol_foreground, tp };
        }
    }

    std::array<int, 8> neighbor_offsets{};
    ranges::transform(Direction::directions_8(), neighbor_offsets.begin(), [&big_map](const auto &d) {
        const auto vec = d.vec();
        return big_map.index_of(vec.y, vec.x);
    });

    for (j = 0; j < floor.height; ++j) {
        for (i = 0; i < floor.width; ++i) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            const auto index = big_map.index_of(j + 1, i + 1);
            const auto &symbol_foreground = big_map[index].symbol;
            tp = big_map[index].priority;
            auto &cell = small_map(y, x);
            if (cell.priority == tp) {
                // 周りの 8 マスのうち同じ記号が 4 つ以下なら優先する。5 つ目が見つかった時点で結論が出る
                auto cnt = 0;
                for (const auto offset : neighbor_offsets) {
                    if ((big_map[index + offset].symbol == symbol_foreground) && (++cnt > 4)) {
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

    x = wid + 1;
    y = hgt + 1;

    small_map(0, 0).symbol.character = small_map(0, x).symbol.character = small_map(y, 0).symbol.character = small_map(y, x).symbol.character = '+';
    for (x = 1; x <= wid; x++) {
        small_map(0, x).symbol.character = small_map(y, x).symbol.character = '-';
    }

    for (y = 1; y <= hgt; y++) {
        small_map(y, 0).symbol.character = small_map(y, x).symbol.character = '|';
    }

    const auto monochrome_color = get_monochrome_display_color(player_ptr);
    for (y = 0; y < hgt + 2; ++y) {
        term_gotoxy(COL_MAP, y);
        for (x = 0; x < wid + 2; ++x) {
            auto symbol_foreground = small_map(y, x).symbol;
            symbol_foreground.color = monochrome_color.value_or(symbol_foreground.color);

            term_add_bigch(symbol_foreground);
        }
    }

    for (y = 1; y < hgt + 1; ++y) {
        match_autopick = -1;
        for (x = 1; x <= wid; x++) {
            const auto &autopick = autopicks(y, x);
            if (autopick.match != -1 && (match_autopick > autopick.match || match_autopick == -1)) {
                match_autopick = autopick.match;
                autopick_obj = autopick.item;
            }
        }

        term_putstr(0, y, 12, 0, "            ");
        if (match_autopick != -1) {
            display_shortened_item_name(player_ptr, *autopick_obj, y);
        }
    }

    (*cy) = player_ptr->y / yrat + 1 + ROW_MAP;
    if (!use_bigtile) {
        (*cx) = player_ptr->x / xrat + 1 + COL_MAP;
    } else {
        (*cx) = (player_ptr->x / xrat + 1) * 2 + COL_MAP;
    }

    view_special_lite = old_view_special_lite;
    view_granite_lite = old_view_granite_lite;
}

DisplaySymbol set_term_color(PlayerType *player_ptr, const Pos2D &pos, const DisplaySymbol &symbol_orig)
{
    if (!player_ptr->is_located_at(pos)) {
        return symbol_orig;
    }

    feat_priority = 31;
    const auto &monrace = MonraceList::get_instance().get_monrace(MonraceId::PLAYER);
    return monrace.symbol_config;
}

/*
 * Calculate panel colum of a location in the map
 */
int panel_col_of(int col)
{
    col -= panel_col_min;
    if (use_bigtile) {
        col *= 2;
    }
    return col + 13;
}
