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

/*!
 * @brief モンスター打撃の命中を判定する /
 * Determine if a monster attack against the player succeeds.
 * @param power 打撃属性毎の基本命中値
 * @param level モンスターのレベル
 * @param stun モンスターの朦朧値
 * @return TRUEならば命中判定
 * @details
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 */
int check_hit_from_monster_to_player(player_type *target_ptr, int power, DEPTH level, int stun)
{
    int k = randint0(100);
    if (stun && one_in_(2))
        return FALSE;
    if (k < 10)
        return (k < 5);
    int i = (power + (level * 3));

    int ac = target_ptr->ac + target_ptr->to_a;
    if (target_ptr->special_attack & ATTACK_SUIKEN)
        ac += (target_ptr->lev * 2);

    if ((i > 0) && (randint1(i) > ((ac * 3) / 4)))
        return TRUE;
    return FALSE;
}

/*!
 * @brief モンスターから敵モンスターへの命中判定
 * @param power 打撃属性による基本命中値
 * @param level 攻撃側モンスターのレベル
 * @param ac 目標モンスターのAC
 * @param stun 攻撃側モンスターが朦朧状態ならTRUEを返す
 * @return 命中ならばTRUEを返す
 */
int check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun)
{
    int k = randint0(100);
    if (stun && one_in_(2))
        return FALSE;
    if (k < 10)
        return (k < 5);
    int i = (power + (level * 3));

    if ((i > 0) && (randint1(i) > ((ac * 3) / 4)))
        return TRUE;
    return FALSE;
}
