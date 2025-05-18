/*!
 * @brief モンスターの魔法によってフロアを明るくする処理及びその判定
 * @date 2020/07/23
 * @author Hourier
 */

#include "mspell/mspell-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/line-of-sight.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "player-base/player-class.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include <optional>

/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster 技能を使用するモンスターへの参照
 * @param pos 最適な目標地点座標の座標
 * @param tc 射線に入れるのを避ける地形の所持フラグ
 * @param checker 射線判定の振り分け
 * @return 有効な座標があった場合はその座標、なかったらnullopt
 */
static std::optional<Pos2D> adjacent_grid_check(PlayerType *player_ptr, const MonsterEntity &monster, const Pos2D &pos, TerrainCharacteristics tc)
{
    constexpr std::array<std::array<int, 8>, 4> directions = {
        {
            { 7, 8, 9, 4, 6, 1, 2, 3 },
            { 9, 8, 7, 6, 4, 3, 2, 1 },
            { 1, 2, 3, 4, 6, 7, 8, 9 },
            { 3, 2, 1, 6, 4, 9, 8, 7 },
        }
    };

    int next;
    if (monster.fy < player_ptr->y && monster.fx < player_ptr->x) {
        next = 0;
    } else if (monster.fy < player_ptr->y) {
        next = 1;
    } else if (monster.fx < player_ptr->x) {
        next = 2;
    } else {
        next = 3;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = monster.get_position();
    for (const auto direction : directions.at(next)) {
        const auto pos_next = pos + Direction(direction).vec();
        if (!floor.has_terrain_characteristics(pos_next, tc)) {
            continue;
        }

        bool check_result;
        switch (tc) {
        case TerrainCharacteristics::PROJECTION:
            check_result = projectable(floor, p_pos, m_pos, pos_next);
            break;
        case TerrainCharacteristics::LOS:
            check_result = los(floor, m_pos, pos_next);
            break;
        default:
            THROW_EXCEPTION(std::logic_error, format("Invalid PathChecker is specified! %d", enum2i(tc)));
        }

        if (check_result) {
            return pos_next;
        }
    }

    return std::nullopt;
}

void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->ability_flags.has_not(MonsterAbilityType::BR_LITE)) {
        return;
    }

    msa_ptr->y_br_lite = msa_ptr->y;
    msa_ptr->x_br_lite = msa_ptr->x;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto pos_lite = msa_ptr->get_position_lite();
    if (los(floor, msa_ptr->m_ptr->get_position(), pos_lite)) {
        const auto &terrain = floor.get_grid(pos_lite).get_terrain();
        if (terrain.flags.has_not(TerrainCharacteristics::LOS) && terrain.flags.has(TerrainCharacteristics::PROJECTION) && one_in_(2)) {
            msa_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
        }
    } else {
        const auto pos = adjacent_grid_check(player_ptr, *msa_ptr->m_ptr, pos_lite, TerrainCharacteristics::LOS);
        if (pos) {
            msa_ptr->set_position_lite(*pos);
        } else {
            msa_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
        }
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
    if (terrain.flags.has(TerrainCharacteristics::PROJECTION)) {
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
    const auto pos = msa_ptr->get_position();
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = msa_ptr->m_ptr->get_position();
    const auto &floor = *player_ptr->current_floor_ptr;
    light_by_disintegration &= in_disintegration_range(floor, m_pos, pos);
    light_by_disintegration &= one_in_(10) || (projectable(floor, p_pos, pos, m_pos) && one_in_(2));
    if (light_by_disintegration) {
        msa_ptr->do_spell = DO_SPELL_BR_DISI;
        msa_ptr->success = true;
        return;
    }

    auto light_by_lite = msa_ptr->ability_flags.has(MonsterAbilityType::BR_LITE);
    light_by_lite &= msa_ptr->m_ptr->cdis < system.get_max_range() / 2;
    light_by_lite &= los(floor, m_pos, pos);
    light_by_lite &= one_in_(5);
    if (light_by_lite) {
        msa_ptr->do_spell = DO_SPELL_BR_LITE;
        msa_ptr->success = true;
        return;
    }

    if (msa_ptr->ability_flags.has_not(MonsterAbilityType::BA_LITE) || (msa_ptr->m_ptr->cdis > system.get_max_range())) {
        return;
    }

    const auto pos_breath = get_project_point(floor, p_pos, m_pos, msa_ptr->get_position(), 0);
    if ((Grid::calc_distance(pos_breath, pos) <= 3) && los(floor, pos_breath, pos) && one_in_(5)) {
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
    const auto &floor = *player_ptr->current_floor_ptr;
    if (projectable(floor, player_ptr->get_position(), msa_ptr->m_ptr->get_position(), msa_ptr->get_position())) {
        feature_projection(floor, msa_ptr);
        return true;
    }

    msa_ptr->success = false;
    check_lite_area_by_mspell(player_ptr, msa_ptr);
    if (!msa_ptr->success) {
        const auto pos = adjacent_grid_check(player_ptr, *msa_ptr->m_ptr, msa_ptr->get_position(), TerrainCharacteristics::PROJECTION);
        msa_ptr->success = pos.has_value();
        if (pos) {
            msa_ptr->set_position(*pos);
        }
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
