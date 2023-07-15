#include "target/grid-selector.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "grid/feature-flag-types.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/screen-util.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include <functional>
#include <unordered_map>
#include <vector>

/*
 * XAngband: determine if a given location is "interesting"
 * based on target_set_accept function.
 */
static bool tgt_pt_accept(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x))) {
        return false;
    }

    if ((y == player_ptr->y) && (x == player_ptr->x)) {
        return true;
    }

    if (player_ptr->effects()->hallucination()->is_hallucinated()) {
        return false;
    }

    auto &grid = floor_ptr->grid_array[y][x];
    if (!grid.is_mark()) {
        return false;
    }

    using Tc = TerrainCharacteristics;
    auto is_acceptable = grid.cave_has_flag(Tc::LESS);
    is_acceptable |= grid.cave_has_flag(Tc::MORE);
    is_acceptable |= grid.cave_has_flag(Tc::QUEST_ENTER);
    is_acceptable |= grid.cave_has_flag(Tc::QUEST_EXIT);
    is_acceptable |= grid.cave_has_flag(Tc::STORE);
    is_acceptable |= grid.cave_has_flag(Tc::BLDG);
    return is_acceptable;
}

/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(PlayerType *player_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs)
{
    if (!expand_list) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION y = 1; y < floor_ptr->height; y++) {
        for (POSITION x = 1; x < floor_ptr->width; x++) {
            if (!tgt_pt_accept(player_ptr, y, x)) {
                continue;
            }

            ys.emplace_back(y);
            xs.emplace_back(x);
        }
    }

    ang_sort(player_ptr, xs.data(), ys.data(), size(ys), ang_sort_comp_distance, ang_sort_swap_position);
}

/*!
 * @brief 指定したシンボルのマスかどうかを判定するための条件式コールバック
 */
std::unordered_map<int, std::function<bool(grid_type *)>> tgt_pt_symbol_call_back = {
    { '<', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STAIRS) && g_ptr->cave_has_flag(TerrainCharacteristics::LESS); } },
    { '>', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STAIRS) && g_ptr->cave_has_flag(TerrainCharacteristics::MORE); } },
    { '+', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::BLDG); } },
    { '0', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('0'); } },
    { '!', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('1'); } },
    { '"', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('2'); } },
    { '#', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('3'); } },
    { '$', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('4'); } },
    { '%', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('5'); } },
    { '&', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('6'); } },
    { '\'', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('7'); } },
    { '(', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('8'); } },
    { ')', [](grid_type *g_ptr) { return g_ptr->cave_has_flag(TerrainCharacteristics::STORE) && g_ptr->is_symbol('9'); } },
};

/*!
 * @brief 位置ターゲット指定情報構造体
 * @details
 * ang_sort() を利用する関係上、y/x 座標それぞれについて配列を作る。
 */
struct tgt_pt_info {
    tgt_pt_info()
    {
        get_screen_size(&this->wid, &this->hgt);
    };

    TERM_LEN wid; //!< 画面サイズ(幅)
    TERM_LEN hgt; //!< 画面サイズ(高さ)
    POSITION y = 0; //!< 現在の指定位置(Y)
    POSITION x = 0; //!< 現在の指定位置(X)
    std::vector<POSITION> ys{}; //!< "interesting" な座標たちを記録する配列(Y)
    std::vector<POSITION> xs{}; //!< "interesting" な座標たちを記録する配列(X)
    size_t n = 0; //<! シンボル配列の何番目か
    char ch = '\0'; //<! 入力キー
    char prev_ch = '\0'; //<! 前回入力キー
    std::function<bool(grid_type *)> callback{}; //<! 条件判定コールバック

    void move_to_symbol(PlayerType *player_ptr);
};

/*!
 * @brief 指定した記号のシンボルのグリッドにカーソルを移動する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @details 自分 (＠)の位置に戻ってくるような処理に見える.
 * コールバックにも依る？
 */
void tgt_pt_info::move_to_symbol(PlayerType *player_ptr)
{
    if (!expand_list || this->ys.empty()) {
        return;
    }

    int dx, dy;
    int cx = (panel_col_min + panel_col_max) / 2;
    int cy = (panel_row_min + panel_row_max) / 2;
    if (this->ch != this->prev_ch) {
        this->n = 0;
    }
    this->prev_ch = this->ch;
    this->n++;

    for (; this->n < size(this->ys); ++this->n) {
        const POSITION y_cur = this->ys[this->n];
        const POSITION x_cur = this->xs[this->n];
        auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y_cur][x_cur];
        if (this->callback(g_ptr)) {
            break;
        }
    }

    if (this->n == size(this->ys)) {
        this->n = 0;
        this->y = player_ptr->y;
        this->x = player_ptr->x;
        verify_panel(player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(player_ptr);
    } else {
        this->y = this->ys[this->n];
        this->x = this->xs[this->n];
        dy = 2 * (this->y - cy) / this->hgt;
        dx = 2 * (this->x - cx) / this->wid;
        if (dy || dx) {
            change_panel(player_ptr, dy, dx);
        }
    }
}

/*!
 * @brief 位置を指定するプロンプト
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param x_ptr x座標への参照ポインタ
 * @param y_ptr y座標への参照ポインタ
 * @return 指定したらTRUE、キャンセルしたらFALSE
 */
bool tgt_pt(PlayerType *player_ptr, POSITION *x_ptr, POSITION *y_ptr)
{
    tgt_pt_info info;
    info.y = player_ptr->y;
    info.x = player_ptr->x;
    if (expand_list) {
        tgt_pt_prepare(player_ptr, info.ys, info.xs);
    }

    msg_print(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."));
    msg_flag = false;

    info.ch = 0;
    info.n = 0;
    bool success = false;
    while ((info.ch != ESCAPE) && !success) {
        bool move_fast = false;
        move_cursor_relative(info.y, info.x);
        info.ch = inkey();
        switch (info.ch) {
        case ESCAPE:
            break;
        case ' ':
        case 't':
        case '.':
            if (player_bold(player_ptr, info.y, info.x)) {
                info.ch = 0;
            } else {
                success = true;
            }
            break;
        case '>':
        case '<':
        case '+':
        case '!':
        case '"':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')': {
            info.callback = tgt_pt_symbol_call_back[info.ch];
            info.move_to_symbol(player_ptr);
            break;
        }
        default: {
            if (rogue_like_commands) {
                if (info.ch >= '0' && info.ch <= '9') {
                    if (info.ch != '0') {
                        info.ch -= 16;
                    }
                    info.callback = tgt_pt_symbol_call_back[info.ch];
                    info.move_to_symbol(player_ptr);
                    break;
                }
            } else {
                if (info.ch == '5' || info.ch == '0') {
                    if (player_bold(player_ptr, info.y, info.x)) {
                        info.ch = 0;
                    } else {
                        success = true;
                    }
                    break;
                }
            }

            int d = get_keymap_dir(info.ch);
            if (isupper(info.ch)) {
                move_fast = true;
            }

            if (d == 0) {
                break;
            }

            int dx = ddx[d];
            int dy = ddy[d];
            if (move_fast) {
                int mag = std::min(info.wid / 2, info.hgt / 2);
                info.x += dx * mag;
                info.y += dy * mag;
            } else {
                info.x += dx;
                info.y += dy;
            }

            if (((info.x < panel_col_min + info.wid / 2) && (dx > 0)) || ((info.x > panel_col_min + info.wid / 2) && (dx < 0))) {
                dx = 0;
            }

            if (((info.y < panel_row_min + info.hgt / 2) && (dy > 0)) || ((info.y > panel_row_min + info.hgt / 2) && (dy < 0))) {
                dy = 0;
            }

            if ((info.y >= panel_row_min + info.hgt) || (info.y < panel_row_min) || (info.x >= panel_col_min + info.wid) || (info.x < panel_col_min)) {
                change_panel(player_ptr, dy, dx);
            }

            if (info.x >= player_ptr->current_floor_ptr->width - 1) {
                info.x = player_ptr->current_floor_ptr->width - 2;
            } else if (info.x <= 0) {
                info.x = 1;
            }

            if (info.y >= player_ptr->current_floor_ptr->height - 1) {
                info.y = player_ptr->current_floor_ptr->height - 2;
            } else if (info.y <= 0) {
                info.y = 1;
            }

            break;
        }
        }
    }

    prt("", 0, 0);
    verify_panel(player_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
    handle_stuff(player_ptr);
    *x_ptr = info.x;
    *y_ptr = info.y;
    return success;
}
