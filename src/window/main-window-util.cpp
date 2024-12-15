#include "window/main-window-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "player/player-status.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/timed-effects.h"
#include "view/display-map.h"
#include "view/display-symbol.h"
#include "world/world.h"
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
ItemEntity *autopick_obj; /*!< 各種自動拾い処理時に使うオブジェクトポインタ */
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

    int v;
    (void)term_get_cursor(&v);

    (void)term_set_cursor(0);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    POSITION xmin = (0 < panel_col_min) ? panel_col_min : 0;
    POSITION xmax = (floor_ptr->width - 1 > panel_col_max) ? panel_col_max : floor_ptr->width - 1;
    POSITION ymin = (0 < panel_row_min) ? panel_row_min : 0;
    POSITION ymax = (floor_ptr->height - 1 > panel_row_max) ? panel_row_max : floor_ptr->height - 1;

    for (auto y = 1; y <= ymin - panel_row_prt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (auto y = ymax - panel_row_prt; y <= hgt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (auto y = ymin; y <= ymax; y++) {
        for (auto x = xmin; x <= xmax; x++) {
            auto symbol_pair = map_info(player_ptr, { y, x });
            symbol_pair.symbol_foreground.color = get_monochrome_display_color(player_ptr).value_or(symbol_pair.symbol_foreground.color);

            term_queue_bigchar(panel_col_of(x), y - panel_row_prt, symbol_pair);
        }
    }

    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
    (void)term_set_cursor(v);
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
    if (player_ptr->effects()->hallucination().is_hallucinated()) {
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

    bool old_view_special_lite = view_special_lite;
    bool old_view_granite_lite = view_granite_lite;

    auto border_width = use_bigtile ? 2 : 1; //!< @note 枠線幅
    auto [wid, hgt] = term_get_size();
    hgt -= 2;
    wid -= 12 + border_width * 2; //!< @note 描画桁数(枠線抜)
    if (use_bigtile) {
        wid = wid / 2 - 1;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto yrat = (floor_ptr->height + hgt - 1) / hgt;
    const auto xrat = (floor_ptr->width + wid - 1) / wid;
    view_special_lite = false;
    view_granite_lite = false;

    using std::vector;
    vector<vector<TERM_COLOR>> ma(hgt + 2, vector<TERM_COLOR>(wid + 2, TERM_WHITE));
    vector<vector<char>> mc(hgt + 2, vector<char>(wid + 2, ' '));
    vector<vector<byte>> mp(hgt + 2, vector<byte>(wid + 2, 0));
    vector<vector<int>> match_autopick_yx(hgt + 2, vector<int>(wid + 2, -1));
    vector<vector<ItemEntity *>> object_autopick_yx(hgt + 2, vector<ItemEntity *>(wid + 2, nullptr));

    vector<vector<TERM_COLOR>> bigma(floor_ptr->height + 2, vector<TERM_COLOR>(floor_ptr->width + 2, TERM_WHITE));
    vector<vector<char>> bigmc(floor_ptr->height + 2, vector<char>(floor_ptr->width + 2, ' '));
    vector<vector<byte>> bigmp(floor_ptr->height + 2, vector<byte>(floor_ptr->width + 2, 0));

    for (i = 0; i < floor_ptr->width; ++i) {
        for (j = 0; j < floor_ptr->height; ++j) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            match_autopick = -1;
            autopick_obj = nullptr;
            feat_priority = -1;
            const auto symbol_pair = map_info(player_ptr, { j, i });
            tp = (byte)feat_priority;
            if (match_autopick != -1 && (match_autopick_yx[y][x] == -1 || match_autopick_yx[y][x] > match_autopick)) {
                match_autopick_yx[y][x] = match_autopick;
                object_autopick_yx[y][x] = autopick_obj;
                tp = 0x7f;
            }

            bigma[j + 1][i + 1] = symbol_pair.symbol_foreground.color;
            bigmc[j + 1][i + 1] = symbol_pair.symbol_foreground.character;
            bigmp[j + 1][i + 1] = tp;
        }
    }

    for (j = 0; j < floor_ptr->height; ++j) {
        for (i = 0; i < floor_ptr->width; ++i) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            DisplaySymbol symbol_foreground(bigma[j + 1][i + 1], bigmc[j + 1][i + 1]);
            tp = bigmp[j + 1][i + 1];
            if (mp[y][x] == tp) {
                int cnt = 0;

                for (const auto &dd : CCW_DD) {
                    if ((symbol_foreground.character == bigmc[j + 1 + dd.y][i + 1 + dd.x]) && (symbol_foreground.color == bigma[j + 1 + dd.y][i + 1 + dd.x])) {
                        cnt++;
                    }
                }
                if (cnt <= 4) {
                    tp++;
                }
            }

            if (mp[y][x] < tp) {
                ma[y][x] = symbol_foreground.color;
                mc[y][x] = symbol_foreground.character;
                mp[y][x] = tp;
            }
        }
    }

    x = wid + 1;
    y = hgt + 1;

    mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';
    for (x = 1; x <= wid; x++) {
        mc[0][x] = mc[y][x] = '-';
    }

    for (y = 1; y <= hgt; y++) {
        mc[y][0] = mc[y][x] = '|';
    }

    for (y = 0; y < hgt + 2; ++y) {
        term_gotoxy(COL_MAP, y);
        for (x = 0; x < wid + 2; ++x) {
            DisplaySymbol symbol_foreground(ma[y][x], mc[y][x]);
            symbol_foreground.color = get_monochrome_display_color(player_ptr).value_or(symbol_foreground.color);

            term_add_bigch(symbol_foreground);
        }
    }

    for (y = 1; y < hgt + 1; ++y) {
        match_autopick = -1;
        for (x = 1; x <= wid; x++) {
            if (match_autopick_yx[y][x] != -1 && (match_autopick > match_autopick_yx[y][x] || match_autopick == -1)) {
                match_autopick = match_autopick_yx[y][x];
                autopick_obj = object_autopick_yx[y][x];
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
