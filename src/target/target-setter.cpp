#include "target/target-setter.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "io/cursor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "window/display-sub-windows.h"
#include "window/main-window-util.h"
#include <tuple>
#include <vector>

// "interesting" な座標一覧を記録する配列
static std::vector<Pos2D> pos_interests;

// Target Setter.
class TargetSetter {
public:
    TargetSetter(PlayerType *player_ptr, target_type mode);
    void sweep_target_grids();

private:
    std::string describe_projectablity();
    void menu_target();
    void switch_target_input();
    bool check_panel_changed();
    void sweep_targets();
    bool set_target_grid();
    std::string describe_grid_wizard() const;
    void switch_next_grid_command();
    void decide_change_panel();

    PlayerType *player_ptr;
    target_type mode;
    POSITION y;
    POSITION x;
    POSITION y2 = 0; // panel_row_min 退避用
    POSITION x2 = 0; // panel_col_min 退避用
    bool done = false;
    bool flag = true; // 移動コマンド入力時、"interesting" な座標へ飛ぶかどうか
    char query{};
    Grid *g_ptr = nullptr;
    TERM_LEN wid, hgt;
    int m = 0; // "interesting" な座標たちのうち現在ターゲットしているもののインデックス
    int distance = 0; // カーソルの移動方向 (1,2,3,4,6,7,8,9)
    int target_num = 0; // target_pick() の結果
    bool move_fast = false; // カーソル移動を粗くする(1マスずつ移動しない)
};

TargetSetter::TargetSetter(PlayerType *player_ptr, target_type mode)
    : player_ptr(player_ptr)
    , mode(mode)
    , y(player_ptr->y)
    , x(player_ptr->x)
{
    std::tie(this->wid, this->hgt) = get_screen_size();
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto dy = 0;
    auto dx = 0;
    [[maybe_unused]] const auto &[wid, hgt] = get_screen_size();
    if (y < panel_row_min) {
        dy = -1;
    }

    if (y > panel_row_max) {
        dy = 1;
    }

    if (x < panel_col_min) {
        dx = -1;
    }

    if (x > panel_col_max) {
        dx = 1;
    }

    if (!dy && !dx) {
        return false;
    }

    return change_panel(player_ptr, dy, dx);
}

/*!
 * @brief "interesting" な座標たちのうち、(y1,x1) から (dy,dx) 方向にある最も近いもののインデックスを得る。
 * @param y1 現在地座標y
 * @param x1 現在地座標x
 * @param dy 現在地からの向きy [-1,1]
 * @param dx 現在地からの向きx [-1,1]
 * @return 最も近い座標のインデックス。適切なものがない場合 -1
 */
static POSITION_IDX target_pick(const POSITION y1, const POSITION x1, const POSITION dy, const POSITION dx)
{
    // 最も近いもののインデックスとその距離。
    POSITION_IDX b_i = -1, b_v = 9999;

    for (POSITION_IDX i = 0; i < std::ssize(pos_interests); i++) {
        const auto &[y2, x2] = pos_interests[i];

        // (y1,x1) から (y2,x2) へ向かうベクトル。
        const POSITION x3 = (x2 - x1);
        const POSITION y3 = (y2 - y1);

        // (dy,dx) 方向にないものを除外する。

        // dx > 0 のとき、x3 <= 0 なるものは除外。
        // dx < 0 のとき、x3 >= 0 なるものは除外。
        if (dx && (x3 * dx <= 0)) {
            continue;
        }

        // dy > 0 のとき、y3 <= 0 なるものは除外。
        // dy < 0 のとき、y3 >= 0 なるものは除外。
        if (dy && (y3 * dy <= 0)) {
            continue;
        }

        const POSITION x4 = std::abs(x3);
        const POSITION y4 = std::abs(y3);

        // (dy,dx) が (-1,0) or (1,0) のとき、|x3| > |y3| なるものは除外。
        if (dy && !dx && (x4 > y4)) {
            continue;
        }

        // (dy,dx) が (0,-1) or (0,1) のとき、|y3| > |x3| なるものは除外。
        if (dx && !dy && (y4 > x4)) {
            continue;
        }

        // (y1,x1), (y2,x2) 間の距離を求め、最も近いものを更新する。
        // 距離の定義は v の式を参照。
        const POSITION_IDX v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));
        if ((b_i >= 0) && (v >= b_v)) {
            continue;
        }

        b_i = i;
        b_v = v;
    }

    return b_i;
}

std::string TargetSetter::describe_projectablity()
{
    this->y = pos_interests[this->m].y;
    this->x = pos_interests[this->m].x;
    change_panel_xy(this->player_ptr, this->y, this->x);
    if ((this->mode & TARGET_LOOK) == 0) {
        print_path(this->player_ptr, this->y, this->x);
    }

    std::string info;
    this->g_ptr = &this->player_ptr->current_floor_ptr->grid_array[this->y][this->x];
    if (target_able(this->player_ptr, this->g_ptr->m_idx)) {
        info = _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>");
    } else {
        info = _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>");
    }

    if (!cheat_sight) {
        return info;
    }

    const auto cheatinfo = format(" X:%d Y:%d LOS:%d LOP:%d",
        this->x, this->y,
        los(this->player_ptr, this->player_ptr->y, this->player_ptr->x, this->y, this->x),
        projectable(this->player_ptr, this->player_ptr->get_position(), { this->y, this->x }));
    return info.append(cheatinfo);
}

void TargetSetter::menu_target()
{
    if (!use_menu) {
        return;
    }

    if (this->query == '\r') {
        this->query = 't';
    }
}

void TargetSetter::switch_target_input()
{
    this->distance = 0;
    switch (this->query) {
    case ESCAPE:
    case 'q':
        this->done = true;
        return;
    case 't':
    case '.':
    case '5':
    case '0':
        if (!target_able(this->player_ptr, this->g_ptr->m_idx)) {
            bell();
            return;
        }

        health_track(this->player_ptr, this->g_ptr->m_idx);
        target_who = this->g_ptr->m_idx;
        target_row = this->y;
        target_col = this->x;
        this->done = true;
        return;
    case ' ':
    case '*':
    case '+':
        if (++this->m != std::ssize(pos_interests)) {
            return;
        }

        this->m = 0;
        if (!expand_list) {
            this->done = true;
        }

        return;
    case '-':
        if (this->m-- != 0) {
            return;
        }

        this->m = std::ssize(pos_interests) - 1;
        if (!expand_list) {
            this->done = true;
        }

        return;
    case 'p': {
        verify_panel(this->player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        this->y = this->player_ptr->y;
        this->x = this->player_ptr->x;
    }
        [[fallthrough]];
    case 'o':
        this->flag = false;
        return;
    case 'm':
        return;
    default: {
        const char queried_command = rogue_like_commands ? 'x' : 'l';
        if (this->query != queried_command) {
            this->distance = get_keymap_dir(this->query);
            if (this->distance == 0) {
                bell();
            }

            return;
        }

        if (++this->m != std::ssize(pos_interests)) {
            return;
        }

        this->m = 0;
        if (!expand_list) {
            this->done = true;
        }

        return;
    }
    }
}

/*!
 * @brief カーソル移動に伴い、描画範囲、"interesting" 座標リスト、現在のターゲットを更新する。
 * @return カーソル移動によって描画範囲が変化したかどうか
 */
bool TargetSetter::check_panel_changed()
{
    // カーソル移動によって描画範囲が変化しないなら何もせずその旨を返す。
    if (!change_panel(this->player_ptr, ddy[this->distance], ddx[this->distance])) {
        return false;
    }

    // 描画範囲が変化した場合、"interesting" 座標リストおよび現在のターゲットを更新する必要がある。

    // "interesting" 座標を探す起点。
    // this->m が有効な座標を指していればそれを使う。
    // さもなくば (this->y, this->x) を使う。
    int v, u;
    if (this->m < std::ssize(pos_interests)) {
        v = pos_interests[this->m].y;
        u = pos_interests[this->m].x;
    } else {
        v = this->y;
        u = this->x;
    }

    // 新たな描画範囲を用いて "interesting" 座標リストを更新。
    pos_interests = target_set_prepare(this->player_ptr, this->mode);

    // 新たな "interesting" 座標リストからターゲットを探す。
    this->flag = true;
    this->target_num = target_pick(v, u, ddy[this->distance], ddx[this->distance]);
    if (this->target_num >= 0) {
        this->m = this->target_num;
    }

    return true;
}

/*!
 * @brief カーソル移動方向に "interesting" な座標がなかったとき、画面外まで探す。
 *
 * 既に "interesting" な座標を発見している場合、この関数は何もしない。
 */
void TargetSetter::sweep_targets()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    while (this->flag && (this->target_num < 0)) {
        // カーソル移動に伴い、必要なだけ描画範囲を更新。
        // "interesting" 座標リストおよび現在のターゲットも更新。
        if (this->check_panel_changed()) {
            continue;
        }

        POSITION dx = ddx[this->distance];
        POSITION dy = ddy[this->distance];
        panel_row_min = this->y2;
        panel_col_min = this->x2;
        panel_bounds_center();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        this->flag = false;
        this->x += dx;
        this->y += dy;
        if (((this->x < panel_col_min + this->wid / 2) && (dx > 0)) || ((this->x > panel_col_min + this->wid / 2) && (dx < 0))) {
            dx = 0;
        }

        if (((this->y < panel_row_min + this->hgt / 2) && (dy > 0)) || ((this->y > panel_row_min + this->hgt / 2) && (dy < 0))) {
            dy = 0;
        }

        if ((this->y >= panel_row_min + this->hgt) || (this->y < panel_row_min) || (this->x >= panel_col_min + this->wid) || (this->x < panel_col_min)) {
            if (change_panel(this->player_ptr, dy, dx)) {
                pos_interests = target_set_prepare(this->player_ptr, this->mode);
            }
        }

        if (this->x >= floor_ptr->width - 1) {
            this->x = floor_ptr->width - 2;
        } else if (this->x <= 0) {
            this->x = 1;
        }

        if (this->y >= floor_ptr->height - 1) {
            this->y = floor_ptr->height - 2;
        } else if (this->y <= 0) {
            this->y = 1;
        }
    }
}

bool TargetSetter::set_target_grid()
{
    if (!this->flag || pos_interests.empty()) {
        return false;
    }

    const auto info = this->describe_projectablity();
    fix_floor_item_list(this->player_ptr, { this->y, this->x });

    while (true) {
        this->query = examine_grid(this->player_ptr, this->y, this->x, this->mode, info.data());
        if (this->query) {
            break;
        }
    }

    this->menu_target();
    this->switch_target_input();
    if (this->distance == 0) {
        return true;
    }

    this->y2 = panel_row_min;
    this->x2 = panel_col_min;
    {
        this->target_num = target_pick(pos_interests[this->m].y, pos_interests[this->m].x, ddy[this->distance], ddx[this->distance]);
    }
    this->sweep_targets();
    this->m = this->target_num;
    return true;
}

std::string TargetSetter::describe_grid_wizard() const
{
    if (!cheat_sight) {
        return "";
    }

    constexpr auto fmt = " X:%d Y:%d LOS:%d LOP:%d SPECIAL:%d";
    const auto is_los = los(this->player_ptr, this->player_ptr->y, this->player_ptr->x, this->y, this->x);
    const auto is_projectable = projectable(this->player_ptr, this->player_ptr->get_position(), { this->y, this->x });
    const auto cheatinfo = format(fmt, this->x, this->y, is_los, is_projectable, this->g_ptr->special);
    return cheatinfo;
}

void TargetSetter::switch_next_grid_command()
{
    switch (this->query) {
    case ESCAPE:
    case 'q':
        this->done = true;
        break;
    case 't':
    case '.':
    case '5':
    case '0':
        target_who = -1;
        target_row = this->y;
        target_col = this->x;
        this->done = true;
        break;
    case 'p': {
        verify_panel(this->player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        this->y = this->player_ptr->y;
        this->x = this->player_ptr->x;
        break;
    }
    case 'o':
        // ターゲット時の「m近」「o現」の切り替え
        // すでに「o現」の時にoを押してもなにも起きない
        break;
    case ' ':
    case '*':
    case '+':
    case '-':
    case 'm': {
        this->flag = true;
        this->m = 0;
        int bd = 999;
        for (auto i = 0; i < std::ssize(pos_interests); i++) {
            const auto t = Grid::calc_distance({ this->y, this->x }, pos_interests[i]);
            if (t < bd) {
                this->m = i;
                bd = t;
            }
        }

        if (bd == 999) {
            this->flag = false;
        }

        break;
    }
    default:
        this->distance = get_keymap_dir(this->query);
        if (isupper(this->query)) {
            this->move_fast = true;
        }

        if (!this->distance) {
            bell();
        }

        break;
    }
}

void TargetSetter::decide_change_panel()
{
    if (this->distance == 0) {
        return;
    }

    POSITION dx = ddx[this->distance];
    POSITION dy = ddy[this->distance];
    if (this->move_fast) {
        int mag = std::min(this->wid / 2, this->hgt / 2);
        this->x += dx * mag;
        this->y += dy * mag;
    } else {
        this->x += dx;
        this->y += dy;
    }

    if (((this->x < panel_col_min + this->wid / 2) && (dx > 0)) || ((this->x > panel_col_min + this->wid / 2) && (dx < 0))) {
        dx = 0;
    }

    if (((this->y < panel_row_min + this->hgt / 2) && (dy > 0)) || ((this->y > panel_row_min + this->hgt / 2) && (dy < 0))) {
        dy = 0;
    }

    auto should_change_panel = this->y >= panel_row_min + this->hgt;
    should_change_panel |= this->y < panel_row_min;
    should_change_panel |= this->x >= panel_col_min + this->wid;
    should_change_panel |= this->x < panel_col_min;
    if (should_change_panel && change_panel(this->player_ptr, dy, dx)) {
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
    }

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (this->x >= floor_ptr->width - 1) {
        this->x = floor_ptr->width - 2;
    } else if (this->x <= 0) {
        this->x = 1;
    }

    if (this->y >= floor_ptr->height - 1) {
        this->y = floor_ptr->height - 2;
    } else if (this->y <= 0) {
        this->y = 1;
    }
}

void TargetSetter::sweep_target_grids()
{
    while (!this->done) {
        if (this->set_target_grid()) {
            continue;
        }

        this->move_fast = false;
        if ((this->mode & TARGET_LOOK) == 0) {
            print_path(this->player_ptr, this->y, this->x);
        }

        this->g_ptr = &this->player_ptr->current_floor_ptr->grid_array[this->y][this->x];
        std::string info = _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>");
        info.append(this->describe_grid_wizard());
        fix_floor_item_list(this->player_ptr, { this->y, this->x });

        /* Describe and Prompt (enable "TARGET_LOOK") */
        const auto target = i2enum<target_type>(this->mode | TARGET_LOOK);
        while ((this->query = examine_grid(this->player_ptr, this->y, this->x, target, info.data())) == 0) {
            ;
        }

        this->distance = 0;
        if (use_menu && (this->query == '\r')) {
            this->query = 't';
        }

        this->switch_next_grid_command();
        this->decide_change_panel();
    }
}

/*
 * Handle "target" and "look".
 */
bool target_set(PlayerType *player_ptr, target_type mode)
{
    TargetSetter ts(player_ptr, mode);
    target_who = 0;
    pos_interests = target_set_prepare(player_ptr, mode);
    ts.sweep_target_grids();
    prt("", 0, 0);
    verify_panel(player_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
    };
    rfu.set_flags(flags);
    handle_stuff(player_ptr);
    return target_who != 0;
}
