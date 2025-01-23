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

class TargetSetter {
public:
    TargetSetter(PlayerType *player_ptr, target_type mode);
    void sweep_target_grids();

private:
    std::optional<int> pick_nearest_interest_target(const Pos2D &pos, int dir);
    std::string describe_projectablity() const;
    char examine_target_grid(std::string_view info, std::optional<target_type> append_mode = std::nullopt) const;
    void change_interest_index(int amount);
    std::optional<int> switch_target_input();
    std::optional<int> check_panel_changed(int dir);
    std::optional<int> sweep_targets(int dir, int panel_row_min_initial, int panel_col_min_initial);
    bool set_target_grid();
    std::string describe_grid_wizard() const;
    std::optional<std::pair<int, bool>> switch_next_grid_command();
    void decide_change_panel(int dir, bool move_fast);

    PlayerType *player_ptr;
    target_type mode;
    Pos2D pos_target;
    bool done = false;
    std::vector<Pos2D> pos_interests; // "interesting" な座標一覧を記録する配列
    std::optional<int> interest_index = 0; // "interesting" な座標一覧のうち現在ターゲットしているもののインデックス
};

TargetSetter::TargetSetter(PlayerType *player_ptr, target_type mode)
    : player_ptr(player_ptr)
    , mode(mode)
    , pos_target(player_ptr->get_position())
    , pos_interests(target_set_prepare(player_ptr, mode))
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
 * @brief 基準の座標から見て目標の座標が dir の方向±45°の範囲にあるかどうかを判定する
 *
 * @param pos_from 基準の座標
 * @param pos_to 目標の座標
 * @param dir 方向
 * @return dirの方向にある場合 true
 */
static bool is_roughly_in_direction(const Pos2D &pos_from, const Pos2D &pos_to, int dir)
{
    const auto dy = ddy[dir];
    const auto dx = ddx[dir];
    const auto vec = pos_to - pos_from;
    const Pos2DVec vec_abs(std::abs(vec.y), std::abs(vec.x));

    if (dx != 0 && (vec.x * dx <= 0)) {
        return false;
    }
    if (dy != 0 && (vec.y * dy <= 0)) {
        return false;
    }

    if (dy != 0 && dx == 0 && (vec_abs.x > vec_abs.y)) {
        return false;
    }
    if (dx != 0 && dy == 0 && (vec_abs.y > vec_abs.x)) {
        return false;
    }

    return true;
}

/*!
 * @brief "interesting" な座標一覧のうち、pos から dir 方向にある最も近いもののインデックスを得る。
 * @param pos 基準座標
 * @param dir 基準座標からの向き
 * @return 最も近い座標のインデックス。適切なものがない場合 std::nullopt
 */
std::optional<int> TargetSetter::pick_nearest_interest_target(const Pos2D &pos, int dir)
{
    std::optional<int> nearest_interest_index;
    std::optional<int> nearest_distance;

    for (auto i = 0; i < std::ssize(this->pos_interests); i++) {
        if (!is_roughly_in_direction(pos, this->pos_interests[i], dir)) {
            continue;
        }

        const auto distance = Grid::calc_distance(pos, this->pos_interests[i]);
        if (nearest_interest_index && (distance >= nearest_distance)) {
            continue;
        }

        nearest_interest_index = i;
        nearest_distance = distance;
    }

    return nearest_interest_index;
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
        los(*this->player_ptr->current_floor_ptr, p_pos, this->pos_target),
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
    if (!this->interest_index || this->pos_interests.empty()) {
        this->interest_index = std::nullopt;
        return;
    }
    *this->interest_index += amount;

    if (*this->interest_index >= 0 && *this->interest_index < std::ssize(this->pos_interests)) {
        return;
    }

    // wrap around
    this->interest_index = (*this->interest_index < 0) ? std::ssize(this->pos_interests) - 1 : 0;

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
        this->pos_interests = target_set_prepare(this->player_ptr, this->mode);
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
 * @return カーソル移動によって "interesting" な座標が選択されたらそのインデックス、そうでなければ std::nullopt
 */
std::optional<int> TargetSetter::check_panel_changed(int dir)
{
    // 描画範囲が変化した場合、"interesting" 座標リストおよび現在のターゲットを更新する必要がある。

    // "interesting" 座標を探す起点。
    const auto is_point_interest = (this->interest_index && *this->interest_index < std::ssize(this->pos_interests));
    const auto pos = is_point_interest ? this->pos_interests[*this->interest_index] : this->pos_target;

    // 新たな描画範囲を用いて "interesting" 座標リストを更新。
    this->pos_interests = target_set_prepare(this->player_ptr, this->mode);

    // 新たな "interesting" 座標一覧からターゲットを探す。
    return pick_nearest_interest_target(pos, dir);
}

/*!
 * @brief カーソル移動方向に "interesting" な座標がなかったとき、画面外まで探す。
 *
 * @return 画面外を探し "interesting" な座標が見つかった場合はそのインデックス、それでも見つからなければ std::nullopt
 */
std::optional<int> TargetSetter::sweep_targets(int dir, int panel_row_min_initial, int panel_col_min_initial)
{
    while (change_panel(this->player_ptr, ddy[dir], ddx[dir])) {
        // カーソル移動に伴い、必要なだけ描画範囲を更新。
        // "interesting" 座標リストおよび現在のターゲットも更新。
        const auto target_index = this->check_panel_changed(dir);
        if (target_index) {
            return target_index;
        }
    }

    panel_row_min = panel_row_min_initial;
    panel_col_min = panel_col_min_initial;
    panel_bounds_center();

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(SubWindowRedrawingFlag::OVERHEAD);
    handle_stuff(this->player_ptr);

    this->pos_interests = target_set_prepare(this->player_ptr, this->mode);

    auto &[y, x] = this->pos_target;
    auto dy = ddy[dir];
    auto dx = ddx[dir];
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
            this->pos_interests = target_set_prepare(this->player_ptr, this->mode);
        }
    }

    const auto &floor = *this->player_ptr->current_floor_ptr;
    x = std::clamp(x, 1, floor.width - 2);
    y = std::clamp(y, 1, floor.height - 2);
    return std::nullopt;
}

bool TargetSetter::set_target_grid()
{
    if (this->pos_interests.empty() || !this->interest_index) {
        return false;
    }

    this->pos_target = this->pos_interests[*this->interest_index];

    fix_floor_item_list(this->player_ptr, this->pos_target);

    const auto dir = this->switch_target_input();
    if (!dir) {
        return true;
    }

    auto target_index = pick_nearest_interest_target(this->pos_interests[*this->interest_index], *dir);
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

    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(this->pos_target);
    constexpr auto fmt = " X:%d Y:%d LOS:%d LOP:%d SPECIAL:%d";
    const auto p_pos = this->player_ptr->get_position();
    const auto is_los = los(floor, p_pos, this->pos_target);
    const auto is_projectable = projectable(this->player_ptr, p_pos, this->pos_target);
    const auto cheatinfo = format(fmt, this->pos_target.x, this->pos_target.y, is_los, is_projectable, grid.special);
    return cheatinfo;
}

/*!
 * @brief 隣接マスへのターゲット切り替えモードでのコマンドを処理する
 *
 * @return 移動方向と早く移動するかどうかのペア。移動しない場合はstd::nullopt
 */
std::optional<std::pair<int, bool>> TargetSetter::switch_next_grid_command()
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
        this->pos_interests = target_set_prepare(this->player_ptr, this->mode);
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
        const auto begin = this->pos_interests.begin();
        const auto end = this->pos_interests.end();
        const auto target = std::min_element(begin, end, [&](const auto &lhs, const auto &rhs) {
            return Grid::calc_distance(this->pos_target, lhs) < Grid::calc_distance(this->pos_target, rhs);
        });
        if (target != end) {
            this->interest_index = std::distance(this->pos_interests.begin(), target);
        } else {
            this->interest_index = std::nullopt;
        }

        return std::nullopt;
    }
    default:
        const auto dir = get_keymap_dir(query);
        if (dir == 0) {
            bell();
            return std::nullopt;
        }

        const auto move_fast = isupper(query) != 0;
        return std::make_pair(dir, move_fast);
    }
}

void TargetSetter::decide_change_panel(int dir, bool move_fast)
{
    auto dx = ddx[dir];
    auto dy = ddy[dir];
    auto &[y, x] = this->pos_target;
    const auto [wid, hgt] = get_screen_size();
    const auto mag = move_fast ? std::min(wid / 2, hgt / 2) : 1;
    x += dx * mag;
    y += dy * mag;

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
        this->pos_interests = target_set_prepare(this->player_ptr, this->mode);
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

        if ((this->mode & TARGET_LOOK) == 0) {
            print_path(this->player_ptr, this->pos_target.y, this->pos_target.x);
        }

        fix_floor_item_list(this->player_ptr, this->pos_target);

        if (auto dir_and_velocity = this->switch_next_grid_command(); dir_and_velocity) {
            const auto &[dir, move_fast] = *dir_and_velocity;
            this->decide_change_panel(dir, move_fast);
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
