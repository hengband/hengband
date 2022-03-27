#include "effect/effect-player-resist-hurt.h"
#include "artifact/fixed-art-types.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-player.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-damage.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/race-indice-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/object-curse.h"
#include "object/object-broken.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "spell-kind/spells-equipment.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

// 毒を除く4元素.
void effect_player_elements(
    PlayerType *player_ptr, EffectPlayerType *ep_ptr, concptr attack_message, int (*damage_func)(PlayerType *, int, concptr, bool))
{
    if (player_ptr->blind) {
        msg_print(attack_message);
    }

    ep_ptr->get_damage = (*damage_func)(player_ptr, ep_ptr->dam, ep_ptr->killer, false);
}

void effect_player_poison(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    bool double_resist = is_oppose_pois(player_ptr);
    if (player_ptr->blind) {
        msg_print(_("毒で攻撃された！", "You are hit by poison!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_pois_damage_rate(player_ptr) / 100;

    if ((!(double_resist || has_resist_pois(player_ptr))) && one_in_(HURT_CHANCE) && !check_multishadow(player_ptr)) {
        do_dec_stat(player_ptr, A_CON);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!(double_resist || has_resist_pois(player_ptr)) && !check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_poison(randint0(ep_ptr->dam) + 10);
    }
}

void effect_player_nuke(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    bool double_resist = is_oppose_pois(player_ptr);
    if (player_ptr->blind) {
        msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_pois_damage_rate(player_ptr) / 100;

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if ((double_resist || has_resist_pois(player_ptr)) || check_multishadow(player_ptr)) {
        return;
    }

    (void)BadStatusSetter(player_ptr).mod_poison(randint0(ep_ptr->dam) + 10);
    if (one_in_(5)) /* 6 */
    {
        msg_print(_("奇形的な変身を遂げた！", "You undergo a freakish metamorphosis!"));
        if (one_in_(4)) { /* 4 */
            do_poly_self(player_ptr);
        } else {
            status_shuffle(player_ptr);
        }
    }

    if (one_in_(6)) {
        inventory_damage(player_ptr, BreakerAcid(), 2);
    }
}

void effect_player_missile(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_holy_fire(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_holy_fire_damage_rate(player_ptr, CALC_RAND) / 100;

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_hell_fire(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_hell_fire_damage_rate(player_ptr, CALC_RAND) / 100;

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_arrow(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        sound(SOUND_SHOOT_HIT);
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    if (has_invuln_arrow(player_ptr)) {
        msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
        return;
    }

    sound(SOUND_SHOOT_HIT);
    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_plasma(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!has_resist_sound(player_ptr) && !check_multishadow(player_ptr)) {
        TIME_EFFECT plus_stun = (randint1((ep_ptr->dam > 40) ? 35 : (ep_ptr->dam * 3 / 4 + 5)));
        (void)BadStatusSetter(player_ptr).mod_stun(plus_stun);
    }

    if (!(has_resist_fire(player_ptr) || is_oppose_fire(player_ptr) || has_immune_fire(player_ptr))) {
        inventory_damage(player_ptr, BreakerAcid(), 3);
    }
}

/*!
 * @brief 地獄属性によるダメージを受ける
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 * @details
 * 幽霊は回復する。追加効果で経験値吸収。
 */

void effect_player_nether(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));
    }

    bool evaded = check_multishadow(player_ptr);

    if (PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
        if (!evaded) {
            hp_player(player_ptr, ep_ptr->dam / 4);
        }
        ep_ptr->get_damage = 0;
        return;
    }

    ep_ptr->dam = ep_ptr->dam * calc_nether_damage_rate(player_ptr, CALC_RAND) / 100;

    if (!has_resist_neth(player_ptr) && !evaded) {
        drain_exp(player_ptr, 200 + (player_ptr->exp / 100), 200 + (player_ptr->exp / 1000), 75);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

/*!
 * @brief 水流属性によるダメージを受ける
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 * @details
 * 追加効果で朦朧と混乱、冷気同様のインベントリ破壊。
 */
void effect_player_water(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
    }

    if (check_multishadow(player_ptr)) {
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    ep_ptr->dam = ep_ptr->dam * calc_water_damage_rate(player_ptr, CALC_RAND) / 100;

    BIT_FLAGS has_res_water = has_resist_water(player_ptr);
    BadStatusSetter bss(player_ptr);
    if (!check_multishadow(player_ptr)) {
        if (!has_resist_sound(player_ptr) && !has_res_water) {
            (void)bss.mod_stun(randint1(40));
        }

        if (!has_resist_conf(player_ptr) && !has_res_water) {
            (void)bss.mod_confusion(randint1(5) + 5);
        }

        if (one_in_(5) && !has_res_water) {
            inventory_damage(player_ptr, BreakerCold(), 3);
        }
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_chaos(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_chaos_damage_rate(player_ptr, CALC_RAND) / 100;
    if (check_multishadow(player_ptr)) {
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    BadStatusSetter bss(player_ptr);
    if (!has_resist_conf(player_ptr)) {
        (void)bss.mod_confusion(randint0(20) + 10);
    }

    if (!has_resist_chaos(player_ptr)) {
        (void)bss.mod_hallucination(randint1(10));
        if (one_in_(3)) {
            msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
            (void)gain_mutation(player_ptr, 0);
        }
    }
    if (!has_resist_neth(player_ptr) && !has_resist_chaos(player_ptr)) {
        drain_exp(player_ptr, 5000 + (player_ptr->exp / 100), 500 + (player_ptr->exp / 1000), 75);
    }

    if (!has_resist_chaos(player_ptr) || one_in_(9)) {
        inventory_damage(player_ptr, BreakerElec(), 2);
        inventory_damage(player_ptr, BreakerFire(), 2);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_shards(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_shards_damage_rate(player_ptr, CALC_RAND) / 100;

    if (!has_resist_shard(player_ptr) && !check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_cut(static_cast<TIME_EFFECT>(ep_ptr->dam));
    }

    if (!has_resist_shard(player_ptr) || one_in_(13)) {
        inventory_damage(player_ptr, BreakerCold(), 2);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_sound(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_sound_damage_rate(player_ptr, CALC_RAND) / 100;

    if (!has_resist_sound(player_ptr) && !check_multishadow(player_ptr)) {
        TIME_EFFECT plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
        (void)BadStatusSetter(player_ptr).mod_stun(plus_stun);
    }

    if (!has_resist_sound(player_ptr) || one_in_(13)) {
        inventory_damage(player_ptr, BreakerCold(), 2);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_confusion(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_conf_damage_rate(player_ptr, CALC_RAND) / 100;
    BadStatusSetter bss(player_ptr);
    if (!has_resist_conf(player_ptr) && !check_multishadow(player_ptr)) {
        (void)bss.mod_confusion(randint1(20) + 10);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_disenchant(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_disenchant_damage_rate(player_ptr, CALC_RAND) / 100;

    if (!has_resist_disen(player_ptr) && !check_multishadow(player_ptr)) {
        (void)apply_disenchant(player_ptr, 0);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_nexus(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_nexus_damage_rate(player_ptr, CALC_RAND) / 100;

    if (!has_resist_nexus(player_ptr) && !check_multishadow(player_ptr)) {
        apply_nexus(ep_ptr->m_ptr, player_ptr);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_force(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
    }
    if (!has_resist_sound(player_ptr) && !check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_stun(randint1(20));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_rocket(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("爆発があった！", "There is an explosion!"));
    }

    BadStatusSetter bss(player_ptr);
    if (!has_resist_sound(player_ptr) && !check_multishadow(player_ptr)) {
        (void)bss.mod_stun(randint1(20));
    }

    ep_ptr->dam = ep_ptr->dam * calc_rocket_damage_rate(player_ptr, CALC_RAND) / 100;
    if (!has_resist_shard(player_ptr) && !check_multishadow(player_ptr)) {
        (void)bss.mod_cut((ep_ptr->dam / 2));
    }

    if (!has_resist_shard(player_ptr) || one_in_(12)) {
        inventory_damage(player_ptr, BreakerCold(), 3);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_inertial(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
    }

    if (!check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_slowness(randint0(4) + 4, false);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_lite(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }
    if (!player_ptr->blind && !has_resist_lite(player_ptr) && !has_resist_blind(player_ptr) && !check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_blindness(randint1(5) + 2);
    }

    ep_ptr->dam = ep_ptr->dam * calc_lite_damage_rate(player_ptr, CALC_RAND) / 100;

    PlayerRace race(player_ptr);
    if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE)) {
        if (!check_multishadow(player_ptr)) {
            msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
        }
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!player_ptr->wraith_form || check_multishadow(player_ptr)) {
        return;
    }

    player_ptr->wraith_form = 0;
    msg_print(_("閃光のため非物質的な影の存在でいられなくなった。", "The light forces you out of your incorporeal shadow form."));

    player_ptr->redraw |= (PR_MAP | PR_STATUS);
    player_ptr->update |= (PU_MONSTERS);
    player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
}

void effect_player_dark(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_dark_damage_rate(player_ptr, CALC_RAND) / 100;

    auto go_blind = player_ptr->blind == 0;
    go_blind &= !has_resist_blind(player_ptr);
    go_blind &= !(has_resist_dark(player_ptr) || has_immune_dark(player_ptr));
    go_blind &= !check_multishadow(player_ptr);

    if (go_blind) {
        (void)BadStatusSetter(player_ptr).mod_blindness(randint1(5) + 2);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

static void effect_player_time_one_disability(PlayerType *player_ptr)
{
    int k = 0;
    concptr act = nullptr;
    switch (randint1(6)) {
    case 1:
        k = A_STR;
        act = _("強く", "strong");
        break;
    case 2:
        k = A_INT;
        act = _("聡明で", "bright");
        break;
    case 3:
        k = A_WIS;
        act = _("賢明で", "wise");
        break;
    case 4:
        k = A_DEX;
        act = _("器用で", "agile");
        break;
    case 5:
        k = A_CON;
        act = _("健康で", "hale");
        break;
    case 6:
        k = A_CHR;
        act = _("美しく", "beautiful");
        break;
    }

    msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act);
    player_ptr->stat_cur[k] = (player_ptr->stat_cur[k] * 3) / 4;
    if (player_ptr->stat_cur[k] < 3) {
        player_ptr->stat_cur[k] = 3;
    }

    player_ptr->update |= (PU_BONUS);
}

static void effect_player_time_all_disabilities(PlayerType *player_ptr)
{
    msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));
    for (int k = 0; k < A_MAX; k++) {
        player_ptr->stat_cur[k] = (player_ptr->stat_cur[k] * 7) / 8;
        if (player_ptr->stat_cur[k] < 3) {
            player_ptr->stat_cur[k] = 3;
        }
    }

    player_ptr->update |= (PU_BONUS);
}

static void effect_player_time_addition(PlayerType *player_ptr)
{
    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: {
        if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
            break;
        }

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(player_ptr, 100 + (player_ptr->exp / 100) * MON_DRAIN_LIFE);
        break;
    }
    case 6:
    case 7:
    case 8:
    case 9:
        effect_player_time_one_disability(player_ptr);
        break;
    case 10:
        effect_player_time_all_disabilities(player_ptr);
        break;
    }
}

/*!
 * @brief 時間逆転属性によるダメージを受ける
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 */
void effect_player_time(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_time_damage_rate(player_ptr, CALC_RAND) / 100;

    bool evaded = check_multishadow(player_ptr);

    if (has_resist_time(player_ptr) && !evaded) {
        msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!has_resist_time(player_ptr) && !evaded) {
        effect_player_time_addition(player_ptr);
    }
}

void effect_player_gravity(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
    }

    msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

    if (!check_multishadow(player_ptr)) {
        teleport_player(player_ptr, 5, TELEPORT_PASSIVE);
        BadStatusSetter bss(player_ptr);
        if (!player_ptr->levitation) {
            (void)bss.mod_slowness(randint0(4) + 4, false);
        }

        if (!(has_resist_sound(player_ptr) || player_ptr->levitation)) {
            TIME_EFFECT plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
            (void)bss.mod_stun(plus_stun);
        }
    }

    ep_ptr->dam = ep_ptr->dam * calc_gravity_damage_rate(player_ptr, CALC_RAND) / 100;
    if (!player_ptr->levitation || one_in_(13)) {
        inventory_damage(player_ptr, BreakerCold(), 2);
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_disintegration(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_death_ray(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));
    }

    ep_ptr->dam = ep_ptr->dam * calc_deathray_damage_rate(player_ptr, CALC_RAND) / 100;
    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_mana(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_psy_spear(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_FORCE, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_meteor(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (!has_resist_shard(player_ptr) || one_in_(13)) {
        if (!has_immune_fire(player_ptr)) {
            inventory_damage(player_ptr, BreakerFire(), 2);
        }
        inventory_damage(player_ptr, BreakerCold(), 2);
    }
}

void effect_player_icee(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if (player_ptr->blind) {
        msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));
    }

    ep_ptr->get_damage = cold_dam(player_ptr, ep_ptr->dam, ep_ptr->killer, false);
    if (check_multishadow(player_ptr)) {
        return;
    }

    BadStatusSetter bss(player_ptr);
    if (!has_resist_shard(player_ptr)) {
        (void)bss.mod_cut(damroll(5, 8));
    }

    if (!has_resist_sound(player_ptr)) {
        (void)bss.mod_stun(randint1(15));
    }

    if ((!(has_resist_cold(player_ptr) || is_oppose_cold(player_ptr))) || one_in_(12)) {
        if (!has_immune_cold(player_ptr)) {
            inventory_damage(player_ptr, BreakerCold(), 3);
        }
    }
}

void effect_player_hand_doom(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(player_ptr)) {
            msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
            curse_equipment(player_ptr, 40, 20);
        }

        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->m_name);

        if (player_ptr->chp < 1) {
            player_ptr->chp = 1;
        }
    }
}

void effect_player_void(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    auto effect_mes = player_ptr->blind ? _("何かに身体が引っ張りこまれる！", "Something absorbs you!")
                                        : _("周辺の空間が歪んだ。", "Sight warps around you.");
    msg_print(effect_mes);
    if (!check_multishadow(player_ptr) && !player_ptr->levitation && !player_ptr->anti_tele) {
        (void)BadStatusSetter(player_ptr).mod_slowness(randint0(4) + 4, false);
    }

    ep_ptr->dam = ep_ptr->dam * calc_void_damage_rate(player_ptr, CALC_RAND) / 100;
    if (!player_ptr->levitation || one_in_(13)) {
        inventory_damage(player_ptr, BreakerCold(), 2);
    }
    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_abyss(PlayerType *player_ptr, EffectPlayerType *ep_ptr)
{
    auto effect_mes = player_ptr->blind ? _("身体が沈み込む気がする！", "You feel you are sinking into something!")
                                        : _("深淵があなたを誘い込んでいる！", "You are falling into the abyss!");
    msg_print(effect_mes);
    ep_ptr->dam = ep_ptr->dam * calc_abyss_damage_rate(player_ptr, CALC_RAND) / 100;
    BadStatusSetter bss(player_ptr);
    if (check_multishadow(player_ptr)) {
        return;
    }

    if (!player_ptr->levitation) {
        (void)bss.mod_slowness(randint0(4) + 4, false);
    }

    if (player_ptr->blind) {
        return;
    }

    msg_print(_("深淵から何かがあなたを覗き込んでいる！", "Something gazes at you from the abyss!"));
    if (!has_resist_chaos(player_ptr)) {
        (void)bss.mod_hallucination(randint1(10));
    }

    if (!has_resist_conf(player_ptr)) {
        (void)bss.mod_confusion(randint1(10));
    }

    if (!has_resist_fear(player_ptr)) {
        (void)bss.mod_fear(randint1(10));
    }
    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}
