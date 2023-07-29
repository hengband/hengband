#include "effect/effect-monster-resist-hurt.h"
#include "effect/effect-monster-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-flags.h"
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
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターの耐性がなく、効果もない場合の処理
 * @param em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続ける
 */
ProcessResult effect_monster_nothing(EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_acid(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::IMMUNE_ACID)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_ACID);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_elec(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::IMMUNE_ELEC)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_ELEC);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_fire(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
        }

        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_FIRE)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_FIRE);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_cold(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_COLD);
        }

        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_COLD)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_pois(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::IMMUNE_POISON)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_POISON);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_nuke(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_POISON);
        }

        return ProcessResult::PROCESS_CONTINUE;
    }

    if (one_in_(3)) {
        em_ptr->do_polymorph = true;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_hell_fire(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::GOOD)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::GOOD);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_holy_fire(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::GOOD);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        em_ptr->dam *= 2;
        em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_plasma(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_PLASMA)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_PLASMA);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

static bool effect_monster_nether_resist(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_NETHER)) {
        return false;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::UNDEAD);
        }
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_NETHER);
    }

    return true;
}

ProcessResult effect_monster_nether(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (effect_monster_nether_resist(player_ptr, em_ptr) || (em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::EVIL))) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_water(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_WATER)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if ((em_ptr->m_ptr->r_idx == MonsterRaceId::WATER_ELEM) || (em_ptr->m_ptr->r_idx == MonsterRaceId::UNMAKER)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_WATER);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_chaos(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_CHAOS)) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_CHAOS);
        }
    } else if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::DEMON) && one_in_(3)) {
        em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::DEMON);
        }
    } else {
        em_ptr->do_polymorph = true;
        em_ptr->do_conf = (5 + randint1(11) + em_ptr->r) / (em_ptr->r + 1);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_shards(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_SHARDS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_SHARDS);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_rocket(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_SHARDS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_SHARDS);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_sound(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_SOUND)) {
        em_ptr->do_stun = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 2;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_SOUND);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_confusion(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if ((em_ptr->r_ptr->flags3 & RF3_NO_CONF) == 0) {
        em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_disenchant(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_DISENCHANT)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_DISENCHANT);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_nexus(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_NEXUS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_NEXUS);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_force(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_FORCE)) {
        em_ptr->do_stun = (randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_FORCE);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

// Powerful monsters can resists and normal monsters slow down.
ProcessResult effect_monster_inertial(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_INERTIA)) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_INERTIA);
        }

        return ProcessResult::PROCESS_CONTINUE;
    }

    bool cancel_effect = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    cancel_effect |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (cancel_effect) {
        em_ptr->obvious = false;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_time(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_TIME)) {
        em_ptr->do_time = (em_ptr->dam + 1) / 2;
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TIME);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

static bool effect_monster_gravity_resist_teleport(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_TELEPORT)) {
        em_ptr->obvious = true;
        return false;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        em_ptr->note = _("には効果がなかった。", " is unaffected!");
        return true;
    }

    if (em_ptr->r_ptr->level <= randint1(100)) {
        em_ptr->obvious = true;
        return false;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    return true;
}

static void effect_monster_gravity_slow(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    bool cancel_effect = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    cancel_effect |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (cancel_effect) {
        em_ptr->obvious = false;
        return;
    }

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }
    em_ptr->obvious = true;
}

static void effect_monster_gravity_stun(EffectMonster *em_ptr)
{
    em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (has_resistance) {
        em_ptr->do_stun = 0;
        return;
    }
    em_ptr->obvious = true;
}

/*
 * Powerful monsters can resist and normal monsters slow down
 * Furthermore, this magic can make non-unique monsters slow/stun.
 */
ProcessResult effect_monster_gravity(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    em_ptr->do_dist = effect_monster_gravity_resist_teleport(player_ptr, em_ptr) ? 0 : 10;
    if (player_ptr->riding && (em_ptr->g_ptr->m_idx == player_ptr->riding)) {
        em_ptr->do_dist = 0;
    }

    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        em_ptr->do_dist = 0;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_GRAVITY);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には効果がなかった。", " is unaffected!");

    effect_monster_gravity_slow(player_ptr, em_ptr);
    effect_monster_gravity_stun(em_ptr);
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_disintegration(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_ROCK)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_ROCK);
    }

    em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
    em_ptr->note_dies = _("は蒸発した！", " evaporates!");
    em_ptr->dam *= 2;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_icee_bolt(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    em_ptr->do_stun = (randint1(15) + 1) / (em_ptr->r + 1);
    if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
        em_ptr->dam /= 9;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_COLD);
        }
    } else if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
        em_ptr->dam *= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
        }
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 虚無属性の耐性と効果の発動
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 量子生物に倍打、壁抜けに1.5倍打、テレポート耐性が耐性
 */
ProcessResult effect_monster_void(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        em_ptr->note = _("の存在確率が減少した。", "'s wave function is reduced.");
        em_ptr->note_dies = _("は観測されなくなった。", "'s wave function collapses.");
        em_ptr->dam *= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::QUANTUM);
        }
    } else if (em_ptr->r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
        em_ptr->note = _("の存在が薄れていく。", "is fading out.");
        em_ptr->note_dies = _("は消えてしまった。", "has disappeared.");
        em_ptr->dam *= 3;
        em_ptr->dam /= 2;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_feature_flags.set(MonsterFeatureType::PASS_WALL);
        }
    } else if (em_ptr->r_ptr->resistance_flags.has_any_of({ MonsterResistanceType::RESIST_TELEPORT, MonsterResistanceType::RESIST_FORCE, MonsterResistanceType::RESIST_GRAVITY })) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
                em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_FORCE)) {
                em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_FORCE);
            }
            if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
                em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_GRAVITY);
            }
        }
    } else {
        em_ptr->note_dies = _("は消滅してしまった。", "has vanished.");
    }

    return ProcessResult::PROCESS_CONTINUE;
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
ProcessResult effect_monster_abyss(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    auto dark = { MonsterBrightnessType::SELF_DARK_1, MonsterBrightnessType::SELF_DARK_2, MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::HAS_DARK_2 };

    if (em_ptr->r_ptr->brightness_flags.has_any_of(dark)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(6) + 6);
    } else if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DARK)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(4) + 5);
        if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_DARK);
        }
    } else if (em_ptr->r_ptr->feature_flags.has_not(MonsterFeatureType::CAN_FLY)) {
        if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
            em_ptr->dam *= 5;
            em_ptr->dam /= 4;
            if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
                em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
        }

        em_ptr->note = _("は深淵に囚われていく。", " is trapped in the abyss.");
        em_ptr->note_dies = _("は深淵に堕ちてしまった。", " has fallen into the abyss.");

        if (one_in_(3) && set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
            em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
        }
    }

    if (any_bits(em_ptr->r_ptr->flags2, RF2_ELDRITCH_HORROR) || any_bits(em_ptr->r_ptr->flags2, RF2_EMPTY_MIND)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (one_in_(3)) {
        if (one_in_(2)) {
            em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        } else {
            em_ptr->do_fear = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        }
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_meteor(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_METEOR)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_METEOR);
    }

    return ProcessResult::PROCESS_CONTINUE;
}
