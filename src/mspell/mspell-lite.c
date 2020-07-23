/*!
 * @brief モンスターの魔法によってフロアを明るくする処理及びその判定
 * @date 2020/07/23
 * @author Hourier
 */

#include "mspell/mspell-lite.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/race-flags4.h"
#include "mspell/mspell-attack-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 技能を使用するモンスター構造体の参照ポインタ
 * @param yp 最適な目標地点のY座標を返す参照ポインタ
 * @param xp 最適な目標地点のX座標を返す参照ポインタ
 * @param f_flag 射線に入れるのを避ける地形の所持フラグ
 * @param path_check 射線を判定するための関数ポインタ
 * @return 有効な座標があった場合TRUEを返す
 */
bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, int f_flag, path_check_pf path_check)
{
    static int tonari_y[4][8] = { { -1, -1, -1, 0, 0, 1, 1, 1 }, { -1, -1, -1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 0, 0, -1, -1, -1 }, { 1, 1, 1, 0, 0, -1, -1, -1 } };
    static int tonari_x[4][8] = { { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 }, { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 } };

    int next;
    if (m_ptr->fy < target_ptr->y && m_ptr->fx < target_ptr->x)
        next = 0;
    else if (m_ptr->fy < target_ptr->y)
        next = 1;
    else if (m_ptr->fx < target_ptr->x)
        next = 2;
    else
        next = 3;

    for (int i = 0; i < 8; i++) {
        int next_x = *xp + tonari_x[next][i];
        int next_y = *yp + tonari_y[next][i];
        grid_type *g_ptr;
        g_ptr = &target_ptr->current_floor_ptr->grid_array[next_y][next_x];
        if (!cave_have_flag_grid(g_ptr, f_flag))
            continue;

        if (path_check(target_ptr, m_ptr->fy, m_ptr->fx, next_y, next_x)) {
            *yp = next_y;
            *xp = next_x;
            return TRUE;
        }
    }

    return FALSE;
}

void decide_lite_range(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->f4 & RF4_BR_LITE) == 0)
        return;

    msa_ptr->y_br_lite = msa_ptr->y;
    msa_ptr->x_br_lite = msa_ptr->x;
    if (los(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y_br_lite, msa_ptr->x_br_lite)) {
        feature_type *f_ptr = &f_info[target_ptr->current_floor_ptr->grid_array[msa_ptr->y_br_lite][msa_ptr->x_br_lite].feat];
        if (!have_flag(f_ptr->flags, FF_LOS) && have_flag(f_ptr->flags, FF_PROJECT) && one_in_(2))
            msa_ptr->f4 &= ~(RF4_BR_LITE);
    } else if (!adjacent_grid_check(target_ptr, msa_ptr->m_ptr, &msa_ptr->y_br_lite, &msa_ptr->x_br_lite, FF_LOS, los))
        msa_ptr->f4 &= ~(RF4_BR_LITE);

    if ((msa_ptr->f4 & RF4_BR_LITE) != 0)
        return;

    msa_ptr->y_br_lite = 0;
    msa_ptr->x_br_lite = 0;
}
