/*!
 * @brief モンスターの移動方向を決定する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-direction.h"
#include "floor/cave.h"
#include "monster-floor/monster-sweep-grid.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief ペットが敵に接近するための方向を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster_from 移動を試みているモンスターへの参照ポインタ
 * @param monster_to 移動先モンスターへの参照ポインタ
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @return ペットがモンスターに近づくならばTRUE
 */
static bool decide_pet_approch_direction(PlayerType *player_ptr, const MonsterEntity &monster_from, const MonsterEntity &monster_to)
{
    if (!monster_from.is_pet()) {
        return false;
    }

    if (player_ptr->pet_follow_distance < 0) {
        if (monster_to.cdis <= (0 - player_ptr->pet_follow_distance)) {
            return true;
        }
    } else if ((monster_from.cdis < monster_to.cdis) && (monster_to.cdis > player_ptr->pet_follow_distance)) {
        return true;
    }

    const auto &monrace = monster_from.get_monrace();
    return monrace.aaf < monster_to.cdis;
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
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_from = floor.m_list[m_idx];
    const auto &monrace = monster_from.get_monrace();
    for (int i = start; ((i < start + floor.m_max) && (i > start - floor.m_max)); i += plus) {
        const auto dummy = (i % floor.m_max);
        if (dummy == 0) {
            continue;
        }

        const auto t_idx = dummy;
        const auto &monster_to = floor.m_list[t_idx];
        if (&monster_to == &monster_from) {
            continue;
        }
        if (!monster_to.is_valid()) {
            continue;
        }
        if (decide_pet_approch_direction(player_ptr, monster_from, monster_to)) {
            continue;
        }
        if (!monster_from.is_hostile_to_melee(monster_to)) {
            continue;
        }

        const auto can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster_from.is_riding() || has_pass_wall(player_ptr));
        const auto can_kill_wall = monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster_from.is_riding();
        const auto m_pos_from = monster_from.get_position();
        const auto m_pos_to = monster_to.get_position();
        if (can_pass_wall || can_kill_wall) {
            if (!in_disintegration_range(floor, m_pos_from, m_pos_to)) {
                continue;
            }
        } else {
            if (!projectable(player_ptr, m_pos_from, m_pos_to)) {
                continue;
            }
        }

        *y = monster_to.fy;
        *x = monster_to.fx;
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
static bool get_enemy_dir(PlayerType *player_ptr, MONSTER_IDX m_idx, std::span<Direction> mm)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];

    POSITION x = 0, y = 0;
    if (player_ptr->riding_t_m_idx && player_ptr->is_located_at({ monster.fy, monster.fx })) {
        y = floor.m_list[player_ptr->riding_t_m_idx].fy;
        x = floor.m_list[player_ptr->riding_t_m_idx].fx;
    } else if (monster.is_pet() && player_ptr->pet_t_m_idx) {
        y = floor.m_list[player_ptr->pet_t_m_idx].fy;
        x = floor.m_list[player_ptr->pet_t_m_idx].fx;
    } else {
        int start;
        int plus = 1;
        if (AngbandSystem::get_instance().is_phase_out()) {
            start = randint1(floor.m_max - 1) + floor.m_max;
            if (randint0(2)) {
                plus = -1;
            }
        } else {
            start = floor.m_max + 1;
        }

        decide_enemy_approch_direction(player_ptr, m_idx, start, plus, &y, &x);

        if ((x == 0) && (y == 0)) {
            return false;
        }
    }

    x -= monster.fx;
    y -= monster.fy;

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
static bool random_walk(PlayerType *player_ptr, std::span<Direction> mm, const MonsterEntity &monster)
{
    auto &monrace = monster.get_monrace();
    if (monrace.behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && evaluate_percent(75)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_behavior_flags.set({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 });
        }

        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
        return true;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && one_in_(2)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_50);
        }

        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
        return true;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25) && one_in_(4)) {
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_25);
        }

        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
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
    const auto &monster = msd->player_ptr->current_floor_ptr->m_list[msd->m_idx];
    if (!monster.is_pet()) {
        return false;
    }

    bool avoid = ((msd->player_ptr->pet_follow_distance < 0) && (monster.cdis <= (0 - msd->player_ptr->pet_follow_distance)));
    bool lonely = (!avoid && (monster.cdis > msd->player_ptr->pet_follow_distance));
    bool distant = (monster.cdis > PET_SEEK_DIST);
    msd->mm[0] = msd->mm[1] = msd->mm[2] = msd->mm[3] = Direction::self();
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
bool decide_monster_movement_direction(PlayerType *player_ptr, std::span<Direction> mm, MONSTER_IDX m_idx, bool aware)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    if (monster.is_confused() || !aware) {
        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
        return true;
    }

    if (random_walk(player_ptr, mm, monster)) {
        return true;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) && (monster.cdis > 1)) {
        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
        return true;
    }

    MonsterSweepGrid msd(player_ptr, m_idx, mm);
    if (decide_pet_movement_direction(&msd)) {
        return true;
    }

    if (!monster.is_hostile()) {
        mm[0] = mm[1] = mm[2] = mm[3] = Direction::self();
        get_enemy_dir(player_ptr, m_idx, mm);
        return true;
    }

    return msd.get_movable_grid();
}
