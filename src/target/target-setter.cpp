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
#include <optional>
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
    std::string describe_projectablity() const;
    char examine_target_grid(std::string_view info, std::optional<target_type> append_mode = std::nullopt) const;
    void change_interest_index(int amount);
    std::optional<int> switch_target_input();
    std::optional<int> check_panel_changed(int dir);
    std::optional<int> sweep_targets(int dir, int panel_row_min_initial, int panel_col_min_initial);
    bool set_target_grid();
    std::string describe_grid_wizard() const;
    std::optional<int> switch_next_grid_command();
    void decide_change_panel(int dir);

    PlayerType *player_ptr;
    target_type mode;
    Pos2D pos_target;
    bool done = false;
    std::optional<int> interest_index = 0; // "interesting" な座標たちのうち現在ターゲットしているもののインデックス
    bool move_fast = false; // カーソル移動を粗くする(1マスずつ移動しない)
};

TargetSetter::TargetSetter(PlayerType *player_ptr, target_type mode)
    : player_ptr(player_ptr)
    , mode(mode)
    , pos_target(player_ptr->get_position())
{
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 変更先のフロア座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(PlayerType *player_ptr, const Pos2D &pos)
{
    auto dy = 0;
    auto dx = 0;
    [[maybe_unused]] const auto &[wid, hgt] = get_screen_size();
    if (pos.y < panel_row_min) {
        dy = -1;
    }

    if (pos.y > panel_row_max) {
        dy = 1;
    }

    if (pos.x < panel_col_min) {
        dx = -1;
    }

    if (pos.x > panel_col_max) {
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
std::optional<int> target_pick(const POSITION y1, const POSITION x1, const POSITION dy, const POSITION dx)
{
    // 最も近いもののインデックスとその距離。
    std::optional<int> b_i;
    auto b_v = 9999;

    for (auto i = 0; i < std::ssize(pos_interests); i++) {
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

std::string TargetSetter::describe_projectablity() const
{
    change_panel_xy(this->player_ptr, this->pos_target);
    if ((this->mode & TARGET_LOOK) == 0) {
        print_path(this->player_ptr, this->pos_target.y, this->pos_target.x);
    }

    std::string info;
    const auto &grid = this->player_ptr->current_floor_ptr->get_grid(this->pos_target);
    if (target_able(this->player_ptr, grid.m_idx)) {
        info = _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>");
    } else {
        info = _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>");
    }

    if (!cheat_sight) {
        return info;
    }

    const auto p_pos = this->player_ptr->get_position();
    const auto cheatinfo = format(" X:%d Y:%d LOS:%d LOP:%d",
        this->pos_target.x, this->pos_target.y,
        los(this->player_ptr, p_pos.y, p_pos.x, this->pos_target.y, this->pos_target.x),
        projectable(this->player_ptr, p_pos, this->pos_target));
    return info.append(cheatinfo);
}

char TargetSetter::examine_target_grid(std::string_view info, std::optional<target_type> append_mode) const
{
    const auto target_mode = append_mode ? i2enum<target_type>(this->mode | *append_mode) : this->mode;
    while (true) {
        const auto query = examine_grid(this->player_ptr, this->pos_target.y, this->pos_target.x, target_mode, info.data());
        if (query != '\0') {
            return (use_menu && (query == '\r')) ? 't' : query;
        }
    }
}

void TargetSetter::change_interest_index(int amount)
{
    if (!this->interest_index || pos_interests.empty()) {
        this->interest_index = std::nullopt;
        return;
    }
    *this->interest_index += amount;

    if (*this->interest_index >= 0 && *this->interest_index < std::ssize(pos_interests)) {
        return;
    }

    // wrap around
    this->interest_index = (*this->interest_index < 0) ? std::ssize(pos_interests) - 1 : 0;

    if (!expand_list) {
        this->done = true;
    }
}

std::optional<int> TargetSetter::switch_target_input()
{
    const auto info = this->describe_projectablity();
    const auto query = this->examine_target_grid(info);
    switch (query) {
    case ESCAPE:
    case 'q':
        this->done = true;
        return std::nullopt;
    case 't':
    case '.':
    case '5':
    case '0': {
        const auto &grid = this->player_ptr->current_floor_ptr->get_grid(this->pos_target);
        if (!target_able(this->player_ptr, grid.m_idx)) {
            bell();
            return std::nullopt;
        }

        health_track(this->player_ptr, grid.m_idx);
        target_who = grid.m_idx;
        target_row = this->pos_target.y;
        target_col = this->pos_target.x;
        this->done = true;
        return std::nullopt;
    }
    case ' ':
    case '*':
    case '+':
        this->change_interest_index(1);
        return std::nullopt;
    case '-':
        this->change_interest_index(-1);
        return std::nullopt;
    case 'p': {
        verify_panel(this->player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        this->pos_target = this->player_ptr->get_position();
    }
        [[fallthrough]];
    case 'o':
        this->interest_index = std::nullopt;
        return std::nullopt;
    case 'm':
        return std::nullopt;
    default: {
        const char queried_command = rogue_like_commands ? 'x' : 'l';
        if (query != queried_command) {
            const auto dir = get_keymap_dir(query);
            if (dir == 0) {
                bell();
                return std::nullopt;
            }

            return dir;
        }

        this->change_interest_index(1);
        return std::nullopt;
    }
    }
}

/*!
 * @brief カーソル移動に伴い、描画範囲、"interesting" 座標リスト、現在のターゲットを更新する。
 * @return カーソル移動によって描画範囲が変化したかどうか
 */
std::optional<int> TargetSetter::check_panel_changed(int dir)
{
    // 描画範囲が変化した場合、"interesting" 座標リストおよび現在のターゲットを更新する必要がある。

    // "interesting" 座標を探す起点。
    // this->m が有効な座標を指していればそれを使う。
    // さもなくば (this->y, this->x) を使う。
    const auto is_point_interest = (this->interest_index && *this->interest_index < std::ssize(pos_interests));
    const auto pos = is_point_interest ? pos_interests[*this->interest_index] : this->pos_target;

    // 新たな描画範囲を用いて "interesting" 座標リストを更新。
    pos_interests = target_set_prepare(this->player_ptr, this->mode);

    // 新たな "interesting" 座標リストからターゲットを探す。
    return target_pick(pos.y, pos.x, ddy[dir], ddx[dir]);
}

/*!
 * @brief カーソル移動方向に "interesting" な座標がなかったとき、画面外まで探す。
 *
 * 既に "interesting" な座標を発見している場合、この関数は何もしない。
 */
std::optional<int> TargetSetter::sweep_targets(int dir, int panel_row_min_initial, int panel_col_min_initial)
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto &[y, x] = this->pos_target;
    std::optional<int> target_index;
    while (true) {
        auto dy = ddy[dir];
        auto dx = ddx[dir];

        // カーソル移動に伴い、必要なだけ描画範囲を更新。
        // "interesting" 座標リストおよび現在のターゲットも更新。
        if (change_panel(this->player_ptr, dy, dx)) {
            const auto target_index = this->check_panel_changed(dir);
            if (target_index) {
                return target_index;
            }
            continue;
        }

        panel_row_min = panel_row_min_initial;
        panel_col_min = panel_col_min_initial;
        panel_bounds_center();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        x += dx;
        y += dy;
        const auto [wid, hgt] = get_screen_size();
        if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0))) {
            dx = 0;
        }

        if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0))) {
            dy = 0;
        }

        if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min)) {
            if (change_panel(this->player_ptr, dy, dx)) {
                pos_interests = target_set_prepare(this->player_ptr, this->mode);
            }
        }

        x = std::clamp(x, 1, floor_ptr->width - 2);
        y = std::clamp(y, 1, floor_ptr->height - 2);
        return std::nullopt;
    }
}

bool TargetSetter::set_target_grid()
{
    if (pos_interests.empty() || !this->interest_index) {
        return false;
    }

    this->pos_target = pos_interests[*this->interest_index];

    fix_floor_item_list(this->player_ptr, this->pos_target);

    const auto dir = this->switch_target_input();
    if (!dir) {
        return true;
    }

    auto target_index = target_pick(pos_interests[*this->interest_index].y, pos_interests[*this->interest_index].x, ddy[*dir], ddx[*dir]);
    if (!target_index) {
        target_index = this->sweep_targets(*dir, panel_row_min, panel_col_min);
    }
    this->interest_index = target_index;
    return true;
}

std::string TargetSetter::describe_grid_wizard() const
{
    if (!cheat_sight) {
        return "";
    }

    const auto &grid = this->player_ptr->current_floor_ptr->get_grid(this->pos_target);
    constexpr auto fmt = " X:%d Y:%d LOS:%d LOP:%d SPECIAL:%d";
    const auto is_los = los(this->player_ptr, this->player_ptr->y, this->player_ptr->x, this->pos_target.y, this->pos_target.x);
    const auto is_projectable = projectable(this->player_ptr, this->player_ptr->get_position(), this->pos_target);
    const auto cheatinfo = format(fmt, this->pos_target.x, this->pos_target.y, is_los, is_projectable, grid.special);
    return cheatinfo;
}

std::optional<int> TargetSetter::switch_next_grid_command()
{
    std::string info = _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>");
    info.append(this->describe_grid_wizard());
    const auto query = this->examine_target_grid(info, TARGET_LOOK);
    switch (query) {
    case ESCAPE:
    case 'q':
        this->done = true;
        return std::nullopt;
    case 't':
    case '.':
    case '5':
    case '0':
        target_who = -1;
        target_row = this->pos_target.y;
        target_col = this->pos_target.x;
        this->done = true;
        return std::nullopt;
    case 'p': {
        verify_panel(this->player_ptr);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
        handle_stuff(this->player_ptr);
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
        this->pos_target = this->player_ptr->get_position();
        return std::nullopt;
    }
    case 'o':
        // ターゲット時の「m近」「o現」の切り替え
        // すでに「o現」の時にoを押してもなにも起きない
        return std::nullopt;
    case ' ':
    case '*':
    case '+':
    case '-':
    case 'm': {
        const auto begin = pos_interests.begin();
        const auto end = pos_interests.end();
        const auto target = std::min_element(begin, end, [&](const auto &lhs, const auto &rhs) {
            return Grid::calc_distance(this->pos_target, lhs) < Grid::calc_distance(this->pos_target, rhs);
        });
        if (target != end) {
            this->interest_index = std::distance(pos_interests.begin(), target);
        } else {
            this->interest_index = std::nullopt;
        }

        return std::nullopt;
    }
    default:
        const auto dir = get_keymap_dir(query);
        if (isupper(query)) {
            this->move_fast = true;
        }

        if (dir == 0) {
            bell();
            return std::nullopt;
        }

        return dir;
    }
}

void TargetSetter::decide_change_panel(int dir)
{
    auto dx = ddx[dir];
    auto dy = ddy[dir];
    auto &[y, x] = this->pos_target;
    const auto [wid, hgt] = get_screen_size();
    if (this->move_fast) {
        int mag = std::min(wid / 2, hgt / 2);
        x += dx * mag;
        y += dy * mag;
    } else {
        x += dx;
        y += dy;
    }

    if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0))) {
        dx = 0;
    }

    if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0))) {
        dy = 0;
    }

    auto should_change_panel = y >= panel_row_min + hgt;
    should_change_panel |= y < panel_row_min;
    should_change_panel |= x >= panel_col_min + wid;
    should_change_panel |= x < panel_col_min;
    if (should_change_panel && change_panel(this->player_ptr, dy, dx)) {
        pos_interests = target_set_prepare(this->player_ptr, this->mode);
    }

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    x = std::clamp(x, 1, floor_ptr->width - 2);
    y = std::clamp(y, 1, floor_ptr->height - 2);
}

void TargetSetter::sweep_target_grids()
{
    while (!this->done) {
        if (this->set_target_grid()) {
            continue;
        }

        this->move_fast = false;
        if ((this->mode & TARGET_LOOK) == 0) {
            print_path(this->player_ptr, this->pos_target.y, this->pos_target.x);
        }

        fix_floor_item_list(this->player_ptr, this->pos_target);

        if (auto dir = this->switch_next_grid_command(); dir) {
            this->decide_change_panel(*dir);
        }
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
