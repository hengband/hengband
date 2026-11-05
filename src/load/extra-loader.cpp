/*!
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 * @todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 */

#include "load/extra-loader.h"
#include "action/run-execution.h"
#include "action/travel-execution.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "load/angband-version-comparer.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/world-loader.h"
#include "player/attack-defense-types.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "target/target.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief 最後にターゲットしたものを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details モンスターの番号は、この後で読み込む今の階のモンスターを指す
 */
static void rd_last_target(PlayerType *player_ptr)
{
    switch (rd_byte()) {
    case 1: {
        const auto y = rd_s16b();
        const auto x = rd_s16b();
        Target::set_last_target(Target::create_grid_target(player_ptr, { y, x }));
        return;
    }
    case 2:
        Target::set_last_target(Target::create_monster_target(player_ptr, rd_s16b()));
        return;
    default:
        Target::clear_last_target();
        return;
    }
}

/*!
 * @brief 方向を読み込む
 * @return 読み込んだ方向
 * @details Direction は範囲外の値で例外を投げるので、壊れた値は方向なしにする
 */
static Direction rd_direction()
{
    const auto dir = rd_byte();
    return Direction::is_valid_dir(dir) ? Direction(dir) : Direction::none();
}

/*!
 * @brief 休憩・釣り・コマンドの繰り返しの続きと、画面に表示している範囲を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 古いセーブには無いので、続きの分からない休憩と釣りはやめ、繰り返しも無い状態から始める。
 * 表示している範囲は負にして、generate_world() で決め直させる
 */
static void rd_continuous_actions(PlayerType *player_ptr)
{
    if (loading_savefile_version_is_older_than(30)) {
        if ((player_ptr->action == ACTION_REST) || (player_ptr->action == ACTION_FISH)) {
            player_ptr->action = ACTION_NONE;
        }

        panel_row_min = -1;
        panel_col_min = -1;
        return;
    }

    player_ptr->resting = rd_s32b();
    player_ptr->fishing_dir = rd_direction().dir();
    command_cmd = rd_s16b();
    command_arg = rd_s16b();
    command_rep = rd_s16b();
    const auto dir = rd_direction();
    command_dir = rd_bool() ? Direction::targetting(Target::get_last_target()) : dir;
    panel_row_min = rd_s16b();
    panel_col_min = rd_s16b();
}

/*!
 * @brief 走りとトラベルの続きを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 版 31 より古いセーブには無いので、走りもトラベルもしていない状態から始める
 */
static void rd_running_and_travel(PlayerType *player_ptr)
{
    if (loading_savefile_version_is_older_than(31)) {
        return;
    }

    player_ptr->running = rd_s16b();
    player_ptr->run_py = rd_s16b();
    player_ptr->run_px = rd_s16b();
    const auto current = rd_direction();
    const auto prevdir = rd_direction();
    const auto openarea = rd_bool();
    const auto breakright = rd_bool();
    const auto breakleft = rd_bool();
    set_running_course({ current, prevdir, openarea, breakright, breakleft });
    ignore_avoid_run = rd_bool();
    const auto flow_center_y = rd_s16b();
    const auto flow_center_x = rd_s16b();
    set_flow_center({ flow_center_y, flow_center_x });

    auto &travel = Travel::get_instance();
    if (!rd_bool()) {
        travel.reset_goal();
        return;
    }

    // 範囲外の値は配列の外を指したり、行程を書いたかどうかの判断を誤って以降を読み違えたりするので、壊れたセーブとして扱う。
    // 目的地は、行程を作るときに周りの 8 マスを見るので、配列の端も除く
    const auto throw_broken = [] { throw SaveDataNotSupportedException(_("トラベルのデータが壊れています。", "The travel data is broken.")); };
    const auto goal_y = rd_s16b();
    const auto goal_x = rd_s16b();
    const auto state = rd_byte();
    if ((goal_y < 1) || (goal_y >= MAX_HGT - 1) || (goal_x < 1) || (goal_x >= MAX_WID - 1) || (state > static_cast<uint8_t>(TravelState::EXECUTING))) {
        throw_broken();
    }

    const auto travel_state = static_cast<TravelState>(state);
    const auto dir = rd_direction();
    short height = 0;
    short width = 0;
    if (travel_state != TravelState::STOP) {
        height = rd_s16b();
        width = rd_s16b();
        if ((height < 1) || (height > MAX_HGT) || (width < 1) || (width > MAX_WID)) {
            throw_broken();
        }
    }

    travel.restore(Pos2D(goal_y, goal_x), travel_state, dir);
    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            travel.set_cost({ y, x }, rd_s16b());
        }
    }
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_extra(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 7)) {
        player_ptr->ride_monster(0);
    } else {
        player_ptr->ride_monster(rd_s16b());
    }

    if (h_older_than(1, 5, 0, 0)) {
        player_ptr->floor_id = 0;
    } else {
        player_ptr->floor_id = rd_s16b();
    }

    rd_dummy_monsters(player_ptr);
    auto &world = AngbandWorld::get_instance();
    if (h_older_than(0, 1, 2)) {
        world.play_time = ElapsedTime();
    } else {
        world.play_time = ElapsedTime(rd_u32b());
    }

    rd_visited_towns();
    if (!h_older_than(1, 0, 5)) {
        player_ptr->count = rd_u32b();
    }

    if (!loading_savefile_version_is_older_than(29)) {
        player_ptr->pet_t_m_idx = rd_s16b();
        player_ptr->riding_t_m_idx = rd_s16b();
        rd_last_target(player_ptr);
    } else {
        player_ptr->pet_t_m_idx = 0;
        player_ptr->riding_t_m_idx = 0;
        Target::clear_last_target();
    }

    rd_continuous_actions(player_ptr);
    rd_running_and_travel(player_ptr);
}
