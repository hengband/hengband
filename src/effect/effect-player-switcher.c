#include "effect/effect-player-switcher.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-action/cmd-attack.h"
#include "effect/effect-player-curse.h"
#include "effect/effect-player-oldies.h"
#include "effect/effect-player-resist-hurt.h"
#include "effect/effect-player-spirit.h"
#include "inventory/inventory-damage.h"
#include "mind/mind-mirror-master.h"
#include "object-enchant/object-curse.h"
#include "object/object-broken.h"
#include "player/mimic-info-table.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-status-flags.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_mana(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_psy_spear(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_FORCE, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_meteor(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
    if (!target_ptr->resist_shard || one_in_(13)) {
        if (!is_immune_fire(target_ptr))
            inventory_damage(target_ptr, set_fire_destroy, 2);
        inventory_damage(target_ptr, set_cold_destroy, 2);
    }
}

void effect_player_icee(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));

    ep_ptr->get_damage = cold_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
    if (check_multishadow(target_ptr))
        return;

    if (!target_ptr->resist_shard) {
        (void)set_cut(target_ptr, target_ptr->cut + damroll(5, 8));
    }

    if (!target_ptr->resist_sound) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(15));
    }

    if ((!(target_ptr->resist_cold || is_oppose_cold(target_ptr))) || one_in_(12)) {
        if (!is_immune_cold(target_ptr))
            inventory_damage(target_ptr, set_cold_destroy, 3);
    }
}

void effect_player_death_ray(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));

    if (target_ptr->mimic_form) {
        if (!(mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
            ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

        return;
    }

    switch (target_ptr->prace) {
    case RACE_GOLEM:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_VAMPIRE:
    case RACE_BALROG:
    case RACE_SPECTRE: {
        ep_ptr->dam = 0;
        break;
    }
    default: {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        break;
    }
    }
}

void effect_player_hand_doom(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !check_multishadow(target_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        learn_spell(target_ptr, ep_ptr->monspell);
    } else {
        if (!check_multishadow(target_ptr)) {
            msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
            curse_equipment(target_ptr, 40, 20);
        }

        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->m_name, ep_ptr->monspell);

        if (target_ptr->chp < 1)
            target_ptr->chp = 1;
    }
}

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
