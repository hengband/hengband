﻿/*!
 * @brief モンスターの魔法によってフロアを明るくする処理及びその判定
 * @date 2020/07/23
 * @author Hourier
 */

#include "mspell/mspell-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/line-of-sight.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-mask-definitions.h"
#include "spell/range-calc.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "target/projection-path-calculator.h"
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
        if (!cave_has_flag_grid(g_ptr, f_flag))
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
        if (!has_flag(f_ptr->flags, FF_LOS) && has_flag(f_ptr->flags, FF_PROJECT) && one_in_(2))
            msa_ptr->f4 &= ~(RF4_BR_LITE);
    } else if (!adjacent_grid_check(target_ptr, msa_ptr->m_ptr, &msa_ptr->y_br_lite, &msa_ptr->x_br_lite, FF_LOS, los))
        msa_ptr->f4 &= ~(RF4_BR_LITE);

    if ((msa_ptr->f4 & RF4_BR_LITE) != 0)
        return;

    msa_ptr->y_br_lite = 0;
    msa_ptr->x_br_lite = 0;
}

static void feature_projection(floor_type *floor_ptr, msa_type *msa_ptr)
{
    feature_type *f_ptr = &f_info[floor_ptr->grid_array[msa_ptr->y][msa_ptr->x].feat];
    if (has_flag(f_ptr->flags, FF_PROJECT))
        return;

    if ((msa_ptr->f4 & RF4_BR_DISI) && has_flag(f_ptr->flags, FF_HURT_DISI) && one_in_(2)) {
        msa_ptr->do_spell = DO_SPELL_BR_DISI;
        return;
    }

    if ((msa_ptr->f4 & RF4_BR_LITE) && has_flag(f_ptr->flags, FF_LOS) && one_in_(2))
        msa_ptr->do_spell = DO_SPELL_BR_LITE;
}

static void check_lite_area_by_mspell(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->f4 & RF4_BR_DISI) && (msa_ptr->m_ptr->cdis < get_max_range(target_ptr) / 2)
        && in_disintegration_range(target_ptr->current_floor_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x)
        && (one_in_(10) || (projectable(target_ptr, msa_ptr->y, msa_ptr->x, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx) && one_in_(2)))) {
        msa_ptr->do_spell = DO_SPELL_BR_DISI;
        msa_ptr->success = TRUE;
        return;
    }

    if ((msa_ptr->f4 & RF4_BR_LITE) && (msa_ptr->m_ptr->cdis < get_max_range(target_ptr) / 2)
        && los(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x) && one_in_(5)) {
        msa_ptr->do_spell = DO_SPELL_BR_LITE;
        msa_ptr->success = TRUE;
        return;
    }

    if (((msa_ptr->f5 & RF5_BA_LITE) == 0) || (msa_ptr->m_ptr->cdis > get_max_range(target_ptr)))
        return;

    POSITION by = msa_ptr->y, bx = msa_ptr->x;
    get_project_point(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, &by, &bx, 0L);
    if ((distance(by, bx, msa_ptr->y, msa_ptr->x) <= 3) && los(target_ptr, by, bx, msa_ptr->y, msa_ptr->x) && one_in_(5)) {
        msa_ptr->do_spell = DO_SPELL_BA_LITE;
        msa_ptr->success = TRUE;
    }
}

static void decide_lite_breath(player_type *target_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->success)
        return;

    if (msa_ptr->m_ptr->target_y && msa_ptr->m_ptr->target_x) {
        msa_ptr->y = msa_ptr->m_ptr->target_y;
        msa_ptr->x = msa_ptr->m_ptr->target_x;
        msa_ptr->f4 &= RF4_INDIRECT_MASK;
        msa_ptr->f5 &= RF5_INDIRECT_MASK;
        msa_ptr->f6 &= RF6_INDIRECT_MASK;
        msa_ptr->success = TRUE;
    }

    if ((msa_ptr->y_br_lite == 0) || (msa_ptr->x_br_lite == 0) || (msa_ptr->m_ptr->cdis > get_max_range(target_ptr) / 2) || !one_in_(5))
        return;

    if (msa_ptr->success) {
        msa_ptr->f4 |= RF4_BR_LITE;
        return;
    }

    msa_ptr->y = msa_ptr->y_br_lite;
    msa_ptr->x = msa_ptr->x_br_lite;
    msa_ptr->do_spell = DO_SPELL_BR_LITE;
    msa_ptr->success = TRUE;
}

bool decide_lite_projection(player_type *target_ptr, msa_type *msa_ptr)
{
    if (projectable(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x)) {
        feature_projection(target_ptr->current_floor_ptr, msa_ptr);
        return TRUE;
    }

    msa_ptr->success = FALSE;
    check_lite_area_by_mspell(target_ptr, msa_ptr);
    if (!msa_ptr->success)
        msa_ptr->success = adjacent_grid_check(target_ptr, msa_ptr->m_ptr, &msa_ptr->y, &msa_ptr->x, FF_PROJECT, projectable);

    decide_lite_breath(target_ptr, msa_ptr);
    return msa_ptr->success;
}

void decide_lite_area(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->f6 & RF6_DARKNESS) == 0)
        return;

    bool can_use_lite_area = (target_ptr->pclass == CLASS_NINJA) && ((msa_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) == 0)
        && ((msa_ptr->r_ptr->flags7 & RF7_DARK_MASK) == 0);

    if ((msa_ptr->r_ptr->flags2 & RF2_STUPID) != 0)
        return;

    if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS) {
        msa_ptr->f6 &= ~(RF6_DARKNESS);
        return;
    }

    if ((target_ptr->pclass == CLASS_NINJA) && !can_use_lite_area)
        msa_ptr->f6 &= ~(RF6_DARKNESS);
}
