#include "system/angband.h"
#include "combat/attack-accuracy.h"

/*!
 * @brief プレイヤーからモンスターへの打撃命中判定 /
 * Determine if the player "hits" a monster (normal combat).
 * @param chance 基本命中値
 * @param ac モンスターのAC
 * @param visible 目標を視界に捕らえているならばTRUEを指定
 * @return 命中と判定された場合TRUEを返す
 * @note Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible)
{
    if (!visible)
        chance = (chance + 1) / 2;
    return hit_chance(attacker_ptr, chance, ac) >= randint1(100);
}

/*!
 * @brief モンスターへの命中率の計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param to_h 命中値
 * @param ac 敵AC
 * @return 命中確率
 */
PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY reli, ARMOUR_CLASS ac)
{
    PERCENTAGE chance = 5, chance_left = 90;
    if (reli <= 0)
        return 5;
    if (attacker_ptr->pseikaku == PERSONALITY_LAZY)
        chance_left = (chance_left * 19 + 9) / 20;
    chance += (100 - ((ac * 75) / reli)) * chance_left / 100;
    if (chance < 5)
        chance = 5;
    return chance;
}
