/*!
 * 本ファイル内の行数はまともなレベルに落ち着いているので、一旦ここに留め置くこととする
 * @brief 魔法種別による各種処理切り替え
 * @date 2020/04/29
 * @author Hourier
 * @todo どうしても「その他」に分類せざるを得ない魔法種別が残った
 */

#include "effect/effect-monster-switcher.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-attack.h"
#include "effect/effect-monster-charm.h"
#include "effect/effect-monster-curse.h"
#include "effect/effect-monster-evil.h"
#include "effect/effect-monster-lite-dark.h"
#include "effect/effect-monster-oldies.h"
#include "effect/effect-monster-psi.h"
#include "effect/effect-monster-resist-hurt.h"
#include "effect/effect-monster-spirit.h"
#include "effect/effect-monster-util.h"
#include "mind/mind-elementalist.h"
#include "monster-floor/monster-death.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player/player-damage.h"
#include "spell-kind/spells-genocide.h"
#include "spell/spell-types.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

process_result effect_monster_hypodynamia(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (monster_living(em_ptr->m_ptr->r_idx)) {
        em_ptr->do_time = (em_ptr->dam + 7) / 8;
        return PROCESS_CONTINUE;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        if (em_ptr->r_ptr->flags3 & RF3_DEMON)
            em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
        if (em_ptr->r_ptr->flags3 & RF3_UNDEAD)
            em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
        if (em_ptr->r_ptr->flags3 & RF3_NONLIVING)
            em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
    }

    em_ptr->note = _("には効果がなかった。", " is unaffected.");
    em_ptr->obvious = false;
    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

/*!
 * @todo リファクタリング前のコード時点で、単に耐性があるだけでもダメージ0だった.
 */
process_result effect_monster_death_ray(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (!monster_living(em_ptr->m_ptr->r_idx)) {
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            if (em_ptr->r_ptr->flags3 & RF3_DEMON)
                em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
            if (em_ptr->r_ptr->flags3 & RF3_UNDEAD)
                em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
            if (em_ptr->r_ptr->flags3 & RF3_NONLIVING)
                em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
        }

        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->obvious = false;
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if (((em_ptr->r_ptr->flags1 & RF1_UNIQUE) && (randint1(888) != 666))
        || (((em_ptr->r_ptr->level + randint1(20)) > randint1((em_ptr->caster_lev / 2) + randint1(10))) && randint1(100) != 66)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->obvious = false;
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

process_result effect_monster_kill_wall(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->r_ptr->flags3 & (RF3_HURT_ROCK)) == 0) {
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

    em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
    em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
    return PROCESS_CONTINUE;
}

process_result effect_monster_hand_doom(effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flags1 & RF1_UNIQUE) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if ((em_ptr->who > 0) ? ((em_ptr->caster_lev + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + 10 + randint1(20)))
                          : (((em_ptr->caster_lev / 2) + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + randint1(200)))) {
        em_ptr->dam = ((40 + randint1(20)) * em_ptr->m_ptr->hp) / 100;
        if (em_ptr->m_ptr->hp < em_ptr->dam)
            em_ptr->dam = em_ptr->m_ptr->hp - 1;
    } else {
        em_ptr->note = _("は破滅の手に耐え切った！", "resists!");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

/*!
 * @brief 剣術「幻惑」の効果をモンスターに与える
 * @param player_ptr プレイヤーの情報へのポインタ
 * @param effect_monster_type モンスターの効果情報へのポインタ
 * @details
 * 精神のないモンスター、寝ているモンスターには無効。
 * 3回試行し、それぞれ2/5で失敗。
 * 寝た場合は試行終了。
 * 与える効果は減速、朦朧、混乱、睡眠。
 */
process_result effect_monster_engetsu(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (any_bits(em_ptr->r_ptr->flags2, RF2_EMPTY_MIND)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
        em_ptr->skipped = true;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            set_bits(em_ptr->r_ptr->r_flags2, RF2_EMPTY_MIND);
        return PROCESS_CONTINUE;
    }

    if (monster_csleep_remaining(em_ptr->m_ptr)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
        em_ptr->skipped = true;
        return PROCESS_CONTINUE;
    }

    bool done = false;
    for (int i = 0; i < 3; i++) {
        if (randint0(5) < 2)
            continue;

        int power = 10 + randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10));
        if (em_ptr->r_ptr->level > power)
            continue;

        switch (randint0(4)) {
        case 0:
            if (!any_bits(em_ptr->r_ptr->flags1, RF1_UNIQUE)) {
                if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, monster_slow_remaining(em_ptr->m_ptr) + 50)) {
                    em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
                }
                done = true;
            }
            break;
        case 1:
            if (any_bits(em_ptr->r_ptr->flags1, RF1_UNIQUE)) {
                em_ptr->do_stun = 0;
            } else {
                em_ptr->do_stun = damroll((player_ptr->lev / 10) + 3, (em_ptr->dam)) + 1;
                done = true;
            }
            break;
        case 2:
            if (any_bits(em_ptr->r_ptr->flags1, RF1_UNIQUE) || any_bits(em_ptr->r_ptr->flags3, RF3_NO_CONF)) {
                if (any_bits(em_ptr->r_ptr->flags3, RF3_NO_CONF)) {
                    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
                        set_bits(em_ptr->r_ptr->r_flags3, RF3_NO_CONF);
                }
                em_ptr->do_conf = 0;
            } else {
                /* Go to sleep (much) later */
                em_ptr->note = _("は混乱したようだ。", " looks confused.");
                em_ptr->do_conf = 10 + randint1(15);
                done = true;
            }
            break;
        default:
            if (any_bits(em_ptr->r_ptr->flags1, RF1_UNIQUE) || any_bits(em_ptr->r_ptr->flags3, RF3_NO_SLEEP)) {
                if (any_bits(em_ptr->r_ptr->flags3, RF3_NO_SLEEP)) {
                    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
                        set_bits(em_ptr->r_ptr->r_flags3, RF3_NO_SLEEP);
                }
                em_ptr->do_sleep = 0;
            } else {
                /* Go to sleep (much) later */
                em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
                em_ptr->do_sleep = 500;
                done = true;
            }
            break;
        }

        if (em_ptr->do_sleep > 0)
            break;
    }

    if (!done) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_genocide(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;
    if (genocide_aux(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, !em_ptr->who, (em_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One"))) {
        if (em_ptr->seen_msg)
            msg_format(_("%sは消滅した！", "%^s disappeared!"), em_ptr->m_name);
        chg_virtue(player_ptr, V_VITALITY, -1);
        return PROCESS_TRUE;
    }

    em_ptr->skipped = true;
    return PROCESS_CONTINUE;
}

process_result effect_monster_photo(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (!em_ptr->who)
        msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), em_ptr->m_name);

    if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE)) {
        if (em_ptr->seen)
            em_ptr->obvious = true;

        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

        em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
        em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
    } else {
        em_ptr->dam = 0;
    }

    em_ptr->photo = em_ptr->m_ptr->r_idx;
    return PROCESS_CONTINUE;
}

process_result effect_monster_wounds(effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (randint0(100 + em_ptr->dam) < (em_ptr->r_ptr->level + 50)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
    }

    return PROCESS_CONTINUE;
}

/*!
 * @brief 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return ここのスイッチングで終るならTRUEかFALSE、後続処理を実行するならCONTINUE
 */
process_result switch_effects_monster(player_type *player_ptr, effect_monster_type *em_ptr)
{
    switch (em_ptr->effect_type) {
    case GF_PSY_SPEAR:
    case GF_MISSILE:
    case GF_ARROW:
    case GF_MANA:
    case GF_METEOR:
    case GF_BLOOD_CURSE:
    case GF_SEEKER:
    case GF_SUPER_RAY:
        return effect_monster_nothing(em_ptr);
    case GF_ACID:
        return effect_monster_acid(player_ptr, em_ptr);
    case GF_ELEC:
        return effect_monster_elec(player_ptr, em_ptr);
    case GF_FIRE:
        return effect_monster_fire(player_ptr, em_ptr);
    case GF_COLD:
        return effect_monster_cold(player_ptr, em_ptr);
    case GF_POIS:
        return effect_monster_pois(player_ptr, em_ptr);
    case GF_NUKE:
        return effect_monster_nuke(player_ptr, em_ptr);
    case GF_HELL_FIRE:
        return effect_monster_hell_fire(player_ptr, em_ptr);
    case GF_HOLY_FIRE:
        return effect_monster_holy_fire(player_ptr, em_ptr);
    case GF_PLASMA:
        return effect_monster_plasma(player_ptr, em_ptr);
    case GF_NETHER:
        return effect_monster_nether(player_ptr, em_ptr);
    case GF_WATER:
        return effect_monster_water(player_ptr, em_ptr);
    case GF_CHAOS:
        return effect_monster_chaos(player_ptr, em_ptr);
    case GF_SHARDS:
        return effect_monster_shards(player_ptr, em_ptr);
    case GF_ROCKET:
        return effect_monster_rocket(player_ptr, em_ptr);
    case GF_SOUND:
        return effect_monster_sound(player_ptr, em_ptr);
    case GF_CONFUSION:
        return effect_monster_confusion(player_ptr, em_ptr);
    case GF_DISENCHANT:
        return effect_monster_disenchant(player_ptr, em_ptr);
    case GF_NEXUS:
        return effect_monster_nexus(player_ptr, em_ptr);
    case GF_FORCE:
        return effect_monster_force(player_ptr, em_ptr);
    case GF_INERTIAL:
        return effect_monster_inertial(player_ptr, em_ptr);
    case GF_TIME:
        return effect_monster_time(player_ptr, em_ptr);
    case GF_GRAVITY:
        return effect_monster_gravity(player_ptr, em_ptr);
    case GF_DISINTEGRATE:
        return effect_monster_disintegration(player_ptr, em_ptr);
    case GF_PSI:
        return effect_monster_psi(player_ptr, em_ptr);
    case GF_PSI_DRAIN:
        return effect_monster_psi_drain(player_ptr, em_ptr);
    case GF_TELEKINESIS:
        return effect_monster_telekinesis(player_ptr, em_ptr);
    case GF_DOMINATION:
        return effect_monster_domination(player_ptr, em_ptr);
    case GF_ICE:
        return effect_monster_icee_bolt(player_ptr, em_ptr);
    case GF_HYPODYNAMIA:
        return effect_monster_hypodynamia(player_ptr, em_ptr);
    case GF_DEATH_RAY:
        return effect_monster_death_ray(player_ptr, em_ptr);
    case GF_OLD_POLY:
        return effect_monster_old_poly(em_ptr);
    case GF_OLD_CLONE:
        return effect_monster_old_clone(player_ptr, em_ptr);
    case GF_STAR_HEAL:
        return effect_monster_star_heal(player_ptr, em_ptr);
    case GF_OLD_HEAL:
        return effect_monster_old_heal(player_ptr, em_ptr);
    case GF_OLD_SPEED:
        return effect_monster_old_speed(player_ptr, em_ptr);
    case GF_OLD_SLOW:
        return effect_monster_old_slow(player_ptr, em_ptr);
    case GF_OLD_SLEEP:
        return effect_monster_old_sleep(player_ptr, em_ptr);
    case GF_STASIS_EVIL:
        return effect_monster_stasis(em_ptr, true);
    case GF_STASIS:
        return effect_monster_stasis(em_ptr, false);
    case GF_CHARM:
        return effect_monster_charm(player_ptr, em_ptr);
    case GF_CONTROL_UNDEAD:
        return effect_monster_control_undead(player_ptr, em_ptr);
    case GF_CONTROL_DEMON:
        return effect_monster_control_demon(player_ptr, em_ptr);
    case GF_CONTROL_ANIMAL:
        return effect_monster_control_animal(player_ptr, em_ptr);
    case GF_CHARM_LIVING:
        return effect_monster_charm_living(player_ptr, em_ptr);
    case GF_OLD_CONF:
        return effect_monster_old_conf(player_ptr, em_ptr);
    case GF_STUN:
        return effect_monster_stun(em_ptr);
    case GF_LITE_WEAK:
        return effect_monster_lite_weak(player_ptr, em_ptr);
    case GF_LITE:
        return effect_monster_lite(player_ptr, em_ptr);
    case GF_DARK:
        return effect_monster_dark(player_ptr, em_ptr);
    case GF_KILL_WALL:
        return effect_monster_kill_wall(player_ptr, em_ptr);
    case GF_AWAY_UNDEAD:
        return effect_monster_away_undead(player_ptr, em_ptr);
    case GF_AWAY_EVIL:
        return effect_monster_away_evil(player_ptr, em_ptr);
    case GF_AWAY_ALL:
        return effect_monster_away_all(player_ptr, em_ptr);
    case GF_TURN_UNDEAD:
        return effect_monster_turn_undead(player_ptr, em_ptr);
    case GF_TURN_EVIL:
        return effect_monster_turn_evil(player_ptr, em_ptr);
    case GF_TURN_ALL:
        return effect_monster_turn_all(em_ptr);
    case GF_DISP_UNDEAD:
        return effect_monster_disp_undead(player_ptr, em_ptr);
    case GF_DISP_EVIL:
        return effect_monster_disp_evil(player_ptr, em_ptr);
    case GF_DISP_GOOD:
        return effect_monster_disp_good(player_ptr, em_ptr);
    case GF_DISP_LIVING:
        return effect_monster_disp_living(em_ptr);
    case GF_DISP_DEMON:
        return effect_monster_disp_demon(player_ptr, em_ptr);
    case GF_DISP_ALL:
        return effect_monster_disp_all(em_ptr);
    case GF_DRAIN_MANA:
        return effect_monster_drain_mana(player_ptr, em_ptr);
    case GF_MIND_BLAST:
        return effect_monster_mind_blast(player_ptr, em_ptr);
    case GF_BRAIN_SMASH:
        return effect_monster_brain_smash(player_ptr, em_ptr);
    case GF_CAUSE_1:
        return effect_monster_curse_1(em_ptr);
    case GF_CAUSE_2:
        return effect_monster_curse_2(em_ptr);
    case GF_CAUSE_3:
        return effect_monster_curse_3(em_ptr);
    case GF_CAUSE_4:
        return effect_monster_curse_4(em_ptr);
    case GF_HAND_DOOM:
        return effect_monster_hand_doom(em_ptr);
    case GF_CAPTURE:
        return effect_monster_capture(player_ptr, em_ptr);
    case GF_ATTACK:
        return (process_result)do_cmd_attack(player_ptr, em_ptr->y, em_ptr->x, static_cast<combat_options>(em_ptr->dam));
    case GF_ENGETSU:
        return effect_monster_engetsu(player_ptr, em_ptr);
    case GF_GENOCIDE:
        return effect_monster_genocide(player_ptr, em_ptr);
    case GF_PHOTO:
        return effect_monster_photo(player_ptr, em_ptr);
    case GF_CRUSADE:
        return effect_monster_crusade(player_ptr, em_ptr);
    case GF_WOUNDS:
        return effect_monster_wounds(em_ptr);
    case GF_E_GENOCIDE:
        return effect_monster_elemental_genocide(player_ptr, em_ptr);
    case GF_VOID:
        return effect_monster_void(player_ptr, em_ptr);
    case GF_ABYSS:
        return effect_monster_abyss(player_ptr, em_ptr);
    default: {
        em_ptr->skipped = true;
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }
    }
}
