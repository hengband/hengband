/*!
 * @brief monster-processのための構造体群初期化処理と共通性の極めて高い処理
 * @date 2020/03/07
 * @author Hourier
 * @details
 * 概ね、PlayerType 構造体が引数でない場合はここへ移動させることを検討しても良い
 * 引数に入っていたらここには移動させないこと
 */

#include "monster/monster-processor-util.h"
#include "monster-race/monster-race.h"
#include "monster/monster-status.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"

/*!
 * @brief ターン経過フラグ構造体の初期化
 * @param riding_idx 乗馬中のモンスターID
 * @param m_idx モンスターID
 * @return 初期化済のターン経過フラグ
 */
turn_flags *init_turn_flags(MONSTER_IDX riding_idx, MONSTER_IDX m_idx, turn_flags *turn_flags_ptr)
{
    turn_flags_ptr->is_riding_mon = (m_idx == riding_idx);
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
 * @brief old_race_flags_ptr の初期化
 */
old_race_flags *init_old_race_flags(old_race_flags *old_race_flags_ptr)
{
    old_race_flags_ptr->old_r_flags1 = 0L;
    old_race_flags_ptr->old_r_flags2 = 0L;
    old_race_flags_ptr->old_r_flags3 = 0L;
    old_race_flags_ptr->old_r_resistance_flags.clear();
    old_race_flags_ptr->old_r_ability_flags.clear();
    old_race_flags_ptr->old_r_behavior_flags.clear();
    old_race_flags_ptr->old_r_kind_flags.clear();
    old_race_flags_ptr->old_r_drop_flags.clear();
    old_race_flags_ptr->old_r_feature_flags.clear();

    old_race_flags_ptr->old_r_blows0 = 0;
    old_race_flags_ptr->old_r_blows1 = 0;
    old_race_flags_ptr->old_r_blows2 = 0;
    old_race_flags_ptr->old_r_blows3 = 0;

    old_race_flags_ptr->old_r_cast_spell = 0;
    return old_race_flags_ptr;
}

/*!
 * @brief coordinate_candidate の初期化
 * @param なし
 * @return 初期化済の構造体
 */
coordinate_candidate init_coordinate_candidate(void)
{
    coordinate_candidate candidate;
    candidate.gy = 0;
    candidate.gx = 0;
    candidate.gdis = 0;
    return candidate;
}

/*!
 * @brief モンスターの移動方向を保存する
 * @param mm 移動方向
 * @param y 移動先Y座標
 * @param x 移動先X座標
 */
void store_enemy_approch_direction(int *mm, POSITION y, POSITION x)
{
    /* North, South, East, West, North-West, North-East, South-West, South-East */
    if ((y < 0) && (x == 0)) {
        mm[0] = 8;
        mm[1] = 7;
        mm[2] = 9;
    } else if ((y > 0) && (x == 0)) {
        mm[0] = 2;
        mm[1] = 1;
        mm[2] = 3;
    } else if ((x > 0) && (y == 0)) {
        mm[0] = 6;
        mm[1] = 9;
        mm[2] = 3;
    } else if ((x < 0) && (y == 0)) {
        mm[0] = 4;
        mm[1] = 7;
        mm[2] = 1;
    } else if ((y < 0) && (x < 0)) {
        mm[0] = 7;
        mm[1] = 4;
        mm[2] = 8;
    } else if ((y < 0) && (x > 0)) {
        mm[0] = 9;
        mm[1] = 6;
        mm[2] = 8;
    } else if ((y > 0) && (x < 0)) {
        mm[0] = 1;
        mm[1] = 4;
        mm[2] = 2;
    } else if ((y > 0) && (x > 0)) {
        mm[0] = 3;
        mm[1] = 6;
        mm[2] = 2;
    }
}

/*!
 * @brief get_movable_grid() における移動の方向を保存する
 * @param mm 移動方向
 * @param y 移動先Y座標
 * @param x 移動先X座標
 */
void store_moves_val(int *mm, int y, int x)
{
    POSITION ax = std::abs(x);
    POSITION ay = std::abs(y);

    int move_val = 0;
    if (y < 0) {
        move_val += 8;
    }
    if (x > 0) {
        move_val += 4;
    }

    if (ay > (ax << 1)) {
        move_val += 2;
    } else if (ax > (ay << 1)) {
        move_val++;
    }

    switch (move_val) {
    case 0: {
        mm[0] = 9;
        if (ay > ax) {
            mm[1] = 8;
            mm[2] = 6;
            mm[3] = 7;
            mm[4] = 3;
        } else {
            mm[1] = 6;
            mm[2] = 8;
            mm[3] = 3;
            mm[4] = 7;
        }

        break;
    }
    case 1:
    case 9: {
        mm[0] = 6;
        if (y < 0) {
            mm[1] = 3;
            mm[2] = 9;
            mm[3] = 2;
            mm[4] = 8;
        } else {
            mm[1] = 9;
            mm[2] = 3;
            mm[3] = 8;
            mm[4] = 2;
        }

        break;
    }
    case 2:
    case 6: {
        mm[0] = 8;
        if (x < 0) {
            mm[1] = 9;
            mm[2] = 7;
            mm[3] = 6;
            mm[4] = 4;
        } else {
            mm[1] = 7;
            mm[2] = 9;
            mm[3] = 4;
            mm[4] = 6;
        }

        break;
    }
    case 4: {
        mm[0] = 7;
        if (ay > ax) {
            mm[1] = 8;
            mm[2] = 4;
            mm[3] = 9;
            mm[4] = 1;
        } else {
            mm[1] = 4;
            mm[2] = 8;
            mm[3] = 1;
            mm[4] = 9;
        }

        break;
    }
    case 5:
    case 13: {
        mm[0] = 4;
        if (y < 0) {
            mm[1] = 1;
            mm[2] = 7;
            mm[3] = 2;
            mm[4] = 8;
        } else {
            mm[1] = 7;
            mm[2] = 1;
            mm[3] = 8;
            mm[4] = 2;
        }

        break;
    }
    case 8: {
        mm[0] = 3;
        if (ay > ax) {
            mm[1] = 2;
            mm[2] = 6;
            mm[3] = 1;
            mm[4] = 9;
        } else {
            mm[1] = 6;
            mm[2] = 2;
            mm[3] = 9;
            mm[4] = 1;
        }

        break;
    }
    case 10:
    case 14: {
        mm[0] = 2;
        if (x < 0) {
            mm[1] = 3;
            mm[2] = 1;
            mm[3] = 6;
            mm[4] = 4;
        } else {
            mm[1] = 1;
            mm[2] = 3;
            mm[3] = 4;
            mm[4] = 6;
        }

        break;
    }
    case 12: {
        mm[0] = 1;
        if (ay > ax) {
            mm[1] = 2;
            mm[2] = 4;
            mm[3] = 3;
            mm[4] = 7;
        } else {
            mm[1] = 4;
            mm[2] = 2;
            mm[3] = 7;
            mm[4] = 3;
        }

        break;
    }
    }
}

/*!
 * @brief 古いモンスター情報の保存
 * @param monster_race_idx モンスターID
 * @param old_race_flags_ptr モンスターフラグへの参照ポインタ
 */
void save_old_race_flags(MonsterRaceId monster_race_idx, old_race_flags *old_race_flags_ptr)
{
    if (!MonsterRace(monster_race_idx).is_valid()) {
        return;
    }

    MonsterRaceInfo *r_ptr;
    r_ptr = &monraces_info[monster_race_idx];

    old_race_flags_ptr->old_r_flags1 = r_ptr->r_flags1;
    old_race_flags_ptr->old_r_flags2 = r_ptr->r_flags2;
    old_race_flags_ptr->old_r_flags3 = r_ptr->r_flags3;
    old_race_flags_ptr->old_r_resistance_flags = r_ptr->r_resistance_flags;
    old_race_flags_ptr->old_r_ability_flags = r_ptr->r_ability_flags;
    old_race_flags_ptr->old_r_behavior_flags = r_ptr->r_behavior_flags;
    old_race_flags_ptr->old_r_drop_flags = r_ptr->r_drop_flags;
    old_race_flags_ptr->old_r_feature_flags = r_ptr->r_feature_flags;

    old_race_flags_ptr->old_r_blows0 = r_ptr->r_blows[0];
    old_race_flags_ptr->old_r_blows1 = r_ptr->r_blows[1];
    old_race_flags_ptr->old_r_blows2 = r_ptr->r_blows[2];
    old_race_flags_ptr->old_r_blows3 = r_ptr->r_blows[3];

    old_race_flags_ptr->old_r_cast_spell = r_ptr->r_cast_spell;
}
