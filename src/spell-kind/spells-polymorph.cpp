#include "spell-kind/spells-polymorph.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 変身処理向けにモンスターの近隣レベル帯モンスターを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monrace_id 基準となるモンスター種族ID
 * @return 変更先のモンスター種族ID
 */
static MonraceId select_polymorph_monrace_id(PlayerType *player_ptr, MonraceId monrace_id)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(monrace_id);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.misc_flags.has(MonsterMiscType::QUESTOR)) {
        return monrace_id;
    }

    const auto lev1 = monrace.level - ((randint1(20) / randint1(9)) + 1);
    const auto lev2 = monrace.level + ((randint1(20) / randint1(9)) + 1);
    for (auto i = 0; i < 1000; i++) {
        const auto new_monrace_id = get_mon_num(player_ptr, 0, (player_ptr->current_floor_ptr->dun_level + monrace.level) / 2 + 5, PM_NONE);
        if (!MonraceList::is_valid(new_monrace_id)) {
            break;
        }

        const auto &new_monrace = monraces.get_monrace(new_monrace_id);
        if (new_monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if ((new_monrace.level < lev1) || (new_monrace.level > lev2)) {
            continue;
        }

        return new_monrace_id;
    }

    return monrace_id;
}

/*!
 * @brief 指定座標にいるモンスターを変身させる /
 * Helper function -- return a "nearby" race for polymorphing
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 指定のY座標
 * @param x 指定のX座標
 * @return 実際に変身したらTRUEを返す
 */
bool polymorph_monster(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.grid_array[y][x];
    auto &monster = floor.m_list[grid.m_idx];
    MonraceId new_r_idx;
    MonraceId old_r_idx = monster.r_idx;
    const auto target_m_idx = Target::get_last_target().get_m_idx();
    const auto targeted = target_m_idx == grid.m_idx;
    auto health_tracked = HealthBarTracker::get_instance().is_tracking(grid.m_idx);

    if (floor.inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return false;
    }
    if (monster.is_riding() || monster.mflag2.has(MonsterConstantFlagType::KAGE)) {
        return false;
    }

    auto back_m = monster.clone();
    new_r_idx = select_polymorph_monrace_id(player_ptr, old_r_idx);
    if (new_r_idx == old_r_idx) {
        return false;
    }

    bool preserve_hold_objects = !back_m.hold_o_idx_list.empty();

    BIT_FLAGS mode = 0L;
    if (monster.is_friendly()) {
        mode |= PM_FORCE_FRIENDLY;
    }
    if (monster.is_pet()) {
        mode |= PM_FORCE_PET;
    }
    if (monster.mflag2.has(MonsterConstantFlagType::NOPET)) {
        mode |= PM_NO_PET;
    }

    monster.hold_o_idx_list.clear();
    delete_monster_idx(player_ptr, grid.m_idx);
    bool polymorphed = false;
    auto m_idx = place_specific_monster(player_ptr, y, x, new_r_idx, mode);
    if (m_idx) {
        auto &monster_polymorphed = floor.m_list[*m_idx];
        monster_polymorphed.nickname = back_m.nickname;
        monster_polymorphed.parent_m_idx = back_m.parent_m_idx;
        monster_polymorphed.hold_o_idx_list = back_m.hold_o_idx_list;
        polymorphed = true;
    } else {
        m_idx = place_specific_monster(player_ptr, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN));
        if (m_idx) {
            floor.m_list[*m_idx] = back_m.clone();
            floor.reset_mproc();
        } else {
            preserve_hold_objects = false;
        }
    }

    if (preserve_hold_objects) {
        for (const auto this_o_idx : back_m.hold_o_idx_list) {
            auto *o_ptr = floor.o_list[this_o_idx].get();
            o_ptr->held_m_idx = *m_idx;
        }
    } else {
        delete_items(player_ptr, back_m.hold_o_idx_list);
    }

    if (targeted) {
        if (m_idx) {
            Target::set_last_target(Target::create_monster_target(player_ptr, *m_idx));
        } else {
            Target::clear_last_target();
        }
    }
    if (health_tracked) {
        health_track(player_ptr, m_idx.value_or(0));
    }
    return polymorphed;
}
