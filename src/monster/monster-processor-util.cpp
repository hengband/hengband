/*!
 * @brief monster-processのための構造体群初期化処理と共通性の極めて高い処理
 * @date 2020/03/07
 * @author Hourier
 * @details
 * 概ね、PlayerType 構造体が引数でない場合はここへ移動させることを検討しても良い
 * 引数に入っていたらここには移動させないこと
 */

#include "monster/monster-processor-util.h"
#include "floor/geometry.h"
#include "monster/monster-status.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/redrawing-flags-updater.h"

/*!
 * @brief ターン経過フラグ構造体の初期化
 * @param riding_idx 乗馬中のモンスターID
 * @param m_idx モンスターID
 * @return 初期化済のターン経過フラグ
 */
turn_flags *init_turn_flags(bool is_riding, turn_flags *turn_flags_ptr)
{
    turn_flags_ptr->is_riding_mon = is_riding;
    turn_flags_ptr->do_turn = false;
    turn_flags_ptr->do_move = false;
    turn_flags_ptr->do_view = false;
    turn_flags_ptr->must_alter_to_move = false;
    turn_flags_ptr->did_open_door = false;
    turn_flags_ptr->did_bash_door = false;
    turn_flags_ptr->did_take_item = false;
    turn_flags_ptr->did_kill_item = false;
    turn_flags_ptr->did_move_body = false;
    turn_flags_ptr->did_pass_wall = false;
    turn_flags_ptr->did_kill_wall = false;
    return turn_flags_ptr;
}

/*!
 * @brief モンスターの移動方向を保存する
 * @param mm 移動方向
 * @param y 移動先Y座標
 * @param x 移動先X座標
 */
void store_enemy_approch_direction(std::span<Direction> mm, POSITION y, POSITION x)
{
    /* North, South, East, West, North-West, North-East, South-West, South-East */
    if ((y < 0) && (x == 0)) {
        mm[0] = Direction(8);
        mm[1] = Direction(7);
        mm[2] = Direction(9);
    } else if ((y > 0) && (x == 0)) {
        mm[0] = Direction(2);
        mm[1] = Direction(1);
        mm[2] = Direction(3);
    } else if ((x > 0) && (y == 0)) {
        mm[0] = Direction(6);
        mm[1] = Direction(9);
        mm[2] = Direction(3);
    } else if ((x < 0) && (y == 0)) {
        mm[0] = Direction(4);
        mm[1] = Direction(7);
        mm[2] = Direction(1);
    } else if ((y < 0) && (x < 0)) {
        mm[0] = Direction(7);
        mm[1] = Direction(4);
        mm[2] = Direction(8);
    } else if ((y < 0) && (x > 0)) {
        mm[0] = Direction(9);
        mm[1] = Direction(6);
        mm[2] = Direction(8);
    } else if ((y > 0) && (x < 0)) {
        mm[0] = Direction(1);
        mm[1] = Direction(4);
        mm[2] = Direction(2);
    } else if ((y > 0) && (x > 0)) {
        mm[0] = Direction(3);
        mm[1] = Direction(6);
        mm[2] = Direction(2);
    }
}

/*!
 * @brief get_movable_grid() における移動の方向を保存する
 * @param mm 移動方向
 * @param vec 移動方向のベクトル
 */
void store_moves_val(std::span<Direction> mm, const Pos2DVec &vec)
{
    const Pos2DVec vec_abs(std::abs(vec.y), std::abs(vec.x));
    auto move_val = 0;
    if (vec.y < 0) {
        move_val += 8;
    }
    if (vec.x > 0) {
        move_val += 4;
    }

    if (vec_abs.y > (vec_abs.x << 1)) {
        move_val += 2;
    } else if (vec_abs.x > (vec_abs.y << 1)) {
        move_val++;
    }

    auto is_left_first = false;
    switch (move_val) {
    case 0: {
        mm[0] = Direction(9);
        is_left_first = vec_abs.y > vec_abs.x;
        break;
    }
    case 1:
    case 9: {
        mm[0] = Direction(6);
        is_left_first = vec.y >= 0;
        break;
    }
    case 2:
    case 6: {
        mm[0] = Direction(8);
        is_left_first = vec.x >= 0;
        break;
    }
    case 4: {
        mm[0] = Direction(7);
        is_left_first = vec_abs.y <= vec_abs.x;
        break;
    }
    case 5:
    case 13: {
        mm[0] = Direction(4);
        is_left_first = vec.y < 0;
        break;
    }
    case 8: {
        mm[0] = Direction(3);
        is_left_first = vec_abs.y <= vec_abs.x;
        break;
    }
    case 10:
    case 14: {
        mm[0] = Direction(2);
        is_left_first = vec.x < 0;
        break;
    }
    case 12: {
        mm[0] = Direction(1);
        is_left_first = vec_abs.y > vec_abs.x;
        break;
    default:
        return;
    }
    }

    mm[1] = mm[0].rotated_45degree(is_left_first ? 1 : -1);
    mm[2] = mm[0].rotated_45degree(is_left_first ? -1 : 1);
    mm[3] = mm[0].rotated_45degree(is_left_first ? 2 : -2);
    mm[4] = mm[0].rotated_45degree(is_left_first ? -2 : 2);
}

/*!
 * @brief 古いモンスター情報の保存
 * @param monrace_id モンスター種族ID
 */
OldRaceFlags::OldRaceFlags(MonraceId monrace_id)
{
    if (!MonraceList::is_valid(monrace_id)) {
        return;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    this->old_r_ability_flags = monrace.r_ability_flags;
    this->old_r_behavior_flags = monrace.r_behavior_flags;
    this->old_r_kind_flags = monrace.r_kind_flags;
    this->old_r_resistance_flags = monrace.r_resistance_flags;
    this->old_r_drop_flags = monrace.r_drop_flags;
    this->old_r_feature_flags = monrace.r_feature_flags;
    this->old_r_special_flags = monrace.r_special_flags;

    this->old_r_blows0 = monrace.r_blows[0];
    this->old_r_blows1 = monrace.r_blows[1];
    this->old_r_blows2 = monrace.r_blows[2];
    this->old_r_blows3 = monrace.r_blows[3];

    this->old_r_cast_spell = monrace.r_cast_spell;
}

/*!
 * @brief モンスターフラグの更新に基づき、モンスター表示を更新する
 * @param monrace 表示対象のモンスター種族定義
 */
void OldRaceFlags::update_lore_window_flag(const MonraceDefinition &monrace) const
{
    if ((this->old_r_ability_flags != monrace.r_ability_flags) ||
        (this->old_r_resistance_flags != monrace.r_resistance_flags) || (this->old_r_blows0 != monrace.r_blows[0]) ||
        (this->old_r_blows1 != monrace.r_blows[1]) || (this->old_r_blows2 != monrace.r_blows[2]) ||
        (this->old_r_blows3 != monrace.r_blows[3]) || (this->old_r_cast_spell != monrace.r_cast_spell) ||
        (this->old_r_behavior_flags != monrace.r_behavior_flags) || (this->old_r_kind_flags != monrace.r_kind_flags) ||
        (this->old_r_drop_flags != monrace.r_drop_flags) || (this->old_r_feature_flags != monrace.r_feature_flags) ||
        (this->old_r_special_flags != monrace.r_special_flags)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }
}
