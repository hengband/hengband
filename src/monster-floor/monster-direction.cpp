/*!
 * @brief モンスターの移動方向を決定する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-direction.h"
#include "floor/cave.h"
#include "monster-floor/monster-sweep-grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "spell/range-calc.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief ペットが敵に接近するための方向を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 移動を試みているモンスターへの参照ポインタ
 * @param t_ptr 移動先モンスターへの参照ポインタ
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @return ペットがモンスターに近づくならばTRUE
 */
static bool decide_pet_approch_direction(PlayerType *player_ptr, monster_type *m_ptr, monster_type *t_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!is_pet(m_ptr))
        return false;

    if (player_ptr->pet_follow_distance < 0) {
        if (t_ptr->cdis <= (0 - player_ptr->pet_follow_distance)) {
            return true;
        }
    } else if ((m_ptr->cdis < t_ptr->cdis) && (t_ptr->cdis > player_ptr->pet_follow_distance)) {
        return true;
    }

    return (r_ptr->aaf < t_ptr->cdis);
}

/*!
 * @brief モンスターが敵に接近するための方向を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param start モンスターIDの開始
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @param y モンスターの移動方向Y
 * @param x モンスターの移動方向X
 */
static void decide_enemy_approch_direction(PlayerType *player_ptr, MONSTER_IDX m_idx, int start, int plus, POSITION *y, POSITION *x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    for (int i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus) {
        MONSTER_IDX dummy = (i % floor_ptr->m_max);
        if (dummy == 0)
            continue;

        MONSTER_IDX t_idx = dummy;
        monster_type *t_ptr;
        t_ptr = &floor_ptr->m_list[t_idx];
        if (t_ptr == m_ptr)
            continue;
        if (!monster_is_valid(t_ptr))
            continue;
        if (decide_pet_approch_direction(player_ptr, m_ptr, t_ptr))
            continue;
        if (!are_enemies(player_ptr, m_ptr, t_ptr))
            continue;

        if (((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != player_ptr->riding) || has_pass_wall(player_ptr))) || ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != player_ptr->riding))) {
            if (!in_disintegration_range(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
                continue;
        } else {
            if (!projectable(player_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
                continue;
        }

        *y = t_ptr->fy;
        *x = t_ptr->fx;
        return;
    }
}

/*!
 * @brief モンスターが敵に接近するための方向を計算するメインルーチン
 * Calculate the direction to the next enemy
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターの参照ID
 * @param mm 移動するべき方角IDを返す参照ポインタ
 * @return 方向が確定した場合TRUE、接近する敵がそもそもいない場合FALSEを返す
 */
bool get_enemy_dir(PlayerType *player_ptr, MONSTER_IDX m_idx, int *mm)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];

    POSITION x = 0, y = 0;
    if (player_ptr->riding_t_m_idx && player_bold(player_ptr, m_ptr->fy, m_ptr->fx)) {
        y = floor_ptr->m_list[player_ptr->riding_t_m_idx].fy;
        x = floor_ptr->m_list[player_ptr->riding_t_m_idx].fx;
    } else if (is_pet(m_ptr) && player_ptr->pet_t_m_idx) {
        y = floor_ptr->m_list[player_ptr->pet_t_m_idx].fy;
        x = floor_ptr->m_list[player_ptr->pet_t_m_idx].fx;
    } else {
        int start;
        int plus = 1;
        if (player_ptr->phase_out) {
            start = randint1(floor_ptr->m_max - 1) + floor_ptr->m_max;
            if (randint0(2))
                plus = -1;
        } else {
            start = floor_ptr->m_max + 1;
        }

        decide_enemy_approch_direction(player_ptr, m_idx, start, plus, &y, &x);

        if ((x == 0) && (y == 0))
            return false;
    }

    x -= m_ptr->fx;
    y -= m_ptr->fy;

    store_enemy_approch_direction(mm, y, x);
    return true;
}

/*!
 * @brief 不規則歩行フラグを持つモンスターの移動方向をその確率に基づいて決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_ptr モンスターへの参照ポインタ
 * @return 不規則な方向へ歩くことになったらTRUE
 * @todo "5"とはもしかして「その場に留まる」という意味か？
 */
static bool random_walk(PlayerType *player_ptr, DIRECTION *mm, monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && (randint0(100) < 75)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr))
            r_ptr->r_behavior_flags.set({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 });

        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        return true;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && (randint0(100) < 50)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr))
            r_ptr->r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_50);

        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        return true;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25) && (randint0(100) < 25)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr))
            r_ptr->r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_25);

        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        return true;
    }

    return false;
}

/*!
 * @brief ペットや友好的なモンスターがフロアから逃げる処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_idx モンスターID
 * @return モンスターがペットであればTRUE
 */
static bool decide_pet_movement_direction(MonsterSweepGrid *msd)
{
    auto *m_ptr = &msd->player_ptr->current_floor_ptr->m_list[msd->m_idx];
    if (!is_pet(m_ptr)) {
        return false;
    }

    bool avoid = ((msd->player_ptr->pet_follow_distance < 0) && (m_ptr->cdis <= (0 - msd->player_ptr->pet_follow_distance)));
    bool lonely = (!avoid && (m_ptr->cdis > msd->player_ptr->pet_follow_distance));
    bool distant = (m_ptr->cdis > PET_SEEK_DIST);
    msd->mm[0] = msd->mm[1] = msd->mm[2] = msd->mm[3] = 5;
    if (get_enemy_dir(msd->player_ptr, msd->m_idx, msd->mm)) {
        return true;
    }

    if (!avoid && !lonely && !distant) {
        return true;
    }

    POSITION dis = msd->player_ptr->pet_follow_distance;
    if (msd->player_ptr->pet_follow_distance > PET_SEEK_DIST) {
        msd->player_ptr->pet_follow_distance = PET_SEEK_DIST;
    }

    (void)msd->get_movable_grid();
    msd->player_ptr->pet_follow_distance = (int16_t)dis;
    return true;
}

/*!
 * @brief モンスターの移動パターンを決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mm 移動方向
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 移動先が存在すればTRUE
 */
bool decide_monster_movement_direction(PlayerType *player_ptr, DIRECTION *mm, MONSTER_IDX m_idx, bool aware)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &r_info[m_ptr->r_idx];

    if (monster_confused_remaining(m_ptr) || !aware) {
        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        return true;
    }

    if (random_walk(player_ptr, mm, m_ptr)) {
        return true;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) && (m_ptr->cdis > 1)) {
        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        return true;
    }

    MonsterSweepGrid msd(player_ptr, m_idx, mm);
    if (decide_pet_movement_direction(&msd)) {
        return true;
    }

    if (!is_hostile(m_ptr)) {
        mm[0] = mm[1] = mm[2] = mm[3] = 5;
        get_enemy_dir(player_ptr, m_idx, mm);
        return true;
    }

    return msd.get_movable_grid();
}
