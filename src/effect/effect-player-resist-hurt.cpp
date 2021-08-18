#include "effect/effect-player-resist-hurt.h"
#include "artifact/fixed-art-types.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-player-util.h"
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
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
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
    player_type *target_ptr, effect_player_type *ep_ptr, concptr attack_message, HIT_POINT (*damage_func)(player_type *, HIT_POINT, concptr, bool))
{
    if (target_ptr->blind)
        msg_print(attack_message);

    ep_ptr->get_damage = (*damage_func)(target_ptr, ep_ptr->dam, ep_ptr->killer, false);
}

void effect_player_poison(player_type *target_ptr, effect_player_type *ep_ptr)
{
    bool double_resist = is_oppose_pois(target_ptr);
    if (target_ptr->blind)
        msg_print(_("毒で攻撃された！", "You are hit by poison!"));

    ep_ptr->dam = ep_ptr->dam * calc_pois_damage_rate(target_ptr) / 100;

    if ((!(double_resist || has_resist_pois(target_ptr))) && one_in_(HURT_CHANCE) && !check_multishadow(target_ptr)) {
        do_dec_stat(target_ptr, A_CON);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!(double_resist || has_resist_pois(target_ptr)) && !check_multishadow(target_ptr))
        set_poisoned(target_ptr, target_ptr->poisoned + randint0(ep_ptr->dam) + 10);
}

void effect_player_nuke(player_type *target_ptr, effect_player_type *ep_ptr)
{
    bool double_resist = is_oppose_pois(target_ptr);
    if (target_ptr->blind)
        msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));

    ep_ptr->dam = ep_ptr->dam * calc_pois_damage_rate(target_ptr) / 100;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if ((double_resist || has_resist_pois(target_ptr)) || check_multishadow(target_ptr))
        return;

    set_poisoned(target_ptr, target_ptr->poisoned + randint0(ep_ptr->dam) + 10);
    if (one_in_(5)) /* 6 */
    {
        msg_print(_("奇形的な変身を遂げた！", "You undergo a freakish metamorphosis!"));
        if (one_in_(4)) /* 4 */
            do_poly_self(target_ptr);
        else
            status_shuffle(target_ptr);
    }

    if (one_in_(6))
        inventory_damage(target_ptr, set_acid_destroy, 2);
}

void effect_player_missile(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_holy_fire(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    ep_ptr->dam = ep_ptr->dam * calc_holy_fire_damage_rate(target_ptr, CALC_RAND) / 100;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_hell_fire(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    ep_ptr->dam = ep_ptr->dam * calc_hell_fire_damage_rate(target_ptr, CALC_RAND) / 100;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_arrow(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind) {
        sound(SOUND_SHOOT_HIT);
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    if (has_invuln_arrow(target_ptr)) {
        msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
        return;
    }

    sound(SOUND_SHOOT_HIT);
    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_plasma(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!has_resist_sound(target_ptr) && !check_multishadow(target_ptr)) {
        int plus_stun = (randint1((ep_ptr->dam > 40) ? 35 : (ep_ptr->dam * 3 / 4 + 5)));
        (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
    }

    if (!(has_resist_fire(target_ptr) || is_oppose_fire(target_ptr) || has_immune_fire(target_ptr)))
        inventory_damage(target_ptr, set_acid_destroy, 3);
}

/*!
 * @brief 地獄属性によるダメージを受ける
 * @param target_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 * @details
 * 幽霊は回復する。追加効果で経験値吸収。
 */

void effect_player_nether(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));

    bool evaded = check_multishadow(target_ptr);

    if (is_specific_player_race(target_ptr, player_race_type::SPECTRE)) {
        if (!evaded) {
            msg_print(_("気分がよくなった。", "You feel invigorated!"));
            hp_player(target_ptr, ep_ptr->dam / 4);
        }
        ep_ptr->get_damage = 0;
        return;
    }

    ep_ptr->dam = ep_ptr->dam * calc_nether_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_neth(target_ptr) && !evaded)
        drain_exp(target_ptr, 200 + (target_ptr->exp / 100), 200 + (target_ptr->exp / 1000), 75);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

/*!
 * @brief 水流属性によるダメージを受ける
 * @param target_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 * @details
 * 追加効果で朦朧と混乱、冷気同様のインベントリ破壊。
 */
void effect_player_water(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    ep_ptr->dam = ep_ptr->dam * calc_water_damage_rate(target_ptr, CALC_RAND) / 100;

    BIT_FLAGS has_res_water = has_resist_water(target_ptr);

    if (!check_multishadow(target_ptr)) {
        if (!has_resist_sound(target_ptr) && !has_res_water) {
            set_stun(target_ptr, target_ptr->stun + randint1(40));
        }
        if (!has_resist_conf(target_ptr) && !has_res_water) {
            set_confused(target_ptr, target_ptr->confused + randint1(5) + 5);
        }

        if (one_in_(5) && !has_res_water) {
            inventory_damage(target_ptr, set_cold_destroy, 3);
        }
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_chaos(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));

    ep_ptr->dam = ep_ptr->dam * calc_chaos_damage_rate(target_ptr, CALC_RAND) / 100;

    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    if (!has_resist_conf(target_ptr)) {
        (void)set_confused(target_ptr, target_ptr->confused + randint0(20) + 10);
    }
    if (!has_resist_chaos(target_ptr)) {
        (void)set_image(target_ptr, target_ptr->image + randint1(10));
        if (one_in_(3)) {
            msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
            (void)gain_mutation(target_ptr, 0);
        }
    }
    if (!has_resist_neth(target_ptr) && !has_resist_chaos(target_ptr)) {
        drain_exp(target_ptr, 5000 + (target_ptr->exp / 100), 500 + (target_ptr->exp / 1000), 75);
    }

    if (!has_resist_chaos(target_ptr) || one_in_(9)) {
        inventory_damage(target_ptr, set_elec_destroy, 2);
        inventory_damage(target_ptr, set_fire_destroy, 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_shards(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));

    ep_ptr->dam = ep_ptr->dam * calc_shards_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_shard(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_cut(target_ptr, target_ptr->cut + ep_ptr->dam);
    }

    if (!has_resist_shard(target_ptr) || one_in_(13))
        inventory_damage(target_ptr, set_cold_destroy, 2);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_sound(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));

    ep_ptr->dam = ep_ptr->dam * calc_sound_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_sound(target_ptr) && !check_multishadow(target_ptr)) {
        int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
        (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
    }

    if (!has_resist_sound(target_ptr) || one_in_(13))
        inventory_damage(target_ptr, set_cold_destroy, 2);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_confusion(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));

    ep_ptr->dam = ep_ptr->dam * calc_conf_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_conf(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_confused(target_ptr, target_ptr->confused + randint1(20) + 10);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_disenchant(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));

    ep_ptr->dam = ep_ptr->dam * calc_disenchant_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_disen(target_ptr) && !check_multishadow(target_ptr)) {
        (void)apply_disenchant(target_ptr, 0);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_nexus(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));

    ep_ptr->dam = ep_ptr->dam * calc_nexus_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_nexus(target_ptr) && !check_multishadow(target_ptr)) {
        apply_nexus(ep_ptr->m_ptr, target_ptr);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_force(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
    if (!has_resist_sound(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(20));
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_rocket(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("爆発があった！", "There is an explosion!"));
    if (!has_resist_sound(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(20));
    }

    ep_ptr->dam = ep_ptr->dam * calc_rocket_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!has_resist_shard(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_cut(target_ptr, target_ptr->cut + (ep_ptr->dam / 2));
    }

    if (!has_resist_shard(target_ptr) || one_in_(12)) {
        inventory_damage(target_ptr, set_cold_destroy, 3);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_inertial(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
    if (!check_multishadow(target_ptr))
        (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, false);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_lite(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    if (!target_ptr->blind && !has_resist_lite(target_ptr) && !has_resist_blind(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
    }

    ep_ptr->dam = ep_ptr->dam * calc_lite_damage_rate(target_ptr, CALC_RAND) / 100;

    if (player_race_life(target_ptr) == PlayerRaceLife::UNDEAD && player_race_has_flag(target_ptr, TR_VUL_LITE)) {
        if (!check_multishadow(target_ptr))
            msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!target_ptr->wraith_form || check_multishadow(target_ptr))
        return;

    target_ptr->wraith_form = 0;
    msg_print(_("閃光のため非物質的な影の存在でいられなくなった。", "The light forces you out of your incorporeal shadow form."));

    target_ptr->redraw |= (PR_MAP | PR_STATUS);
    target_ptr->update |= (PU_MONSTERS);
    target_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
}

void effect_player_dark(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    ep_ptr->dam = ep_ptr->dam * calc_dark_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!target_ptr->blind && !has_resist_dark(target_ptr) && !has_resist_blind(target_ptr) && !check_multishadow(target_ptr)) {
        (void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

static void effect_player_time_one_disability(player_type *target_ptr)
{
    int k = 0;
    concptr act = NULL;
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
    target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 3) / 4;
    if (target_ptr->stat_cur[k] < 3)
        target_ptr->stat_cur[k] = 3;

    target_ptr->update |= (PU_BONUS);
}

static void effect_player_time_all_disabilities(player_type *target_ptr)
{
    msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));
    for (int k = 0; k < A_MAX; k++) {
        target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 7) / 8;
        if (target_ptr->stat_cur[k] < 3)
            target_ptr->stat_cur[k] = 3;
    }

    target_ptr->update |= (PU_BONUS);
}

static void effect_player_time_addition(player_type *target_ptr)
{
    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: {
        if (target_ptr->prace == player_race_type::ANDROID)
            break;

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
        break;
    }
    case 6:
    case 7:
    case 8:
    case 9:
        effect_player_time_one_disability(target_ptr);
        break;
    case 10:
        effect_player_time_all_disabilities(target_ptr);
        break;
    }
}

/*!
 * @brief 時間逆転属性によるダメージを受ける
 * @param target_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr プレイヤー効果情報への参照ポインタ
 */
void effect_player_time(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));

    ep_ptr->dam = ep_ptr->dam * calc_time_damage_rate(target_ptr, CALC_RAND) / 100;

    bool evaded = check_multishadow(target_ptr);

    if (has_resist_time(target_ptr) && !evaded)
        msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);

    if (!has_resist_time(target_ptr) && !evaded)
        effect_player_time_addition(target_ptr);
}

void effect_player_gravity(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
    msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

    if (!check_multishadow(target_ptr)) {
        teleport_player(target_ptr, 5, TELEPORT_PASSIVE);
        if (!target_ptr->levitation)
            (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, false);
        if (!(has_resist_sound(target_ptr) || target_ptr->levitation)) {
            int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
            (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
        }
    }

    ep_ptr->dam = ep_ptr->dam * calc_gravity_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!target_ptr->levitation || one_in_(13)) {
        inventory_damage(target_ptr, set_cold_destroy, 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_disintegration(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_death_ray(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));

    ep_ptr->dam = ep_ptr->dam * calc_deathray_damage_rate(target_ptr, CALC_RAND) / 100;
    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_mana(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_psy_spear(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_FORCE, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_meteor(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (!has_resist_shard(target_ptr) || one_in_(13)) {
        if (!has_immune_fire(target_ptr))
            inventory_damage(target_ptr, set_fire_destroy, 2);
        inventory_damage(target_ptr, set_cold_destroy, 2);
    }
}

void effect_player_icee(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));

    ep_ptr->get_damage = cold_dam(target_ptr, ep_ptr->dam, ep_ptr->killer, false);
    if (check_multishadow(target_ptr))
        return;

    if (!has_resist_shard(target_ptr)) {
        (void)set_cut(target_ptr, target_ptr->cut + damroll(5, 8));
    }

    if (!has_resist_sound(target_ptr)) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(15));
    }

    if ((!(has_resist_cold(target_ptr) || is_oppose_cold(target_ptr))) || one_in_(12)) {
        if (!has_immune_cold(target_ptr))
            inventory_damage(target_ptr, set_cold_destroy, 3);
    }
}

void effect_player_hand_doom(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < target_ptr->skill_sav) && !check_multishadow(target_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(target_ptr)) {
            msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
            curse_equipment(target_ptr, 40, 20);
        }

        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->m_name);

        if (target_ptr->chp < 1)
            target_ptr->chp = 1;
    }
}

void effect_player_void(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かに身体が引っ張りこまれる！", "Something absorbs you!"));
    else
        msg_print(_("周辺の空間が歪んだ。", "Sight warps around you."));

    if (!check_multishadow(target_ptr)) {
        if (!target_ptr->levitation && !target_ptr->anti_tele)
            (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, false);
    }

    ep_ptr->dam = ep_ptr->dam * calc_void_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!target_ptr->levitation || one_in_(13)) {
        inventory_damage(target_ptr, set_cold_destroy, 2);
    }
}

void effect_player_abyss(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("身体が沈み込む気がする！", "You feel you are sinking into something!"));
    else
        msg_print(_("深淵があなたを誘い込んでいる！", "You are falling in abyss!"));

    ep_ptr->dam = ep_ptr->dam * calc_abyss_damage_rate(target_ptr, CALC_RAND) / 100;

    if (!check_multishadow(target_ptr)) {
        if (!target_ptr->levitation)
            (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, false);

        if (!target_ptr->blind) {
            msg_print(_("深淵から何かがあなたを覗き込んでいる！", "Something gazes you from abyss!"));

            if (!has_resist_chaos(target_ptr))
                (void)set_image(target_ptr, target_ptr->image + randint1(10));

            if (!has_resist_conf(target_ptr))
                (void)set_confused(target_ptr, target_ptr->confused + randint1(10));

            if (!has_resist_fear(target_ptr))
                (void)set_afraid(target_ptr, target_ptr->afraid + randint1(10));
        }
    }
}
