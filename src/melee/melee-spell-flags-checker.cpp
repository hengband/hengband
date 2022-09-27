#include "melee/melee-spell-flags-checker.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "melee/melee-spell-util.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-util.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "spell-kind/spells-world.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
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
    ms_ptr->t_ptr = &player_ptr->current_floor_ptr->m_list[ms_ptr->target_idx];
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || !projectable(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx)) {
        ms_ptr->target_idx = 0;
    }
}

static void decide_indirection_melee_spell(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    const auto &m_ref = *ms_ptr->m_ptr;
    if ((ms_ptr->target_idx != 0) || (m_ref.target_y == 0)) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    ms_ptr->target_idx = floor_ptr->grid_array[m_ref.target_y][m_ref.target_x].m_idx;
    if (ms_ptr->target_idx == 0) {
        return;
    }

    ms_ptr->t_ptr = &floor_ptr->m_list[ms_ptr->target_idx];
    const auto &t_ref = *ms_ptr->t_ptr;
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || ((ms_ptr->target_idx != player_ptr->pet_t_m_idx) && !are_enemies(player_ptr, m_ref, t_ref))) {
        ms_ptr->target_idx = 0;
        return;
    }

    if (projectable(player_ptr, m_ref.fy, m_ref.fx, t_ref.fy, t_ref.fx)) {
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
    int plus = 1;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->phase_out) {
        start = randint1(floor_ptr->m_max - 1) + floor_ptr->m_max;
        if (randint0(2)) {
            plus = -1;
        }
    } else {
        start = floor_ptr->m_max + 1;
    }

    for (int i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus) {
        MONSTER_IDX dummy = (i % floor_ptr->m_max);
        if (!dummy) {
            continue;
        }

        ms_ptr->target_idx = dummy;
        ms_ptr->t_ptr = &floor_ptr->m_list[ms_ptr->target_idx];
        const auto &m_ref = *ms_ptr->m_ptr;
        const auto &t_ref = *ms_ptr->t_ptr;
        const auto is_enemies = are_enemies(player_ptr, m_ref, t_ref);
        const auto is_projectable = projectable(player_ptr, m_ref.fy, m_ref.fx, t_ref.fy, t_ref.fx);
        if (!t_ref.is_valid() || (ms_ptr->m_idx == ms_ptr->target_idx) || !is_enemies || !is_projectable) {
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

    bool vs_ninja = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) && !ms_ptr->t_ptr->is_hostile();
    bool can_use_lite_area = vs_ninja && ms_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD) && ms_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_LITE) && !(ms_ptr->r_ptr->flags7 & RF7_DARK_MASK);
    if (ms_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
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

static void check_arena(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (!player_ptr->current_floor_ptr->inside_arena && !player_ptr->phase_out) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK).reset(MonsterAbilityType::TELE_LEVEL);
    if (ms_ptr->m_ptr->r_idx == MonsterRaceId::ROLENTO) {
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

    POSITION real_y = ms_ptr->y;
    POSITION real_x = ms_ptr->x;
    get_project_point(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, &real_y, &real_x, 0L);
    if (!projectable(player_ptr, real_y, real_x, player_ptr->y, player_ptr->x) && ms_ptr->ability_flags.has(MonsterAbilityType::BA_LITE) && (distance(real_y, real_x, player_ptr->y, player_ptr->x) <= 4) && los(player_ptr, real_y, real_x, player_ptr->y, player_ptr->x)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BA_LITE);

        return;
    }

    int dist = distance(real_y, real_x, player_ptr->y, player_ptr->x);
    if (dist <= 2) {
        ms_ptr->ability_flags.reset(ball_mask_except_rocket);
        return;
    }

    if (dist > 4) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_BIG_BALL_MASK);
}

static void check_melee_spell_rocket(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_not(MonsterAbilityType::ROCKET)) {
        return;
    }

    POSITION real_y = ms_ptr->y;
    POSITION real_x = ms_ptr->x;
    get_project_point(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, &real_y, &real_x, PROJECT_STOP);
    if (projectable(player_ptr, real_y, real_x, player_ptr->y, player_ptr->x) && (distance(real_y, real_x, player_ptr->y, player_ptr->x) <= 2)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::ROCKET);
    }
}

static void check_melee_spell_beam(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_none_of(RF_ABILITY_BEAM_MASK) || direct_beam(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, ms_ptr->m_ptr)) {
        return;
    }

    ms_ptr->ability_flags.reset(RF_ABILITY_BEAM_MASK);
}

static void check_melee_spell_breath(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_none_of(RF_ABILITY_BREATH_MASK)) {
        return;
    }

    POSITION rad = (ms_ptr->r_ptr->flags2 & RF2_POWERFUL) ? 3 : 2;
    if (!breath_direct(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, AttributeType::NONE, true)) {
        ms_ptr->ability_flags.reset(RF_ABILITY_BREATH_MASK);
        return;
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_LITE) && !breath_direct(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx,
                                                                      ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, AttributeType::LITE, true)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
        return;
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_DISI) && !breath_direct(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx,
                                                                      ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, AttributeType::DISINTEGRATE, true)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_DISI);
    }
}

static void check_melee_spell_special(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->ability_flags.has_not(MonsterAbilityType::SPECIAL)) {
        return;
    }

    if (ms_ptr->m_ptr->r_idx == MonsterRaceId::ROLENTO) {
        if ((player_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) {
            ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
        }

        return;
    }

    if (ms_ptr->r_ptr->d_char == 'B') {
        if ((player_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT)) {
            ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
        }

        return;
    }

    ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
}

static void check_riding(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->m_idx != player_ptr->riding) {
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

    if (!(player_ptr->pet_extra_flags & PF_BALL_SPELL) && (ms_ptr->m_idx != player_ptr->riding)) {
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

    if (ms_ptr->ability_flags.has(MonsterAbilityType::RAISE_DEAD) && !raise_possible(player_ptr, ms_ptr->m_ptr)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::RAISE_DEAD);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::SPECIAL) && (ms_ptr->m_ptr->r_idx == MonsterRaceId::ROLENTO) && !summon_possible(player_ptr, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }
}

static void check_smart(PlayerType *player_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->r_ptr->behavior_flags.has_not(MonsterBehaviorType::SMART)) {
        return;
    }

    if ((ms_ptr->m_ptr->hp < ms_ptr->m_ptr->maxhp / 10) && (randint0(100) < 50)) {
        ms_ptr->ability_flags &= RF_ABILITY_INT_MASK;
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::TELE_LEVEL) && is_teleport_level_ineffective(player_ptr, (ms_ptr->target_idx == player_ptr->riding) ? 0 : ms_ptr->target_idx)) {
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
    reset_target(ms_ptr->m_ptr);
    ms_ptr->ability_flags.reset({ MonsterAbilityType::WORLD, MonsterAbilityType::TRAPS, MonsterAbilityType::FORGET });
    if (ms_ptr->ability_flags.has(MonsterAbilityType::BR_LITE) && !los(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
    }

    if (ms_ptr->ability_flags.has(MonsterAbilityType::SPECIAL) && (ms_ptr->m_ptr->r_idx != MonsterRaceId::ROLENTO) && (ms_ptr->r_ptr->d_char != 'B')) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }

    check_darkness(player_ptr, ms_ptr);
    check_stupid(ms_ptr);
    check_arena(player_ptr, ms_ptr);
    if (player_ptr->phase_out && !one_in_(3)) {
        ms_ptr->ability_flags.reset(MonsterAbilityType::HEAL);
    }

    check_riding(player_ptr, ms_ptr);
    check_pet(player_ptr, ms_ptr);
    check_non_stupid(player_ptr, ms_ptr);
    check_smart(player_ptr, ms_ptr);
    return set_melee_spell_set(player_ptr, ms_ptr);
}
