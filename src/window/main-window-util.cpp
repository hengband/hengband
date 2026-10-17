#include "window/main-window-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
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
#include "view/map-info-cache.h"
#include "window/overview-map-cache.h"
#include "window/shrunk-overview-map.h"
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
    // マップ全体を描き直すような変化は、ダンジョン全体図などでも全グリッドを求め直す。
    // 画面の保存中に残った描き直しの要求は、OverviewMapCache::update() や MapInfoCache::prepare() の側で拾う
    OverviewMapCache::get_instance().mark_all_dirty();
    // 描いたグリッドの結果を覚えられるよう、覚える配列もフロアの大きさに合わせておく
    auto &map_info_cache = MapInfoCache::get_instance();
    map_info_cache.prepare(*player_ptr);
    map_info_cache.invalidate_all();

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
            map_info_cache.store({ y, x }, symbol_pair);
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

    const auto &floor = *player_ptr->current_floor_ptr;
    auto &overview_map_cache = OverviewMapCache::get_instance();
    overview_map_cache.update(player_ptr);
    auto &shrunk_map = get_shrunk_overview_map(floor.height, floor.width, hgt, wid);
    shrunk_map.update(overview_map_cache);
    const auto yrat = shrunk_map.get_yrat();
    const auto xrat = shrunk_map.get_xrat();

    const auto monochrome_color = get_monochrome_display_color(player_ptr);
    for (auto y = 0; y < hgt + 2; ++y) {
        term_gotoxy(COL_MAP, y);
        for (auto x = 0; x < wid + 2; ++x) {
            // 縮小マップの周りに枠を描く
            const auto is_top_or_bottom = (y == 0) || (y == hgt + 1);
            const auto is_left_or_right = (x == 0) || (x == wid + 1);
            DisplaySymbol symbol_foreground;
            if (is_top_or_bottom && is_left_or_right) {
                symbol_foreground = { TERM_WHITE, '+' };
            } else if (is_top_or_bottom) {
                symbol_foreground = { TERM_WHITE, '-' };
            } else if (is_left_or_right) {
                symbol_foreground = { TERM_WHITE, '|' };
            } else {
                symbol_foreground = shrunk_map.get_cell(y - 1, x - 1).symbol;
            }

            symbol_foreground.color = monochrome_color.value_or(symbol_foreground.color);
            term_add_bigch(symbol_foreground);
        }
    }

    for (auto y = 0; y < hgt; ++y) {
        tl::optional<OverviewMapAutopick> row_autopick;
        for (auto x = 0; x < wid; x++) {
            const auto &autopick = shrunk_map.get_autopick(y, x);
            if (autopick && (!row_autopick || (row_autopick->rule_index > autopick->rule_index))) {
                row_autopick = autopick;
            }
        }

        // 枠の分だけ 1 行下に描く
        term_putstr(0, y + 1, 12, 0, "            ");
        if (row_autopick) {
            display_shortened_item_name(player_ptr, *row_autopick->item, y + 1);
        }
    }

    (*cy) = player_ptr->y / yrat + 1 + ROW_MAP;
    if (!use_bigtile) {
        (*cx) = player_ptr->x / xrat + 1 + COL_MAP;
    } else {
        (*cx) = (player_ptr->x / xrat + 1) * 2 + COL_MAP;
    }
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
