#include "melee/melee-spell-flags-checker.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "floor/line-of-sight.h"
#include "melee/melee-spell-util.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-util.h"
#include "pet/pet-util.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"

static void decide_melee_spell_target(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((target_ptr->pet_t_m_idx == 0) || !ms_ptr->pet)
        return;

    ms_ptr->target_idx = target_ptr->pet_t_m_idx;
    ms_ptr->t_ptr = &target_ptr->current_floor_ptr->m_list[ms_ptr->target_idx];
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || !projectable(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))
        ms_ptr->target_idx = 0;
}

static void decide_indirection_melee_spell(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->target_idx != 0) || (ms_ptr->m_ptr->target_y == 0))
        return;

    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    ms_ptr->target_idx = floor_ptr->grid_array[ms_ptr->m_ptr->target_y][ms_ptr->m_ptr->target_x].m_idx;
    if (ms_ptr->target_idx == 0)
        return;

    ms_ptr->t_ptr = &floor_ptr->m_list[ms_ptr->target_idx];
    if ((ms_ptr->m_idx == ms_ptr->target_idx) || ((ms_ptr->target_idx != target_ptr->pet_t_m_idx) && !are_enemies(target_ptr, ms_ptr->m_ptr, ms_ptr->t_ptr))) {
        ms_ptr->target_idx = 0;
        return;
    }

    if (projectable(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))
        return;

    ms_ptr->f4 &= RF4_INDIRECT_MASK;
    ms_ptr->f5 &= RF5_INDIRECT_MASK;
    ms_ptr->f6 &= RF6_INDIRECT_MASK;
}

static bool check_melee_spell_projection(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->target_idx != 0)
        return TRUE;

    int start;
    int plus = 1;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    if (target_ptr->phase_out) {
        start = randint1(floor_ptr->m_max - 1) + floor_ptr->m_max;
        if (randint0(2))
            plus = -1;
    } else
        start = floor_ptr->m_max + 1;

    for (int i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus) {
        MONSTER_IDX dummy = (i % floor_ptr->m_max);
        if (!dummy)
            continue;

        ms_ptr->target_idx = dummy;
        ms_ptr->t_ptr = &floor_ptr->m_list[ms_ptr->target_idx];
        if (!monster_is_valid(ms_ptr->t_ptr) || (ms_ptr->m_idx == ms_ptr->target_idx) || !are_enemies(target_ptr, ms_ptr->m_ptr, ms_ptr->t_ptr)
            || !projectable(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))
            continue;

        return TRUE;
    }

    return FALSE;
}

static void check_darkness(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->f6 & RF6_DARKNESS) == 0)
        return;

    bool vs_ninja = (target_ptr->pclass == CLASS_NINJA) && !is_hostile(ms_ptr->t_ptr);
    bool can_use_lite_area = vs_ninja && !(ms_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) && !(ms_ptr->r_ptr->flags7 & RF7_DARK_MASK);
    if ((ms_ptr->r_ptr->flags2 & RF2_STUPID) != 0)
        return;

    if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS) {
        ms_ptr->f6 &= ~(RF6_DARKNESS);
        return;
    }

    if (vs_ninja && !can_use_lite_area)
        ms_ptr->f6 &= ~(RF6_DARKNESS);
}

static void check_stupid(melee_spell_type *ms_ptr)
{
    if (!ms_ptr->in_no_magic_dungeon || ((ms_ptr->r_ptr->flags2 & RF2_STUPID) != 0))
        return;

    ms_ptr->f4 &= (RF4_NOMAGIC_MASK);
    ms_ptr->f5 &= (RF5_NOMAGIC_MASK);
    ms_ptr->f6 &= (RF6_NOMAGIC_MASK);
}

static void check_arena(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (!target_ptr->current_floor_ptr->inside_arena && !target_ptr->phase_out)
        return;

    ms_ptr->f4 &= ~(RF4_SUMMON_MASK);
    ms_ptr->f5 &= ~(RF5_SUMMON_MASK);
    ms_ptr->f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);
    if (ms_ptr->m_ptr->r_idx == MON_ROLENTO)
        ms_ptr->f6 &= ~(RF6_SPECIAL);
}

static void check_melee_spell_distance(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (((ms_ptr->f4 & (RF4_BALL_MASK & ~(RF4_ROCKET))) == 0) && ((ms_ptr->f5 & RF5_BALL_MASK) == 0) && ((ms_ptr->f6 & RF6_BALL_MASK) == 0))
        return;

    POSITION real_y = ms_ptr->y;
    POSITION real_x = ms_ptr->x;
    get_project_point(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, &real_y, &real_x, 0L);
    if (!projectable(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x) && ((ms_ptr->f5 & RF5_BA_LITE) != 0)
        && (distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 4) && los(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x)) {
        ms_ptr->f5 &= ~(RF5_BA_LITE);

        return;
    }

    int dist = distance(real_y, real_x, target_ptr->y, target_ptr->x);
    if (dist <= 2) {
        ms_ptr->f4 &= ~(RF4_BALL_MASK & ~(RF4_ROCKET));
        ms_ptr->f5 &= ~(RF5_BALL_MASK);
        ms_ptr->f6 &= ~(RF6_BALL_MASK);
        return;
    }

    if (dist > 4)
        return;

    ms_ptr->f4 &= ~(RF4_BIG_BALL_MASK);
    ms_ptr->f5 &= ~(RF5_BIG_BALL_MASK);
    ms_ptr->f6 &= ~(RF6_BIG_BALL_MASK);
}

static void check_melee_spell_rocket(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->f4 & RF4_ROCKET) == 0)
        return;

    POSITION real_y = ms_ptr->y;
    POSITION real_x = ms_ptr->x;
    get_project_point(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, &real_y, &real_x, PROJECT_STOP);
    if (projectable(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x) && (distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 2))
        ms_ptr->f4 &= ~(RF4_ROCKET);
}

static void check_melee_spell_beam(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((((ms_ptr->f4 & RF4_BEAM_MASK) == 0) && ((ms_ptr->f5 & RF5_BEAM_MASK) == 0) && ((ms_ptr->f6 & RF6_BEAM_MASK) == 0))
        || direct_beam(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, ms_ptr->m_ptr))
        return;

    ms_ptr->f4 &= ~(RF4_BEAM_MASK);
    ms_ptr->f5 &= ~(RF5_BEAM_MASK);
    ms_ptr->f6 &= ~(RF6_BEAM_MASK);
}

static void check_melee_spell_breath(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (((ms_ptr->f4 & RF4_BREATH_MASK) == 0) && ((ms_ptr->f5 & RF5_BREATH_MASK) == 0) && ((ms_ptr->f6 & RF6_BREATH_MASK) == 0))
        return;

    POSITION rad = (ms_ptr->r_ptr->flags2 & RF2_POWERFUL) ? 3 : 2;
    if (!breath_direct(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, 0, TRUE)) {
        ms_ptr->f4 &= ~(RF4_BREATH_MASK);
        ms_ptr->f5 &= ~(RF5_BREATH_MASK);
        ms_ptr->f6 &= ~(RF6_BREATH_MASK);
        return;
    }

    if ((ms_ptr->f4 & RF4_BR_LITE)
        && !breath_direct(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, GF_LITE, TRUE)) {
        ms_ptr->f4 &= ~(RF4_BR_LITE);
        return;
    }

    if ((ms_ptr->f4 & RF4_BR_DISI)
        && !breath_direct(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, rad, GF_DISINTEGRATE, TRUE)) {
        ms_ptr->f4 &= ~(RF4_BR_DISI);
    }
}

static void check_melee_spell_special(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->f6 & RF6_SPECIAL) == 0)
        return;

    if (ms_ptr->m_ptr->r_idx == MON_ROLENTO) {
        if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL))
            ms_ptr->f6 &= ~(RF6_SPECIAL);

        return;
    }

    if (ms_ptr->r_ptr->d_char == 'B') {
        if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT))
            ms_ptr->f6 &= ~(RF6_SPECIAL);

        return;
    }

    ms_ptr->f6 &= ~(RF6_SPECIAL);
}

static void check_riding(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (ms_ptr->m_idx != target_ptr->riding)
        return;

    ms_ptr->f4 &= ~(RF4_RIDING_MASK);
    ms_ptr->f5 &= ~(RF5_RIDING_MASK);
    ms_ptr->f6 &= ~(RF6_RIDING_MASK);
}

static void check_pet(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (!ms_ptr->pet)
        return;

    ms_ptr->f4 &= ~(RF4_SHRIEK);
    ms_ptr->f6 &= ~(RF6_DARKNESS | RF6_TRAPS);
    if (!(target_ptr->pet_extra_flags & PF_TELEPORT))
        ms_ptr->f6 &= ~(RF6_BLINK | RF6_TPORT | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL);

    if (!(target_ptr->pet_extra_flags & PF_ATTACK_SPELL)) {
        ms_ptr->f4 &= ~(RF4_ATTACK_MASK);
        ms_ptr->f5 &= ~(RF5_ATTACK_MASK);
        ms_ptr->f6 &= ~(RF6_ATTACK_MASK);
    }

    if (!(target_ptr->pet_extra_flags & PF_SUMMON_SPELL)) {
        ms_ptr->f4 &= ~(RF4_SUMMON_MASK);
        ms_ptr->f5 &= ~(RF5_SUMMON_MASK);
        ms_ptr->f6 &= ~(RF6_SUMMON_MASK);
    }

    if (!(target_ptr->pet_extra_flags & PF_BALL_SPELL) && (ms_ptr->m_idx != target_ptr->riding)) {
        check_melee_spell_distance(target_ptr, ms_ptr);
        check_melee_spell_rocket(target_ptr, ms_ptr);
        check_melee_spell_beam(target_ptr, ms_ptr);
        check_melee_spell_breath(target_ptr, ms_ptr);
    }

    check_melee_spell_special(target_ptr, ms_ptr);
}

static void check_non_stupid(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->r_ptr->flags2 & RF2_STUPID) != 0)
        return;

    if (((ms_ptr->f4 & RF4_BOLT_MASK) || (ms_ptr->f5 & RF5_BOLT_MASK) || (ms_ptr->f6 & RF6_BOLT_MASK))
        && !clean_shot(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx, ms_ptr->pet)) {
        ms_ptr->f4 &= ~(RF4_BOLT_MASK);
        ms_ptr->f5 &= ~(RF5_BOLT_MASK);
        ms_ptr->f6 &= ~(RF6_BOLT_MASK);
    }

    if (((ms_ptr->f4 & RF4_SUMMON_MASK) || (ms_ptr->f5 & RF5_SUMMON_MASK) || (ms_ptr->f6 & RF6_SUMMON_MASK))
        && !(summon_possible(target_ptr, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))) {
        ms_ptr->f4 &= ~(RF4_SUMMON_MASK);
        ms_ptr->f5 &= ~(RF5_SUMMON_MASK);
        ms_ptr->f6 &= ~(RF6_SUMMON_MASK);
    }

    if ((ms_ptr->f4 & RF4_DISPEL) && !dispel_check_monster(target_ptr, ms_ptr->m_idx, ms_ptr->target_idx))
        ms_ptr->f4 &= ~(RF4_DISPEL);

    if ((ms_ptr->f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, ms_ptr->m_ptr))
        ms_ptr->f6 &= ~(RF6_RAISE_DEAD);

    if (((ms_ptr->f6 & RF6_SPECIAL) != 0) && (ms_ptr->m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))
        ms_ptr->f6 &= ~(RF6_SPECIAL);
}

static void check_smart(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if ((ms_ptr->r_ptr->flags2 & RF2_SMART) == 0)
        return;

    if ((ms_ptr->m_ptr->hp < ms_ptr->m_ptr->maxhp / 10) && (randint0(100) < 50)) {
        ms_ptr->f4 &= (RF4_INT_MASK);
        ms_ptr->f5 &= (RF5_INT_MASK);
        ms_ptr->f6 &= (RF6_INT_MASK);
    }

    if ((ms_ptr->f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, (ms_ptr->target_idx == target_ptr->riding) ? 0 : ms_ptr->target_idx))
        ms_ptr->f6 &= ~(RF6_TELE_LEVEL);
}

static bool set_melee_spell_set(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (!ms_ptr->f4 && !ms_ptr->f5 && !ms_ptr->f6)
        return FALSE;

    for (int k = 0; k < 32; k++)
        if (ms_ptr->f4 & (1L << k))
            ms_ptr->spell[ms_ptr->num++] = k + RF4_SPELL_START;

    for (int k = 0; k < 32; k++)
        if (ms_ptr->f5 & (1L << k))
            ms_ptr->spell[ms_ptr->num++] = k + RF5_SPELL_START;

    for (int k = 0; k < 32; k++)
        if (ms_ptr->f6 & (1L << k))
            ms_ptr->spell[ms_ptr->num++] = k + RF6_SPELL_START;

    return (ms_ptr->num != 0) && target_ptr->playing && !target_ptr->is_dead && !target_ptr->leaving;
}

bool check_melee_spell_set(player_type *target_ptr, melee_spell_type *ms_ptr)
{
    if (monster_confused_remaining(ms_ptr->m_ptr))
        return FALSE;

    ms_ptr->f4 = ms_ptr->r_ptr->flags4;
    ms_ptr->f5 = ms_ptr->r_ptr->a_ability_flags1;
    ms_ptr->f6 = ms_ptr->r_ptr->a_ability_flags2;
    decide_melee_spell_target(target_ptr, ms_ptr);
    decide_indirection_melee_spell(target_ptr, ms_ptr);
    if (!check_melee_spell_projection(target_ptr, ms_ptr))
        return FALSE;

    ms_ptr->y = ms_ptr->t_ptr->fy;
    ms_ptr->x = ms_ptr->t_ptr->fx;
    reset_target(ms_ptr->m_ptr);
    ms_ptr->f6 &= ~(RF6_WORLD | RF6_TRAPS | RF6_FORGET);
    if (((ms_ptr->f4 & RF4_BR_LITE) != 0) && !los(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx, ms_ptr->t_ptr->fy, ms_ptr->t_ptr->fx))
        ms_ptr->f4 &= ~(RF4_BR_LITE);

    if (((ms_ptr->f6 & RF6_SPECIAL) != 0) && (ms_ptr->m_ptr->r_idx != MON_ROLENTO) && (ms_ptr->r_ptr->d_char != 'B'))
        ms_ptr->f6 &= ~(RF6_SPECIAL);

    check_darkness(target_ptr, ms_ptr);
    check_stupid(ms_ptr);
    check_arena(target_ptr, ms_ptr);
    if (target_ptr->phase_out && !one_in_(3))
        ms_ptr->f6 &= ~(RF6_HEAL);

    check_riding(target_ptr, ms_ptr);
    check_pet(target_ptr, ms_ptr);
    check_non_stupid(target_ptr, ms_ptr);
    check_smart(target_ptr, ms_ptr);
    return set_melee_spell_set(target_ptr, ms_ptr);
}
