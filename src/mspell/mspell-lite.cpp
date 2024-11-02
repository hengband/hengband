/*!
 * @brief モンスターの魔法によってフロアを明るくする処理及びその判定
 * @date 2020/07/23
 * @author Hourier
 */

#include "mspell/mspell-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "player-base/player-class.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 技能を使用するモンスター構造体の参照ポインタ
 * @param yp 最適な目標地点のY座標を返す参照ポインタ
 * @param xp 最適な目標地点のX座標を返す参照ポインタ
 * @param f_flag 射線に入れるのを避ける地形の所持フラグ
 * @param path_check 射線を判定するための関数ポインタ
 * @return 有効な座標があった場合TRUEを返す
 */
bool adjacent_grid_check(PlayerType *player_ptr, MonsterEntity *m_ptr, POSITION *yp, POSITION *xp, TerrainCharacteristics f_flag, path_check_pf path_check)
{
    static int tonari_y[4][8] = { { -1, -1, -1, 0, 0, 1, 1, 1 }, { -1, -1, -1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 0, 0, -1, -1, -1 }, { 1, 1, 1, 0, 0, -1, -1, -1 } };
    static int tonari_x[4][8] = { { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 }, { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 } };

    int next;
    if (m_ptr->fy < player_ptr->y && m_ptr->fx < player_ptr->x) {
        next = 0;
    } else if (m_ptr->fy < player_ptr->y) {
        next = 1;
    } else if (m_ptr->fx < player_ptr->x) {
        next = 2;
    } else {
        next = 3;
    }

    for (int i = 0; i < 8; i++) {
        int next_x = *xp + tonari_x[next][i];
        int next_y = *yp + tonari_y[next][i];
        Grid *g_ptr;
        g_ptr = &player_ptr->current_floor_ptr->grid_array[next_y][next_x];
        if (!g_ptr->cave_has_flag(f_flag)) {
            continue;
        }

        if (path_check(player_ptr, m_ptr->fy, m_ptr->fx, next_y, next_x)) {
            *yp = next_y;
            *xp = next_x;
            return true;
        }
    }

    return false;
}

void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->ability_flags.has_not(MonsterAbilityType::BR_LITE)) {
        return;
    }

    msa_ptr->y_br_lite = msa_ptr->y;
    msa_ptr->x_br_lite = msa_ptr->x;
    if (los(msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y_br_lite, msa_ptr->x_br_lite)) {
        const Pos2D pos(msa_ptr->y_br_lite, msa_ptr->x_br_lite);
        const auto &terrain = player_ptr->current_floor_ptr->get_grid(pos).get_terrain();
        if (terrain.flags.has_not(TerrainCharacteristics::LOS) && terrain.flags.has(TerrainCharacteristics::PROJECT) && one_in_(2)) {
            msa_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
        }
    } else if (!adjacent_grid_check(
                   player_ptr, msa_ptr->m_ptr, &msa_ptr->y_br_lite, &msa_ptr->x_br_lite, TerrainCharacteristics::LOS,
                   [](PlayerType *, POSITION y1, POSITION x1, POSITION y2, POSITION x2) { return los(y1, x1, y2, x2); })) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::BR_LITE)) {
        return;
    }

    msa_ptr->y_br_lite = 0;
    msa_ptr->x_br_lite = 0;
}

static void feature_projection(const FloorType &floor, msa_type *msa_ptr)
{
    const Pos2D pos(msa_ptr->y, msa_ptr->x);
    const auto &terrain = floor.get_grid(pos).get_terrain();
    if (terrain.flags.has(TerrainCharacteristics::PROJECT)) {
        return;
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::BR_DISI) && terrain.flags.has(TerrainCharacteristics::HURT_DISI) && one_in_(2)) {
        msa_ptr->do_spell = DO_SPELL_BR_DISI;
        return;
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::BR_LITE) && terrain.flags.has(TerrainCharacteristics::LOS) && one_in_(2)) {
        msa_ptr->do_spell = DO_SPELL_BR_LITE;
    }
}

static void check_lite_area_by_mspell(PlayerType *player_ptr, msa_type *msa_ptr)
{
    const auto &system = AngbandSystem::get_instance();
    auto light_by_disintegration = msa_ptr->ability_flags.has(MonsterAbilityType::BR_DISI);
    light_by_disintegration &= msa_ptr->m_ptr->cdis < system.get_max_range() / 2;
    light_by_disintegration &= in_disintegration_range(player_ptr->current_floor_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x);
    light_by_disintegration &= one_in_(10) || (projectable(player_ptr, msa_ptr->y, msa_ptr->x, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx) && one_in_(2));
    if (light_by_disintegration) {
        msa_ptr->do_spell = DO_SPELL_BR_DISI;
        msa_ptr->success = true;
        return;
    }

    auto light_by_lite = msa_ptr->ability_flags.has(MonsterAbilityType::BR_LITE);
    light_by_lite &= msa_ptr->m_ptr->cdis < system.get_max_range() / 2;
    light_by_lite &= los(msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x);
    light_by_lite &= one_in_(5);
    if (light_by_lite) {
        msa_ptr->do_spell = DO_SPELL_BR_LITE;
        msa_ptr->success = true;
        return;
    }

    if (msa_ptr->ability_flags.has_not(MonsterAbilityType::BA_LITE) || (msa_ptr->m_ptr->cdis > system.get_max_range())) {
        return;
    }

    auto by = msa_ptr->y;
    auto bx = msa_ptr->x;
    get_project_point(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, &by, &bx, 0L);
    if ((distance(by, bx, msa_ptr->y, msa_ptr->x) <= 3) && los(by, bx, msa_ptr->y, msa_ptr->x) && one_in_(5)) {
        msa_ptr->do_spell = DO_SPELL_BA_LITE;
        msa_ptr->success = true;
    }
}

static void decide_lite_breath(msa_type *msa_ptr)
{
    if (msa_ptr->success) {
        return;
    }

    if (msa_ptr->m_ptr->target_y && msa_ptr->m_ptr->target_x) {
        msa_ptr->y = msa_ptr->m_ptr->target_y;
        msa_ptr->x = msa_ptr->m_ptr->target_x;
        msa_ptr->ability_flags &= RF_ABILITY_INDIRECT_MASK;
        msa_ptr->success = true;
    }

    auto should_set = msa_ptr->y_br_lite == 0;
    should_set |= msa_ptr->x_br_lite == 0;
    should_set |= msa_ptr->m_ptr->cdis > AngbandSystem::get_instance().get_max_range() / 2;
    should_set |= !one_in_(5);
    if (should_set) {
        return;
    }

    if (msa_ptr->success) {
        msa_ptr->ability_flags.set(MonsterAbilityType::BR_LITE);
        return;
    }

    msa_ptr->y = msa_ptr->y_br_lite;
    msa_ptr->x = msa_ptr->x_br_lite;
    msa_ptr->do_spell = DO_SPELL_BR_LITE;
    msa_ptr->success = true;
}

bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (projectable(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, msa_ptr->y, msa_ptr->x)) {
        feature_projection(*player_ptr->current_floor_ptr, msa_ptr);
        return true;
    }

    msa_ptr->success = false;
    check_lite_area_by_mspell(player_ptr, msa_ptr);
    if (!msa_ptr->success) {
        msa_ptr->success = adjacent_grid_check(player_ptr, msa_ptr->m_ptr, &msa_ptr->y, &msa_ptr->x, TerrainCharacteristics::PROJECT, projectable);
    }

    decide_lite_breath(msa_ptr);
    return msa_ptr->success;
}

void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->ability_flags.has_not(MonsterAbilityType::DARKNESS)) {
        return;
    }

    PlayerClass pc(player_ptr);
    auto can_use_lite_area = pc.equals(PlayerClassType::NINJA);
    can_use_lite_area &= msa_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD);
    can_use_lite_area &= msa_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_LITE);
    can_use_lite_area &= (msa_ptr->r_ptr->brightness_flags.has_none_of(dark_mask));

    if (msa_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (player_ptr->current_floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::DARKNESS);
        return;
    }

    if (pc.equals(PlayerClassType::NINJA) && !can_use_lite_area) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::DARKNESS);
    }
}
