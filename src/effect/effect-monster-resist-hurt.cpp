#include "effect/effect-monster-resist-hurt.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターの耐性がなく、効果もない場合の処理
 * @param em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続ける
 */
process_result effect_monster_nothing(effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    return PROCESS_CONTINUE;
}

process_result effect_monster_acid(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_IM_ACID) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_IM_ACID);

    return PROCESS_CONTINUE;
}

process_result effect_monster_elec(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_IM_ELEC) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_IM_ELEC);

    return PROCESS_CONTINUE;
}

process_result effect_monster_fire(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flagsr & RFR_IM_FIRE) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_IM_FIRE);

        return PROCESS_CONTINUE;
    }

    if ((em_ptr->r_ptr->flags3 & (RF3_HURT_FIRE)) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_HURT_FIRE);

    return PROCESS_CONTINUE;
}

process_result effect_monster_cold(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flagsr & RFR_IM_COLD) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);

        return PROCESS_CONTINUE;
    }

    if ((em_ptr->r_ptr->flags3 & (RF3_HURT_COLD)) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);

    return PROCESS_CONTINUE;
}

process_result effect_monster_pois(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_IM_POIS) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);

    return PROCESS_CONTINUE;
}

process_result effect_monster_nuke(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flagsr & RFR_IM_POIS) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);

        return PROCESS_CONTINUE;
    }

    if (one_in_(3))
        em_ptr->do_polymorph = true;

    return PROCESS_CONTINUE;
}

process_result effect_monster_hell_fire(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flags3 & RF3_GOOD) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= RF3_GOOD;

    return PROCESS_CONTINUE;
}

process_result effect_monster_holy_fire(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (any_bits(em_ptr->r_ptr->flags3, RF3_GOOD)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            set_bits(em_ptr->r_ptr->r_flags3, RF3_GOOD);
        return PROCESS_CONTINUE;
    }

    if (any_bits(em_ptr->r_ptr->flags3, RF3_EVIL)) {
        em_ptr->dam *= 2;
        em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            set_bits(em_ptr->r_ptr->r_flags3, RF3_EVIL);
        return PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;

    return PROCESS_CONTINUE;
}

process_result effect_monster_plasma(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_PLAS) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_PLAS);

    return PROCESS_CONTINUE;
}

static bool effect_monster_nether_resist(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->r_ptr->flagsr & RFR_RES_NETH) == 0)
        return false;

    if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_NETH);

    return true;
}

process_result effect_monster_nether(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (effect_monster_nether_resist(player_ptr, em_ptr) || ((em_ptr->r_ptr->flags3 & RF3_EVIL) == 0))
        return PROCESS_CONTINUE;

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

    return PROCESS_CONTINUE;
}

process_result effect_monster_water(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_WATE) == 0)
        return PROCESS_CONTINUE;

    if ((em_ptr->m_ptr->r_idx == MON_WATER_ELEM) || (em_ptr->m_ptr->r_idx == MON_UNMAKER)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_WATE);

    return PROCESS_CONTINUE;
}

process_result effect_monster_chaos(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flagsr & RFR_RES_CHAO) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_RES_CHAO);
    } else if ((em_ptr->r_ptr->flags3 & RF3_DEMON) && one_in_(3)) {
        em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
    } else {
        em_ptr->do_polymorph = true;
        em_ptr->do_conf = (5 + randint1(11) + em_ptr->r) / (em_ptr->r + 1);
    }

    return PROCESS_CONTINUE;
}

process_result effect_monster_shards(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_SHAR) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);

    return PROCESS_CONTINUE;
}

process_result effect_monster_rocket(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_SHAR) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);

    return PROCESS_CONTINUE;
}

process_result effect_monster_sound(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_SOUN) == 0) {
        em_ptr->do_stun = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 2;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_SOUN);

    return PROCESS_CONTINUE;
}

process_result effect_monster_confusion(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flags3 & RF3_NO_CONF) == 0) {
        em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);

    return PROCESS_CONTINUE;
}

process_result effect_monster_disenchant(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_DISE) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_DISE);

    return PROCESS_CONTINUE;
}

process_result effect_monster_nexus(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_NEXU) == 0)
        return PROCESS_CONTINUE;

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_NEXU);

    return PROCESS_CONTINUE;
}

process_result effect_monster_force(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_WALL) == 0) {
        em_ptr->do_stun = (randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_WALL);

    return PROCESS_CONTINUE;
}

// Powerful monsters can resists and normal monsters slow down.
process_result effect_monster_inertial(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (em_ptr->r_ptr->flagsr & RFR_RES_INER) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_RES_INER);

        return PROCESS_CONTINUE;
    }

    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        em_ptr->obvious = false;
        return PROCESS_CONTINUE;
    }

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, monster_slow_remaining(em_ptr->m_ptr) + 50))
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");

    return PROCESS_CONTINUE;
}

process_result effect_monster_time(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_TIME) == 0) {
        em_ptr->do_time = (em_ptr->dam + 1) / 2;
        return PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= (RFR_RES_TIME);

    return PROCESS_CONTINUE;
}

static bool effect_monster_gravity_resist_teleport(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flagsr & RFR_RES_TELE) == 0)
        return false;

    if (em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) {
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;

        em_ptr->note = _("には効果がなかった。", " is unaffected!");
        return true;
    }

    if (em_ptr->r_ptr->level <= randint1(100))
        return false;

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;

    em_ptr->note = _("には耐性がある！", " resists!");
    return true;
}

static void effect_monster_gravity_slow(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
        em_ptr->obvious = false;

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, monster_slow_remaining(em_ptr->m_ptr) + 50))
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
}

static void effect_monster_gravity_stun(effect_monster_type *em_ptr)
{
    em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        em_ptr->do_stun = 0;
        em_ptr->note = _("には効果がなかった。", " is unaffected!");
        em_ptr->obvious = false;
    }
}

/*
 * Powerful monsters can resist and normal monsters slow down
 * Furthermore, this magic can make non-unique monsters slow/stun.
 */
process_result effect_monster_gravity(player_type *player_ptr, effect_monster_type *em_ptr)
{
    em_ptr->do_dist = effect_monster_gravity_resist_teleport(player_ptr, em_ptr) ? 0 : 10;
    if (player_ptr->riding && (em_ptr->g_ptr->m_idx == player_ptr->riding))
        em_ptr->do_dist = 0;

    if (em_ptr->r_ptr->flagsr & RFR_RES_GRAV) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        em_ptr->do_dist = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_RES_GRAV);

        return PROCESS_CONTINUE;
    }

    effect_monster_gravity_slow(player_ptr, em_ptr);
    effect_monster_gravity_stun(em_ptr);
    return PROCESS_CONTINUE;
}

process_result effect_monster_disintegration(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flags3 & RF3_HURT_ROCK) == 0)
        return PROCESS_CONTINUE;

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

    em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
    em_ptr->note_dies = _("は蒸発した！", " evaporates!");
    em_ptr->dam *= 2;
    return PROCESS_CONTINUE;
}

process_result effect_monster_icee_bolt(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    em_ptr->do_stun = (randint1(15) + 1) / (em_ptr->r + 1);
    if (em_ptr->r_ptr->flagsr & RFR_IM_COLD) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
    } else if (em_ptr->r_ptr->flags3 & (RF3_HURT_COLD)) {
        em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
        em_ptr->dam *= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
    }

    return PROCESS_CONTINUE;
}

/*!
 * @brief 虚無属性の耐性と効果の発動
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 量子生物に倍打、壁抜けに1.5倍打、テレポート耐性が耐性
 */
process_result effect_monster_void(player_type* player_ptr, effect_monster_type* em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    if (any_bits(em_ptr->r_ptr->flags2, RF2_QUANTUM)) {
        em_ptr->note = _("の存在確率が減少した。", "'s wave function is reduced.");
        em_ptr->note_dies = _("は観測されなくなった。", "'s wave function is collapsed.");
        em_ptr->dam *= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            set_bits(em_ptr->r_ptr->r_flags2, RF2_QUANTUM);
    } else if (any_bits(em_ptr->r_ptr->flags2, RF2_PASS_WALL)) {
        em_ptr->note = _("の存在が薄れていく。", "is fading out.");
        em_ptr->note_dies = _("は消えてしまった。", "has disappeared.");
        em_ptr->dam *= 3;
        em_ptr->dam /= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            set_bits(em_ptr->r_ptr->r_flags2, RF2_PASS_WALL);
    } else if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_TELE | RFR_RES_WALL | RFR_RES_GRAV)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_TELE))
                set_bits(em_ptr->r_ptr->r_flagsr, RFR_RES_TELE);
            if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_WALL))
                set_bits(em_ptr->r_ptr->r_flagsr, RFR_RES_WALL);
            if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_GRAV))
                set_bits(em_ptr->r_ptr->r_flagsr, RFR_RES_GRAV);
        }
    } else
        em_ptr->note_dies = _("は消滅してしまった。", "has vanished.");

    return PROCESS_CONTINUE;
}

/*!
 * @brief 深淵属性の耐性と効果の発動
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 飛ばないテレポート耐性に1.25倍打、暗黒耐性が耐性
 * 1/3で追加に混乱か恐怖
 */
process_result effect_monster_abyss(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    BIT_FLAGS dark = RF7_SELF_DARK_1 | RF7_SELF_DARK_2 | RF7_HAS_DARK_1 | RF7_HAS_DARK_2;

    if (any_bits(em_ptr->r_ptr->flags7, dark)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(6) + 6);
    } else if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_DARK)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(4) + 5);
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flagsr |= (RFR_RES_DARK);
    } else if (!any_bits(em_ptr->r_ptr->flags7, RF7_CAN_FLY)) {
        if (any_bits(em_ptr->r_ptr->flagsr, RFR_RES_TELE)) {
            em_ptr->dam *= 5;
            em_ptr->dam /= 4;
            if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
                set_bits(em_ptr->r_ptr->r_flagsr, RFR_RES_TELE);
        }

        em_ptr->note = _("は深淵に囚われていく。", " has be captured by abyss.");
        em_ptr->note_dies = _("は深淵に堕ちてしまった。", " has fall in abyss.");

        if (one_in_(3) && set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, monster_slow_remaining(em_ptr->m_ptr) + 50))
            em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }

    if (any_bits(em_ptr->r_ptr->flags2, RF2_ELDRITCH_HORROR) || any_bits(em_ptr->r_ptr->flags2, RF2_EMPTY_MIND))
        return PROCESS_CONTINUE;

    if (one_in_(3)) {
        if (one_in_(2))
            em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        else
            em_ptr->do_fear = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
    }

    return PROCESS_CONTINUE;
}
