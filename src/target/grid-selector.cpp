#include "target/grid-selector.h"
#include "core/stuff-handler.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/screen-util.h"
#include "system/building-type-definition.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-checker.h"
#include "target/target-sorter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/candidate-selector.h"
#include "util/finalizer.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include <functional>
#include <range/v3/view.hpp>
#include <unordered_map>
#include <vector>

/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static std::vector<Pos2D> tgt_pt_prepare(PlayerType *player_ptr)
{
    if (!expand_list) {
        return {};
    }

    std::vector<Pos2D> positions;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    for (const auto &pos : floor.get_area(FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        if (pos == p_pos) {
            positions.push_back(pos);
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
            positions.push_back(pos);
        }
    }

    TargetSorter sorter(p_pos);
    std::stable_sort(positions.begin(), positions.end(), [&sorter](const auto &a, const auto &b) { return sorter.compare_distance(a, b); });
    return positions;
}

static tl::optional<Pos2D> select_building_pos(const FloorType &floor)
{
    const auto is_building_pos = [&](const Pos2D &pos) {
        return floor.get_grid(pos).has(TerrainCharacteristics::BLDG);
    };
    const auto pos_buildings =
        floor.get_area() |
        ranges::views::filter(is_building_pos) |
        ranges::to<std::vector>();

    if (pos_buildings.size() <= 1) {
        return pos_buildings.empty() ? tl::nullopt : tl::make_optional(pos_buildings.front());
    }

    CandidateSelector cs(_("施設を選択してください:", "Select a building:"), 15);
    const auto describer = [&](const Pos2D &pos) {
        const auto &grid = floor.get_grid(pos);
        const auto &terrain = grid.get_terrain();
        return buildings.at(terrain.subtype).name;
    };

    const auto choice = cs.select(pos_buildings, describer);
    return choice != pos_buildings.end() ? tl::make_optional(*choice) : tl::nullopt;
}

/*!
 * @brief 指定したシンボルのマスかどうかを判定するための条件式コールバック
 */
std::unordered_map<int, std::function<bool(const Grid &)>> tgt_pt_symbol_call_back = {
    { '<', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STAIRS) && grid.has(TerrainCharacteristics::LESS); } },
    { '>', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STAIRS) && grid.has(TerrainCharacteristics::MORE); } },
    { '+', [](const Grid &grid) { return grid.has(TerrainCharacteristics::BLDG); } },
    { '0', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('0'); } },
    { '!', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('1'); } },
    { '"', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('2'); } },
    { '#', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('3'); } },
    { '$', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('4'); } },
    { '%', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('5'); } },
    { '&', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('6'); } },
    { '\'', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('7'); } },
    { '(', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('8'); } },
    { ')', [](const Grid &grid) { return grid.has(TerrainCharacteristics::STORE) && grid.is_symbol('9'); } },
};

/*!
 * @brief 位置ターゲット指定情報構造体
 */
struct tgt_pt_info {
    tgt_pt_info()
    {
        std::tie(this->width, this->height) = get_screen_size();
    };

    int width; //!< 画面サイズ(幅)
    int height; //!< 画面サイズ(高さ)
    Pos2D pos = { 0, 0 }; //!< 現在の指定位置
    std::vector<Pos2D> positions{}; //!< "interesting" な座標一覧の記録
    size_t n = 0; //<! シンボル配列の何番目か
    char ch = '\0'; //<! 入力キー
    char prev_ch = '\0'; //<! 前回入力キー
    std::function<bool(const Grid &)> callback{}; //<! 条件判定コールバック

    void move_to_symbol(PlayerType *player_ptr);
    bool is_ch_numeric() const
    {
        return is_numeric(this->ch);
    }
};

/*!
 * @brief 指定した記号のシンボルのグリッドにカーソルを移動する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @details 自分 (＠)の位置に戻ってくるような処理に見える.
 * コールバックにも依る？
 */
void tgt_pt_info::move_to_symbol(PlayerType *player_ptr)
{
    if (!expand_list || this->positions.empty()) {
        return;
    }

    if (this->ch != this->prev_ch) {
        this->n = 0;
    }
    this->prev_ch = this->ch;
    this->n++;

    if (this->ch == '+') {
        const auto pos_building = select_building_pos(*player_ptr->current_floor_ptr);
        if (!pos_building) {
            return;
        }
        this->n = 0;
        this->pos = *pos_building;
    } else {
        for (; this->n < this->positions.size(); ++this->n) {
            const auto &pos_cur = this->positions.at(this->n);
            const auto &grid = player_ptr->current_floor_ptr->get_grid(pos_cur);
            if (this->callback(grid)) {
                this->pos = this->positions.at(this->n);
                break;
            }
        }
    }

    if (this->n == this->positions.size()) {
        this->n = 0;
        this->pos = player_ptr->get_position();
        verify_panel(player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(player_ptr);
    } else {
        const auto cx = (panel_col_min + panel_col_max) / 2;
        const auto cy = (panel_row_min + panel_row_max) / 2;
        const auto dy = 2 * (this->pos.y - cy) / this->height;
        const auto dx = 2 * (this->pos.x - cx) / this->width;
        if ((dy != 0) || (dx != 0)) {
            change_panel(player_ptr, dy, dx);
        }
    }
}

/*!
 * @brief 位置を指定するプロンプト
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 指定したらその座標、キャンセルしたらnullopt
 */
tl::optional<Pos2D> point_target(PlayerType *player_ptr)
{
    tgt_pt_info info;
    info.pos = player_ptr->get_position();
    if (expand_list) {
        info.positions = tgt_pt_prepare(player_ptr);
    }

    msg_flag = false;

    info.ch = 0;
    info.n = 0;
    tl::optional<Pos2D> pos_target;
    while ((info.ch != ESCAPE) && !pos_target) {
        prt(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."), 0, 0);
        auto move_fast = false;
        move_cursor_relative(info.pos.y, info.pos.x);
        info.ch = inkey();
        switch (info.ch) {
        case ESCAPE:
            break;
        case ' ':
        case 't':
        case '.':
            if (player_ptr->is_located_at(info.pos)) {
                info.ch = 0;
            } else {
                pos_target = info.pos;
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
                if (info.is_ch_numeric()) {
                    if (info.ch != '0') {
                        info.ch -= 16;
                    }
                    info.callback = tgt_pt_symbol_call_back[info.ch];
                    info.move_to_symbol(player_ptr);
                    break;
                }
            } else {
                if (info.ch == '5' || info.ch == '0') {
                    if (player_ptr->is_located_at(info.pos)) {
                        info.ch = 0;
                    } else {
                        pos_target = info.pos;
                    }

                    break;
                }
            }

            const auto dir = get_keymap_dir(info.ch);
            if (isupper(info.ch)) {
                move_fast = true;
            }

            if (!dir) {
                break;
            }

            auto [dy, dx] = dir.vec();
            if (move_fast) {
                int mag = std::min(info.width / 2, info.height / 2);
                info.pos.y += dy * mag;
                info.pos.x += dx * mag;
            } else {
                info.pos.y += dy;
                info.pos.x += dx;
            }

            if (((info.pos.x < panel_col_min + info.width / 2) && (dx > 0)) || ((info.pos.x > panel_col_min + info.width / 2) && (dx < 0))) {
                dx = 0;
            }

            if (((info.pos.y < panel_row_min + info.height / 2) && (dy > 0)) || ((info.pos.y > panel_row_min + info.height / 2) && (dy < 0))) {
                dy = 0;
            }

            if ((info.pos.y >= panel_row_min + info.height) || (info.pos.y < panel_row_min) || (info.pos.x >= panel_col_min + info.width) || (info.pos.x < panel_col_min)) {
                change_panel(player_ptr, dy, dx);
            }

            const auto &floor = *player_ptr->current_floor_ptr;
            if (info.pos.x >= floor.width - 1) {
                info.pos.x = floor.width - 2;
            } else if (info.pos.x <= 0) {
                info.pos.x = 1;
            }

            if (info.pos.y >= floor.height - 1) {
                info.pos.y = floor.height - 2;
            } else if (info.pos.y <= 0) {
                info.pos.y = 1;
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
    return pos_target;
}
