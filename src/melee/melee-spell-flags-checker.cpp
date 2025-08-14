#include "melee/melee-spell-flags-checker.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/effect-characteristics.h"
#include "floor/line-of-sight.h"
#include "melee/melee-spell-util.h"
#include "monster-floor/monster-move.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-status.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-util.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "spell-kind/spells-world.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include <iterator>

static void decide_melee_spell_target(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if ((player_ptr->pet_t_m_idx == 0) || !ms_ptr->pet) {
        return;
    }

    ms_ptr->target_idx = player_ptr->pet_t_m_idx;
    const auto &floor = *player_ptr->current_floor_ptr;
    ms_ptr->t_ptr = &floor.m_list[ms_ptr->target_idx];
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || !projectable(floor, ms_ptr->m_ptr->get_position(), ms_ptr->t_ptr->get_position())) {
        ms_ptr->target_idx = 0;
    }
}

static void decide_indirection_melee_spell(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    const auto &monster_from = *ms_ptr->m_ptr;
    if ((ms_ptr->target_idx != 0) || (monster_from.target_y == 0)) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    ms_ptr->target_idx = floor.grid_array[monster_from.target_y][monster_from.target_x].m_idx;
    if (ms_ptr->target_idx == 0) {
        return;
    }

    ms_ptr->t_ptr = &floor.m_list[ms_ptr->target_idx];
    const auto &monster_to = *ms_ptr->t_ptr;
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || ((ms_ptr->target_idx != player_ptr->pet_t_m_idx) && !monster_from.is_hostile_to_melee(monster_to))) {
        ms_ptr->target_idx = 0;
        return;
    }

    if (projectable(floor, monster_from.get_position(), monster_to.get_position())) {
        return;
    }

    ms_ptr->ability_flags &= RF_ABILITY_INDIRECT_MASK;
}

static bool check_melee_spell_projection(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->target_idx != 0) {
        return true;
    }

    int start;
    auto plus = 1;
    const auto &floor = *player_ptr->current_floor_ptr;
    if (AngbandSystem::get_instance().is_phase_out()) {
        start = randint1(floor.m_max - 1) + floor.m_max;
        if (randint0(2)) {
            plus = -1;
        }
    } else {
        start = floor.m_max + 1;
    }

    for (int i = start; ((i < start + floor.m_max) && (i > start - floor.m_max)); i += plus) {
        short dummy = i % floor.m_max;
        if (dummy == 0) {
            continue;
        }

        ms_ptr->target_idx = dummy;
        ms_ptr->t_ptr = &floor.m_list[ms_ptr->target_idx];
        const auto &monster_from = *ms_ptr->m_ptr;
        const auto &monster_to = *ms_ptr->t_ptr;
        const auto is_enemies = monster_from.is_hostile_to_melee(monster_to);
        const auto is_projectable = projectable(floor, monster_from.get_position(), monster_to.get_position());
        if (!monster_to.is_valid() || (ms_ptr->m_idx == ms_ptr->target_idx) || !is_enemies || !is_projectable) {
            continue;
        }

        return true;
    }

    return false;
}

static void check_darkness(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_not(MonsterAbilityType::DARKNESS)) {
        return;
    }

    const auto vs_ninja = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) && !ms_ptr->t_ptr->is_hostile();
    auto can_use_lite_area = vs_ninja && ms_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD);
    can_use_lite_area &= ms_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_LITE);
    can_use_lite_area &= ms_ptr->r_ptr->brightness_flags.has_none_of(dark_mask);
    if (ms_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (player_ptr->current_floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::DARKNESS);
        return;
    }

    if (vs_ninja && !can_use_lite_area) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::DARKNESS);
    }
}

static void check_stupid(melee_spell_type *ms_ptr)
{
    if (!ms_ptr->in_no_magic_dungeon || ms_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    ms_ptr->ability_flags &= RF_ABILITY_NOMAGIC_MASK;
}

static void check_arena(const FloorType &floor, melee_spell_type *ms_ptr)
{
    if (!floor.inside_arena && !AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK).reset(MonsterAbilityType::TELE_LEVEL);
    if (ms_ptr->m_ptr->r_idx == MonraceId::ROLENTO) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }
}

static void check_melee_spell_distance(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    auto ball_mask_except_rocket = RF_ABILITY_BALL_MASK;
    ball_mask_except_rocket.reset(MonsterAbilityType::ROCKET);
    if (ms_ptr->ability_flags.has_none_of(ball_mask_except_rocket)) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = ms_ptr->m_ptr->get_position();
    const auto pos_real = get_project_point(floor, p_pos, m_pos, ms_ptr->get_position(), 0L);
    auto should_preserve = !projectable(floor, pos_real, p_pos);
    should_preserve &= ms_ptr->ability_flags.has(MonsterAbilityType::BA_LITE);
    should_preserve &= Grid::calc_distance(pos_real, p_pos) <= 4;
    should_preserve &= los(*player_ptr->current_floor_ptr, pos_real, p_pos);
    if (should_preserve) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BA_LITE);
        return;
    }

    const auto dist = Grid::calc_distance(pos_real, p_pos);
    if (dist <= 2) {
        ms_ptr->ability_flags.reset(ball_mask_except_rocket);
        return;
    }

    if (dist > 4) {
        return;
    }

    const auto ball_when_powerful_rad4 = {
        MonsterAbilityType::BA_ACID,
        MonsterAbilityType::BA_ELEC,
        MonsterAbilityType::BA_FIRE,
        MonsterAbilityType::BA_COLD
    };
    const auto &monrace = ms_ptr->m_ptr->get_monrace();
    if (monrace.misc_flags.has(MonsterMiscType::POWERFUL)) {
        ms_ptr->ability_flags.reset(ball_when_powerful_rad4);
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_BIG_BALL_MASK);
}

static void check_melee_spell_rocket(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_not(MonsterAbilityType::ROCKET)) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto m_pos = ms_ptr->m_ptr->get_position();
    const auto pos_real = get_project_point(floor, p_pos, m_pos, ms_ptr->get_position(), PROJECT_STOP);
    if (projectable(floor, pos_real, p_pos) && (Grid::calc_distance(pos_real, p_pos) <= 2)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::ROCKET);
    }
}

static void check_melee_spell_beam(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_none_of(RF_ABILITY_BEAM_MASK) || direct_beam(player_ptr, *ms_ptr->m_ptr, ms_ptr->t_ptr->get_position())) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_BEAM_MASK);
}

static void check_melee_spell_breath(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_none_of(RF_ABILITY_BREATH_MASK)) {
        return;
    }

    POSITION rad = ms_ptr->r_ptr->misc_flags.has(MonsterMiscType::POWERFUL) ? 3 : 2;
    if (!breath_direct(player_ptr, ms_ptr->m_ptr->get_position(), ms_ptr->t_ptr->get_position(), rad, AttributeType::NONE, true)) {
        ms_ptr->ability_flags.reset(RF_ABILITY_BREATH_MASK);
        return;
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_LITE) && !breath_direct(player_ptr, ms_ptr->m_ptr->get_position(),
                                                                      ms_ptr->t_ptr->get_position(), rad, AttributeType::LITE, true)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
        return;
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_DISI) && !breath_direct(player_ptr, ms_ptr->m_ptr->get_position(),
                                                                      ms_ptr->t_ptr->get_position(), rad, AttributeType::DISINTEGRATE, true)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_DISI);
    }
}

static void check_melee_spell_special(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_not(MonsterAbilityType::SPECIAL)) {
        return;
    }

    if (ms_ptr->m_ptr->r_idx == MonraceId::ROLENTO) {
        if ((player_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) {
            ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
        }

        return;
    }

    if (ms_ptr->r_ptr->symbol_char_is_any_of("B")) {
        if ((player_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT)) {
            ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
        }

        return;
    }

    ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
}

static void check_riding(melee_spell_type *ms_ptr)
{
    if (!ms_ptr->m_ptr->is_riding()) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_RIDING_MASK);
}

static void check_pet(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (!ms_ptr->pet) {
        return;
    }

    ms_ptr->ability_flags.reset({ MonsterAbilityType::SHRIEK, MonsterAbilityType::DARKNESS, MonsterAbilityType::TRAPS });
    if (!(player_ptr->pet_extra_flags & PF_TELEPORT)) {
        ms_ptr->ability_flags.reset({ MonsterAbilityType::BLINK, MonsterAbilityType::TPORT, MonsterAbilityType::TELE_TO, MonsterAbilityType::TELE_AWAY, MonsterAbilityType::TELE_LEVEL });
    }

    if (!(player_ptr->pet_extra_flags & PF_ATTACK_SPELL)) {
        ms_ptr->ability_flags.reset(RF_ABILITY_ATTACK_MASK);
    }

    if (!(player_ptr->pet_extra_flags & PF_SUMMON_SPELL)) {
        ms_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK);
    }

    if (!(player_ptr->pet_extra_flags & PF_BALL_SPELL) && !ms_ptr->m_ptr->is_riding()) {
        check_melee_spell_distance(player_ptr, ms_ptr);
        check_melee_spell_rocket(player_ptr, ms_ptr);
        check_melee_spell_beam(player_ptr, ms_ptr);
        check_melee_spell_breath(player_ptr, ms_ptr);
    }

    check_melee_spell_special(player_ptr, ms_ptr);
}

static void check_non_stupid(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (ms_ptr->ability_flags.has_any_of(RF_ABILITY_BOLT_MASK) && !clean_shot(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, ms_ptr->pet)) {
        ms_ptr->ability_flags.reset(RF_ABILITY_BOLT_MASK);
    }

    if (ms_ptr->ability_flags.has_any_of(RF_ABILITY_SUMMON_MASK) && !(summon_possible(player_ptr, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))) {
        ms_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::DISPEL) && !dispel_check_monster(player_ptr, ms_ptr->m_idx, ms_ptr->target_idx)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::DISPEL);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::RAISE_DEAD) && !raise_possible(player_ptr, *ms_ptr->m_ptr)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::RAISE_DEAD);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::SPECIAL) && (ms_ptr->m_ptr->r_idx == MonraceId::ROLENTO) && !summon_possible(player_ptr, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }
}

static void check_smart(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->r_ptr->behavior_flags.has_not(MonsterBehaviorType::SMART)) {
        return;
    }

    if ((ms_ptr->m_ptr->hp < ms_ptr->m_ptr->maxhp / 10) && one_in_(2)) {
        ms_ptr->ability_flags &= RF_ABILITY_INT_MASK;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    if (ms_ptr->ability_flags.has(MonsterAbilityType::TELE_LEVEL) && floor.can_teleport_level(!ms_ptr->t_ptr->is_riding() ? ms_ptr->target_idx != 0 : false)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::TELE_LEVEL);
    }
}

static bool set_melee_spell_set(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.none()) {
        return false;
    }

    EnumClassFlagGroup<MonsterAbilityType>::get_flags(ms_ptr->ability_flags, std::back_inserter(ms_ptr->spells));

    return !ms_ptr->spells.empty() && player_ptr->playing && !player_ptr->is_dead && !player_ptr->leaving;
}

bool check_melee_spell_set(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->m_ptr->is_confused()) {
        return false;
    }

    ms_ptr->ability_flags = ms_ptr->r_ptr->ability_flags;
    decide_melee_spell_target(player_ptr, ms_ptr);
    decide_indirection_melee_spell(player_ptr, ms_ptr);
    if (!check_melee_spell_projection(player_ptr, ms_ptr)) {
        return false;
    }

    ms_ptr->y = ms_ptr->t_ptr->fy;
    ms_ptr->x = ms_ptr->t_ptr->fx;
    ms_ptr->m_ptr->reset_target();
    ms_ptr->ability_flags.reset({ MonsterAbilityType::WORLD, MonsterAbilityType::TRAPS, MonsterAbilityType::FORGET });
    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_LITE) && !los(*player_ptr->current_floor_ptr, ms_ptr->m_ptr->get_position(), ms_ptr->t_ptr->get_position())) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::SPECIAL) && (ms_ptr->m_ptr->r_idx != MonraceId::ROLENTO) && !ms_ptr->r_ptr->symbol_char_is_any_of("B")) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }

    check_darkness(player_ptr, ms_ptr);
    check_stupid(ms_ptr);
    check_arena(*player_ptr->current_floor_ptr, ms_ptr);
    if (AngbandSystem::get_instance().is_phase_out() && !one_in_(3)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::HEAL);
    }

    check_riding(ms_ptr);
    check_pet(player_ptr, ms_ptr);
    check_non_stupid(player_ptr, ms_ptr);
    check_smart(player_ptr, ms_ptr);
    return set_melee_spell_set(player_ptr, ms_ptr);
}
