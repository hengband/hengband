#include "window/main-window-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "player/player-status.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "view/display-map.h"
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
void print_field(concptr info, TERM_LEN row, TERM_LEN col)
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

    for (POSITION y = 1; y <= ymin - panel_row_prt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymax - panel_row_prt; y <= hgt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymin; y <= ymax; y++) {
        for (POSITION x = xmin; x <= xmax; x++) {
            TERM_COLOR a;
            char c;
            TERM_COLOR ta;
            char tc;
            map_info(player_ptr, y, x, &a, &c, &ta, &tc);
            if (!use_graphics) {
                if (w_ptr->timewalk_m_idx) {
                    a = TERM_DARK;
                } else if (is_invuln(player_ptr) || player_ptr->timewalk) {
                    a = TERM_WHITE;
                } else if (player_ptr->wraith_form) {
                    a = TERM_L_DARK;
                }
            }

            term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);
        }
    }

    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
    (void)term_set_cursor(v);
}

/*!
 * @brief 短縮マップにおける自動拾い対象のアイテムを短縮表記する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr アイテムへの参照ポインタ
 * @param y 表示する行番号
 */
static void display_shortened_item_name(PlayerType *player_ptr, ItemEntity *o_ptr, int y)
{
    auto item_name = describe_flavor(player_ptr, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NAME_ONLY));
    auto attr = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
    if (player_ptr->effects()->hallucination()->is_hallucinated()) {
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

    TERM_COLOR ta;
    char tc;

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
            map_info(player_ptr, j, i, &ta, &tc, &ta, &tc);
            tp = (byte)feat_priority;
            if (match_autopick != -1 && (match_autopick_yx[y][x] == -1 || match_autopick_yx[y][x] > match_autopick)) {
                match_autopick_yx[y][x] = match_autopick;
                object_autopick_yx[y][x] = autopick_obj;
                tp = 0x7f;
            }

            bigmc[j + 1][i + 1] = tc;
            bigma[j + 1][i + 1] = ta;
            bigmp[j + 1][i + 1] = tp;
        }
    }

    for (j = 0; j < floor_ptr->height; ++j) {
        for (i = 0; i < floor_ptr->width; ++i) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            tc = bigmc[j + 1][i + 1];
            ta = bigma[j + 1][i + 1];
            tp = bigmp[j + 1][i + 1];
            if (mp[y][x] == tp) {
                int t;
                int cnt = 0;

                for (t = 0; t < 8; t++) {
                    if (tc == bigmc[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]] && ta == bigma[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]]) {
                        cnt++;
                    }
                }
                if (cnt <= 4) {
                    tp++;
                }
            }

            if (mp[y][x] < tp) {
                mc[y][x] = tc;
                ma[y][x] = ta;
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
            ta = ma[y][x];
            tc = mc[y][x];
            if (!use_graphics) {
                if (w_ptr->timewalk_m_idx) {
                    ta = TERM_DARK;
                } else if (is_invuln(player_ptr) || player_ptr->timewalk) {
                    ta = TERM_WHITE;
                } else if (player_ptr->wraith_form) {
                    ta = TERM_L_DARK;
                }
            }

            term_add_bigch(ta, tc);
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
            display_shortened_item_name(player_ptr, autopick_obj, y);
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

void set_term_color(PlayerType *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, char *cp)
{
    if (!player_bold(player_ptr, y, x)) {
        return;
    }

    auto *r_ptr = &monraces_info[MonsterRaceId::PLAYER];
    *ap = r_ptr->x_attr;
    *cp = r_ptr->x_char;
    feat_priority = 31;
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
