#include "effect/effect-player-resist-hurt.h"
#include "art-definition/art-sword-types.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-damage.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/race-indice-types.h"
#include "mutation/mutation.h"
#include "object/object-broken.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "spell-kind/spells-equipment.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

// 毒を除く4元素.
void effect_player_elements(
    player_type *target_ptr, effect_player_type *ep_ptr, concptr attack_message, HIT_POINT (*damage_func)(player_type *, HIT_POINT, concptr, int, bool))
{
    if (target_ptr->blind)
        msg_print(attack_message);

    ep_ptr->get_damage = (*damage_func)(target_ptr, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell, FALSE);
}

void effect_player_poison(player_type *target_ptr, effect_player_type *ep_ptr)
{
    bool double_resist = is_oppose_pois(target_ptr);
    if (target_ptr->blind)
        msg_print(_("毒で攻撃された！", "You are hit by poison!"));

    if (target_ptr->resist_pois)
        ep_ptr->dam = (ep_ptr->dam + 2) / 3;
    if (double_resist)
        ep_ptr->dam = (ep_ptr->dam + 2) / 3;

    if ((!(double_resist || target_ptr->resist_pois)) && one_in_(HURT_CHANCE) && !check_multishadow(target_ptr)) {
        do_dec_stat(target_ptr, A_CON);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

    if (!(double_resist || target_ptr->resist_pois) && !check_multishadow(target_ptr))
        set_poisoned(target_ptr, target_ptr->poisoned + randint0(ep_ptr->dam) + 10);
}

void effect_player_nuke(player_type *target_ptr, effect_player_type *ep_ptr)
{
    bool double_resist = is_oppose_pois(target_ptr);
    if (target_ptr->blind)
        msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));

    if (target_ptr->resist_pois)
        ep_ptr->dam = (2 * ep_ptr->dam + 2) / 5;
    if (double_resist)
        ep_ptr->dam = (2 * ep_ptr->dam + 2) / 5;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
    if ((double_resist || target_ptr->resist_pois) || check_multishadow(target_ptr))
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

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_holy_fire(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    if (target_ptr->align > 10)
        ep_ptr->dam /= 2;
    else if (target_ptr->align < -10)
        ep_ptr->dam *= 2;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_hell_fire(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    if (target_ptr->align > 10)
        ep_ptr->dam *= 2;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_arrow(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind) {
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    if ((target_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU) || (target_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU)) {
        msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
        return;
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_plasma(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

    if (!target_ptr->resist_sound && !check_multishadow(target_ptr)) {
        int plus_stun = (randint1((ep_ptr->dam > 40) ? 35 : (ep_ptr->dam * 3 / 4 + 5)));
        (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
    }

    if (!(target_ptr->resist_fire || is_oppose_fire(target_ptr) || target_ptr->immune_fire))
        inventory_damage(target_ptr, set_acid_destroy, 3);
}

void effect_player_nether(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));

    if (target_ptr->resist_neth) {
        if (!is_specific_player_race(target_ptr, RACE_SPECTRE))
            ep_ptr->dam *= 6;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr))
        drain_exp(target_ptr, 200 + (target_ptr->exp / 100), 200 + (target_ptr->exp / 1000), 75);

    if (!is_specific_player_race(target_ptr, RACE_SPECTRE) || check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    msg_print(_("気分がよくなった。", "You feel invigorated!"));
    hp_player(target_ptr, ep_ptr->dam / 4);
    learn_spell(target_ptr, ep_ptr->monspell);
}

void effect_player_water(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    if (!target_ptr->resist_sound && !target_ptr->resist_water) {
        set_stun(target_ptr, target_ptr->stun + randint1(40));
    }
    if (!target_ptr->resist_conf && !target_ptr->resist_water) {
        set_confused(target_ptr, target_ptr->confused + randint1(5) + 5);
    }

    if (one_in_(5) && !target_ptr->resist_water) {
        inventory_damage(target_ptr, set_cold_destroy, 3);
    }

    if (target_ptr->resist_water)
        ep_ptr->get_damage /= 4;

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_chaos(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));
    if (target_ptr->resist_chaos) {
        ep_ptr->dam *= 6;
        ep_ptr->dam /= (randint1(4) + 7);
    }

    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    if (!target_ptr->resist_conf) {
        (void)set_confused(target_ptr, target_ptr->confused + randint0(20) + 10);
    }
    if (!target_ptr->resist_chaos) {
        (void)set_image(target_ptr, target_ptr->image + randint1(10));
        if (one_in_(3)) {
            msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
            (void)gain_mutation(target_ptr, 0);
        }
    }
    if (!target_ptr->resist_neth && !target_ptr->resist_chaos) {
        drain_exp(target_ptr, 5000 + (target_ptr->exp / 100), 500 + (target_ptr->exp / 1000), 75);
    }

    if (!target_ptr->resist_chaos || one_in_(9)) {
        inventory_damage(target_ptr, set_elec_destroy, 2);
        inventory_damage(target_ptr, set_fire_destroy, 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_shards(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
    if (target_ptr->resist_shard) {
        ep_ptr->dam *= 6;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr)) {
        (void)set_cut(target_ptr, target_ptr->cut + ep_ptr->dam);
    }

    if (!target_ptr->resist_shard || one_in_(13))
        inventory_damage(target_ptr, set_cold_destroy, 2);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_sound(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));
    if (target_ptr->resist_sound) {
        ep_ptr->dam *= 5;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr)) {
        int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
        (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
    }

    if (!target_ptr->resist_sound || one_in_(13))
        inventory_damage(target_ptr, set_cold_destroy, 2);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_confusion(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));
    if (target_ptr->resist_conf) {
        ep_ptr->dam *= 5;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr)) {
        (void)set_confused(target_ptr, target_ptr->confused + randint1(20) + 10);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_disenchant(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));
    if (target_ptr->resist_disen) {
        ep_ptr->dam *= 6;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr)) {
        (void)apply_disenchant(target_ptr, 0);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_nexus(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));
    if (target_ptr->resist_nexus) {
        ep_ptr->dam *= 6;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!check_multishadow(target_ptr)) {
        apply_nexus(ep_ptr->m_ptr, target_ptr);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_force(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
    if (!target_ptr->resist_sound && !check_multishadow(target_ptr)) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(20));
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_rocket(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("爆発があった！", "There is an explosion!"));
    if (!target_ptr->resist_sound && !check_multishadow(target_ptr)) {
        (void)set_stun(target_ptr, target_ptr->stun + randint1(20));
    }

    if (target_ptr->resist_shard) {
        ep_ptr->dam /= 2;
    } else if (!check_multishadow(target_ptr)) {
        (void)set_cut(target_ptr, target_ptr->cut + (ep_ptr->dam / 2));
    }

    if (!target_ptr->resist_shard || one_in_(12)) {
        inventory_damage(target_ptr, set_cold_destroy, 3);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_inertial(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
    if (!check_multishadow(target_ptr))
        (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_lite(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    if (target_ptr->resist_lite) {
        ep_ptr->dam *= 4;
        ep_ptr->dam /= (randint1(4) + 7);
    } else if (!target_ptr->blind && !target_ptr->resist_blind && !check_multishadow(target_ptr)) {
        (void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
    }

    if (is_specific_player_race(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE)) {
        if (!check_multishadow(target_ptr))
            msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
        ep_ptr->dam *= 2;
    } else if (is_specific_player_race(target_ptr, RACE_S_FAIRY)) {
        ep_ptr->dam = ep_ptr->dam * 4 / 3;
    }

    if (target_ptr->wraith_form)
        ep_ptr->dam *= 2;
    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);

    if (!target_ptr->wraith_form || check_multishadow(target_ptr))
        return;

    target_ptr->wraith_form = 0;
    msg_print(_("閃光のため非物質的な影の存在でいられなくなった。", "The light forces you out of your incorporeal shadow form."));

    target_ptr->redraw |= (PR_MAP | PR_STATUS);
    target_ptr->update |= (PU_MONSTERS);
    target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}

void effect_player_dark(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    if (target_ptr->resist_dark) {
        ep_ptr->dam *= 4;
        ep_ptr->dam /= (randint1(4) + 7);

        if (is_specific_player_race(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE) || target_ptr->wraith_form)
            ep_ptr->dam = 0;
    } else if (!target_ptr->blind && !target_ptr->resist_blind && !check_multishadow(target_ptr)) {
        (void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
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
        if (target_ptr->prace == RACE_ANDROID)
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

void effect_player_time(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));

    if (target_ptr->resist_time) {
        ep_ptr->dam *= 4;
        ep_ptr->dam /= (randint1(4) + 7);
        msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    effect_player_time_addition(target_ptr);
    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_gravity(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
    msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

    if (!check_multishadow(target_ptr)) {
        teleport_player(target_ptr, 5, TELEPORT_PASSIVE);
        if (!target_ptr->levitation)
            (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
        if (!(target_ptr->resist_sound || target_ptr->levitation)) {
            int plus_stun = (randint1((ep_ptr->dam > 90) ? 35 : (ep_ptr->dam / 3 + 5)));
            (void)set_stun(target_ptr, target_ptr->stun + plus_stun);
        }
    }

    if (target_ptr->levitation) {
        ep_ptr->dam = (ep_ptr->dam * 2) / 3;
    }

    if (!target_ptr->levitation || one_in_(13)) {
        inventory_damage(target_ptr, set_cold_destroy, 2);
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_disintegration(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}
