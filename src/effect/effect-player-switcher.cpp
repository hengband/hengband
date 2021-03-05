﻿#include "effect/effect-player-switcher.h"
#include "effect/effect-player-curse.h"
#include "effect/effect-player-oldies.h"
#include "effect/effect-player-resist-hurt.h"
#include "effect/effect-player-spirit.h"
#include "player/player-damage.h"
#include "spell/spell-types.h"

/*!
 * @brief
 * 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param em_ptr プレーヤー効果構造体への参照ポインタ
 * @return なし
 */
void switch_effects_player(player_type *target_ptr, effect_player_type *ep_ptr)
{
    switch (ep_ptr->effect_type) {
    case GF_ACID:
        effect_player_elements(target_ptr, ep_ptr, _("酸で攻撃された！", "You are hit by acid!"), acid_dam);
        return;
    case GF_FIRE:
        effect_player_elements(target_ptr, ep_ptr, _("火炎で攻撃された！", "You are hit by fire!"), fire_dam);
        return;
    case GF_COLD:
        effect_player_elements(target_ptr, ep_ptr, _("冷気で攻撃された！", "You are hit by cold!"), cold_dam);
        return;
    case GF_ELEC:
        effect_player_elements(target_ptr, ep_ptr, _("電撃で攻撃された！", "You are hit by lightning!"), elec_dam);
        return;
    case GF_POIS:
        effect_player_poison(target_ptr, ep_ptr);
        return;
    case GF_NUKE:
        effect_player_nuke(target_ptr, ep_ptr);
        return;
    case GF_MISSILE:
        effect_player_missile(target_ptr, ep_ptr);
        return;
    case GF_HOLY_FIRE:
        effect_player_holy_fire(target_ptr, ep_ptr);
        return;
    case GF_HELL_FIRE:
        effect_player_hell_fire(target_ptr, ep_ptr);
        return;
    case GF_ARROW:
        effect_player_arrow(target_ptr, ep_ptr);
        return;
    case GF_PLASMA:
        effect_player_plasma(target_ptr, ep_ptr);
        return;
    case GF_NETHER:
        effect_player_nether(target_ptr, ep_ptr);
        return;
    case GF_WATER:
        effect_player_water(target_ptr, ep_ptr);
        return;
    case GF_CHAOS:
        effect_player_chaos(target_ptr, ep_ptr);
        return;
    case GF_SHARDS:
        effect_player_shards(target_ptr, ep_ptr);
        return;
    case GF_SOUND:
        effect_player_sound(target_ptr, ep_ptr);
        return;
    case GF_CONFUSION:
        effect_player_confusion(target_ptr, ep_ptr);
        return;
    case GF_DISENCHANT:
        effect_player_disenchant(target_ptr, ep_ptr);
        return;
    case GF_NEXUS:
        effect_player_nexus(target_ptr, ep_ptr);
        return;
    case GF_FORCE:
        effect_player_force(target_ptr, ep_ptr);
        return;
    case GF_ROCKET:
        effect_player_rocket(target_ptr, ep_ptr);
        return;
    case GF_INERTIAL:
        effect_player_inertial(target_ptr, ep_ptr);
        return;
    case GF_LITE:
        effect_player_lite(target_ptr, ep_ptr);
        return;
    case GF_DARK:
        effect_player_dark(target_ptr, ep_ptr);
        return;
    case GF_TIME:
        effect_player_time(target_ptr, ep_ptr);
        return;
    case GF_GRAVITY:
        effect_player_gravity(target_ptr, ep_ptr);
        return;
    case GF_DISINTEGRATE:
        effect_player_disintegration(target_ptr, ep_ptr);
        return;
    case GF_OLD_HEAL:
        effect_player_old_heal(target_ptr, ep_ptr);
        return;
    case GF_OLD_SPEED:
        effect_player_old_speed(target_ptr, ep_ptr);
        return;
    case GF_OLD_SLOW:
        effect_player_old_slow(target_ptr);
        return;
    case GF_OLD_SLEEP:
        effect_player_old_sleep(target_ptr, ep_ptr);
        return;
    case GF_MANA:
    case GF_SEEKER:
    case GF_SUPER_RAY:
        effect_player_mana(target_ptr, ep_ptr);
        return;
    case GF_PSY_SPEAR:
        effect_player_psy_spear(target_ptr, ep_ptr);
        return;
    case GF_METEOR:
        effect_player_meteor(target_ptr, ep_ptr);
        return;
    case GF_ICE:
        effect_player_icee(target_ptr, ep_ptr);
        return;
    case GF_DEATH_RAY:
        effect_player_death_ray(target_ptr, ep_ptr);
        return;
    case GF_DRAIN_MANA:
        effect_player_drain_mana(target_ptr, ep_ptr);
        return;
    case GF_MIND_BLAST:
        effect_player_mind_blast(target_ptr, ep_ptr);
        return;
    case GF_BRAIN_SMASH:
        effect_player_brain_smash(target_ptr, ep_ptr);
        return;
    case GF_CAUSE_1:
        effect_player_curse_1(target_ptr, ep_ptr);
        return;
    case GF_CAUSE_2:
        effect_player_curse_2(target_ptr, ep_ptr);
        return;
    case GF_CAUSE_3:
        effect_player_curse_3(target_ptr, ep_ptr);
        return;
    case GF_CAUSE_4:
        effect_player_curse_4(target_ptr, ep_ptr);
        return;
    case GF_HAND_DOOM:
        effect_player_hand_doom(target_ptr, ep_ptr);
        return;
    default: {
        ep_ptr->dam = 0;
        return;
    }
    }
}
