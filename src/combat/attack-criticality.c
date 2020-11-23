#include "combat/attack-criticality.h"
#include "combat/combat-options-type.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "sv-definition/sv-weapon-types.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーからモンスターへの打撃クリティカル判定 /
 * Critical hits (by player) Factor in weapon weight, total plusses, player melee bonus
 * @param weight 矢弾の重量
 * @param plus 武器の命中修正
 * @param dam 現在算出中のダメージ値
 * @param meichuu 打撃の基本命中力
 * @param mode オプションフラグ
 * @return クリティカル修正が入ったダメージ値
 */
HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, combat_options mode)
{
    /* Extract "blow" power */
    int i = (weight + (meichuu * 3 + plus * 5) + attacker_ptr->skill_thn);

    /* Chance */
    bool is_special_option = randint1((attacker_ptr->pclass == CLASS_NINJA) ? 4444 : 5000) <= i;
    is_special_option |= mode == HISSATSU_MAJIN;
    is_special_option |= mode == HISSATSU_3DAN;
    if (!is_special_option)
        return dam;

    int k = weight + randint1(650);
    if ((mode == HISSATSU_MAJIN) || (mode == HISSATSU_3DAN))
        k += randint1(650);

    if (k < 400) {
        msg_print(_("手ごたえがあった！", "It was a good hit!"));

        dam = 2 * dam + 5;
        return dam;
    }

    if (k < 700) {
        msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
        dam = 2 * dam + 10;
        return dam;
    }

    if (k < 900) {
        msg_print(_("会心の一撃だ！", "It was a superb hit!"));
        dam = 3 * dam + 15;
        return dam;
    }

    if (k < 1300) {
        msg_print(_("最高の会心の一撃だ！", "It was a *GREAT* hit!"));
        dam = 3 * dam + 20;
        return dam;
    }

    msg_print(_("比類なき最高の会心の一撃だ！", "It was a *SUPERB* hit!"));
    dam = ((7 * dam) / 2) + 25;
    return dam;
}

/*!
 * @brief モンスター打撃のクリティカルランクを返す /
 * Critical blow. All hits that do 95% of total possible damage,
 * @param dice モンスター打撃のダイス数
 * @param sides モンスター打撃の最大ダイス目
 * @param dam プレイヤーに与えたダメージ
 * @details
 * and which also do at least 20 damage, or, sometimes, N damage.
 * This is used only to determine "cuts" and "stuns".
 */
int calc_monster_critical(DICE_NUMBER dice, DICE_SID sides, HIT_POINT dam)
{
    int total = dice * sides;
    if (dam < total * 19 / 20)
        return 0;

    if ((dam < 20) && (randint0(100) >= dam))
        return 0;

    int max = 0;
    if ((dam >= total) && (dam >= 40))
        max++;

    if (dam >= 20)
        while (randint0(100) < 2)
            max++;

    if (dam > 45)
        return (6 + max);

    if (dam > 33)
        return (5 + max);

    if (dam > 25)
        return (4 + max);

    if (dam > 18)
        return (3 + max);

    if (dam > 11)
        return (2 + max);

    return (1 + max);
}

/*!
 * todo 3つの処理をdetailsに書くよりは関数自体を分割すべきだが、一旦後回しにする。他の項目と一緒に処理する
 * @brief 忍者ヒットで急所を突く
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 * @details 闇討ち＆追討ちを実施した後に致命傷チェックを行う
 * チェックを通ったら、ユニークならば2倍ダメージ、それ以外は一撃死
 */
static void ninja_critical(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    int maxhp = pa_ptr->m_ptr->maxhp;
    if (one_in_(pa_ptr->backstab ? 13 : (pa_ptr->stab_fleeing || pa_ptr->surprise_attack) ? 15 : 27)) {
        pa_ptr->attack_damage *= 5;
        pa_ptr->drain_result *= 2;
        msg_format(_("刃が%sに深々と突き刺さった！", "You critically injured %s!"), pa_ptr->m_name);
        return;
    }

    bool is_weaken = pa_ptr->m_ptr->hp < maxhp / 2;
    bool is_unique = ((r_ptr->flags1 & RF1_UNIQUE) != 0) || ((r_ptr->flags7 & RF7_UNIQUE2) != 0);
    bool is_critical = (is_weaken && one_in_((attacker_ptr->num_blow[0] + attacker_ptr->num_blow[1] + 1) * 10))
        || ((one_in_(666) || ((pa_ptr->backstab || pa_ptr->surprise_attack) && one_in_(11))) && !is_unique);
    if (!is_critical)
        return;

    if (is_unique || !is_weaken) {
        pa_ptr->attack_damage = MAX(pa_ptr->attack_damage * 5, pa_ptr->m_ptr->hp / 2);
        pa_ptr->drain_result *= 2;
        msg_format(_("%sに致命傷を負わせた！", "You fatally injured %s!"), pa_ptr->m_name);
    } else {
        pa_ptr->attack_damage = pa_ptr->m_ptr->hp + 1;
        msg_format(_("刃が%sの急所を貫いた！", "You hit %s on a fatal spot!"), pa_ptr->m_name);
    }
}

/*!
 * @brief 急所を突く
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void critical_attack(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) || (pa_ptr->mode == HISSATSU_KYUSHO)) {
        if ((randint1(randint1(r_ptr->level / 7) + 5) == 1) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_UNIQUE2)) {
            pa_ptr->attack_damage = pa_ptr->m_ptr->hp + 1;
            msg_format(_("%sの急所を突き刺した！", "You hit %s on a fatal spot!"), pa_ptr->m_name);
        } else
            pa_ptr->attack_damage = 1;

        return;
    }

    bool is_ninja_hit = (attacker_ptr->pclass == CLASS_NINJA) && has_melee_weapon(attacker_ptr, INVEN_RARM + pa_ptr->hand)
        && !attacker_ptr->icky_wield[pa_ptr->hand] && ((attacker_ptr->cur_lite <= 0) || one_in_(7));
    if (is_ninja_hit)
        ninja_critical(attacker_ptr, pa_ptr);
}
