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
