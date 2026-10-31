/*!
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 * @todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 */

#include "load/extra-loader.h"
#include "io/input-key-requester.h"
#include "load/angband-version-comparer.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/world-loader.h"
#include "player/attack-defense-types.h"
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
    // 方向は Direction に渡すと範囲外で例外を投げるので、壊れた値は方向なしにする
    const auto fishing_dir = rd_byte();
    player_ptr->fishing_dir = Direction::is_valid_dir(fishing_dir) ? fishing_dir : 0;
    command_cmd = rd_s16b();
    command_arg = rd_s16b();
    command_rep = rd_s16b();
    const auto dir_id = rd_byte();
    const auto dir = Direction::is_valid_dir(dir_id) ? Direction(dir_id) : Direction::none();
    command_dir = rd_bool() ? Direction::targetting(Target::get_last_target()) : dir;
    panel_row_min = rd_s16b();
    panel_col_min = rd_s16b();
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
}
