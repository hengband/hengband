#include "effect/effect-monster-oldies.h"
#include "avatar/avatar.h"
#include "core/player-redraw-types.h"
#include "effect/effect-monster-util.h"
#include "monster-floor/monster-generator.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

// Powerful monsters can resist.
process_result effect_monster_old_poly(effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;
    em_ptr->do_polymorph = true;

    if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) || (em_ptr->r_ptr->flags1 & RF1_QUESTOR)
        || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->do_polymorph = false;
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_old_clone(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((player_ptr->current_floor_ptr->inside_arena) || is_pet(em_ptr->m_ptr) || (em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR))
        || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2))) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;
    if (multiply_monster(player_ptr, em_ptr->g_ptr->m_idx, true, 0L))
        em_ptr->note = _("が分裂した！", " spawns!");

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_star_heal(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, 0);

    if (em_ptr->m_ptr->maxhp < em_ptr->m_ptr->max_maxhp) {
        if (em_ptr->seen_msg)
            msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), em_ptr->m_name, em_ptr->m_poss);
        em_ptr->m_ptr->maxhp = em_ptr->m_ptr->max_maxhp;
    }

    if (!em_ptr->dam) {
        if (player_ptr->health_who == em_ptr->g_ptr->m_idx)
            player_ptr->redraw |= (PR_HEALTH);
        if (player_ptr->riding == em_ptr->g_ptr->m_idx)
            player_ptr->redraw |= (PR_UHEALTH);

        return PROCESS_FALSE;
    }

    effect_monster_old_heal(player_ptr, em_ptr);
    return PROCESS_TRUE;
}

// who == 0ならばプレイヤーなので、それの判定.
static void effect_monster_old_heal_check_player(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->who != 0)
        return;

    chg_virtue(player_ptr, V_VITALITY, 1);
    if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
        chg_virtue(player_ptr, V_INDIVIDUALISM, 1);

    if (is_friendly(em_ptr->m_ptr))
        chg_virtue(player_ptr, V_HONOUR, 1);
    else if (!(em_ptr->r_ptr->flags3 & RF3_EVIL)) {
        if (em_ptr->r_ptr->flags3 & RF3_GOOD)
            chg_virtue(player_ptr, V_COMPASSION, 2);
        else
            chg_virtue(player_ptr, V_COMPASSION, 1);
    }

    if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
        chg_virtue(player_ptr, V_NATURE, 1);
}

static void effect_monster_old_heal_recovery(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (monster_stunned_remaining(em_ptr->m_ptr)) {
        if (em_ptr->seen_msg)
            msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), em_ptr->m_name);

        (void)set_monster_stunned(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }

    if (monster_confused_remaining(em_ptr->m_ptr)) {
        if (em_ptr->seen_msg)
            msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), em_ptr->m_name);

        (void)set_monster_confused(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }

    if (monster_fear_remaining(em_ptr->m_ptr)) {
        if (em_ptr->seen_msg)
            msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), em_ptr->m_name, em_ptr->m_poss);

        (void)set_monster_monfear(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }
}

process_result effect_monster_old_heal(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    /* Wake up */
    (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, 0);
    effect_monster_old_heal_recovery(player_ptr, em_ptr);
    if (em_ptr->m_ptr->hp < MONSTER_MAXHP)
        em_ptr->m_ptr->hp += em_ptr->dam;
    if (em_ptr->m_ptr->hp > em_ptr->m_ptr->maxhp)
        em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;

    effect_monster_old_heal_check_player(player_ptr, em_ptr);
    if (em_ptr->m_ptr->r_idx == MON_LEPER) {
        em_ptr->heal_leper = true;
        if (!em_ptr->who)
            chg_virtue(player_ptr, V_COMPASSION, 5);
    }

    if (player_ptr->health_who == em_ptr->g_ptr->m_idx)
        player_ptr->redraw |= (PR_HEALTH);
    if (player_ptr->riding == em_ptr->g_ptr->m_idx)
        player_ptr->redraw |= (PR_UHEALTH);

    em_ptr->note = _("は体力を回復したようだ。", " looks healthier.");
    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_old_speed(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, monster_fast_remaining(em_ptr->m_ptr) + 100)) {
        em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
    }

    if (!em_ptr->who) {
        if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
            chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
        if (is_friendly(em_ptr->m_ptr))
            chg_virtue(player_ptr, V_HONOUR, 1);
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_old_slow(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    /* Powerful monsters can resist */
    if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, monster_slow_remaining(em_ptr->m_ptr) + 50))
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

/*!
 * @todo 「ユニークは (魔法では)常に眠らない」はr_infoの趣旨に反すると思われる
 * 眠る確率を半分にするとかしておいた方が良さそう
 */
process_result effect_monster_old_sleep(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) || (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
        || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        if (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP) {
            if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
                em_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
        }

        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    } else {
        em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
        em_ptr->do_sleep = 500;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

/*!
 * @todo 「ユニークは (魔法では)常に混乱しない」はr_infoの趣旨に反すると思われる
 * 眠る確率を半分にするとかしておいた方が良さそう
 */
process_result effect_monster_old_conf(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    em_ptr->do_conf = damroll(3, (em_ptr->dam / 2)) + 1;
    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
        || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF)) {
            if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
                em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
        }

        em_ptr->do_conf = 0;
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_stasis(effect_monster_type *em_ptr, bool to_evil)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    int stasis_damage = (em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10);
    bool has_resistance = (em_ptr->r_ptr->flags1 & RF1_UNIQUE) != 0;
    has_resistance |= em_ptr->r_ptr->level > randint1(stasis_damage) + 10;
    if (to_evil)
        has_resistance |= (em_ptr->r_ptr->flags3 & RF3_EVIL) == 0;

    if (has_resistance) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    } else {
        em_ptr->note = _("は動けなくなった！", " is suspended!");
        em_ptr->do_sleep = 500;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_stun(effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        em_ptr->do_stun = 0;
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}
