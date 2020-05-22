#include "system/angband.h"
#include "combat/combat-options-type.h"
#include "combat/attack-criticality.h"

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
