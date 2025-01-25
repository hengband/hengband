#include "target/grid-selector.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/screen-util.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-sorter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include <functional>
#include <unordered_map>
#include <vector>

/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(PlayerType *player_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs)
{
    if (!expand_list) {
        return;
    }

    std::vector<Pos2D> pos_list;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    for (auto y = 1; y < floor.height; y++) {
        for (auto x = 1; x < floor.width; x++) {
            const Pos2D pos(y, x);
            if (!in_bounds(&floor, pos.y, pos.x)) {
                continue;
            }

            if (pos == p_pos) {
                pos_list.push_back(pos);
                continue;
            }

            if (is_hallucinated) {
                continue;
            }

            const auto &grid = floor.get_grid(pos);
            if (!grid.is_mark()) {
                continue;
            }

            if (grid.is_acceptable_target()) {
                pos_list.push_back(pos);
            }
        }
    }

    TargetSorter sorter(p_pos);
    std::stable_sort(pos_list.begin(), pos_list.end(), [&sorter](const auto &a, const auto &b) { return sorter.compare_distance(a, b); });
    for (const auto &pos : pos_list) {
        ys.push_back(pos.y);
        xs.push_back(pos.x);
    }
}

/*!
 * @brief 指定したシンボルのマスかどうかを判定するための条件式コールバック
 */
std::unordered_map<int, std::function<bool(Grid *)>> tgt_pt_symbol_call_back = {
    { '<', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STAIRS) && g_ptr->has(TerrainCharacteristics::LESS); } },
    { '>', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STAIRS) && g_ptr->has(TerrainCharacteristics::MORE); } },
    { '+', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::BLDG); } },
    { '0', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('0'); } },
    { '!', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('1'); } },
    { '"', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('2'); } },
    { '#', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('3'); } },
    { '$', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('4'); } },
    { '%', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('5'); } },
    { '&', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('6'); } },
    { '\'', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('7'); } },
    { '(', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('8'); } },
    { ')', [](Grid *g_ptr) { return g_ptr->has(TerrainCharacteristics::STORE) && g_ptr->is_symbol('9'); } },
};

/*!
 * @brief 位置ターゲット指定情報構造体
 * @details
 * ang_sort() を利用する関係上、y/x 座標それぞれについて配列を作る。
 */
struct tgt_pt_info {
    tgt_pt_info()
    {
        std::tie(this->width, this->height) = get_screen_size();
    };

    int width; //!< 画面サイズ(幅)
    int height; //!< 画面サイズ(高さ)
    POSITION y = 0; //!< 現在の指定位置(Y)
    POSITION x = 0; //!< 現在の指定位置(X)
    std::vector<POSITION> ys{}; //!< "interesting" な座標たちを記録する配列(Y)
    std::vector<POSITION> xs{}; //!< "interesting" な座標たちを記録する配列(X)
    size_t n = 0; //<! シンボル配列の何番目か
    char ch = '\0'; //<! 入力キー
    char prev_ch = '\0'; //<! 前回入力キー
    std::function<bool(Grid *)> callback{}; //<! 条件判定コールバック

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
        dy = 2 * (this->y - cy) / this->height;
        dx = 2 * (this->x - cx) / this->width;
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
            if (player_ptr->is_located_at({ info.y, info.x })) {
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
                    if (player_ptr->is_located_at({ info.y, info.x })) {
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
                int mag = std::min(info.width / 2, info.height / 2);
                info.x += dx * mag;
                info.y += dy * mag;
            } else {
                info.x += dx;
                info.y += dy;
            }

            if (((info.x < panel_col_min + info.width / 2) && (dx > 0)) || ((info.x > panel_col_min + info.width / 2) && (dx < 0))) {
                dx = 0;
            }

            if (((info.y < panel_row_min + info.height / 2) && (dy > 0)) || ((info.y > panel_row_min + info.height / 2) && (dy < 0))) {
                dy = 0;
            }

            if ((info.y >= panel_row_min + info.height) || (info.y < panel_row_min) || (info.x >= panel_col_min + info.width) || (info.x < panel_col_min)) {
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
