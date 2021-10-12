#include "combat/attack-accuracy.h"
#include "combat/combat-options-type.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "player-attack/player-attack-util.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "specific-object/death-scythe.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーからモンスターへの打撃命中判定 /
 * Determine if the player "hits" a monster (normal combat).
 * @param chance 基本命中値
 * @param ac モンスターのAC
 * @param visible 目標を視界に捕らえているならばTRUEを指定
 * @return 命中と判定された場合TRUEを返す
 * @note Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_norm(player_type *player_ptr, int chance, ARMOUR_CLASS ac, bool visible)
{
    if (!visible)
        chance = (chance + 1) / 2;
    return hit_chance(player_ptr, chance, ac) >= randint1(100);
}

/*!
 * @brief モンスターへの命中率の計算
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param to_h 命中値
 * @param ac 敵AC
 * @return 命中確率
 */
PERCENTAGE hit_chance(player_type *player_ptr, int reli, ARMOUR_CLASS ac)
{
    PERCENTAGE chance = 5, chance_left = 90;
    if (reli <= 0)
        return 5;
    if (player_ptr->ppersonality == PERSONALITY_LAZY)
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
bool check_hit_from_monster_to_player(player_type *player_ptr, int power, DEPTH level, int stun)
{
    int k = randint0(100);
    if (stun && one_in_(2))
        return false;
    if (k < 10)
        return (k < 5);
    int i = (power + (level * 3));

    int ac = player_ptr->ac + player_ptr->to_a;
    if (player_ptr->special_attack & ATTACK_SUIKEN)
        ac += (player_ptr->lev * 2);

    if ((i > 0) && (randint1(i) > ((ac * 3) / 4)))
        return true;
    return false;
}

/*!
 * @brief モンスターから敵モンスターへの命中判定
 * @param power 打撃属性による基本命中値
 * @param level 攻撃側モンスターのレベル
 * @param ac 目標モンスターのAC
 * @param stun 攻撃側モンスターが朦朧状態ならTRUEを返す
 * @return 命中ならばTRUEを返す
 */
bool check_hit_from_monster_to_monster(int power, DEPTH level, ARMOUR_CLASS ac, int stun)
{
    int k = randint0(100);
    if (stun && one_in_(2))
        return false;
    if (k < 10)
        return (k < 5);
    int i = (power + (level * 3));

    if ((i > 0) && (randint1(i) > ((ac * 3) / 4)))
        return true;
    return false;
}

/*!
 * @brief 攻撃が当たるかどうかを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param chance 基本命中値
 */
static bool decide_attack_hit(player_type *player_ptr, player_attack_type *pa_ptr, int chance)
{
    bool success_hit = false;
    object_type *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (((o_ptr->tval == ItemKindType::SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) || (pa_ptr->mode == HISSATSU_KYUSHO)) {
        int n = 1;

        if (can_attack_with_main_hand(player_ptr) && can_attack_with_sub_hand(player_ptr))
            n *= 2;

        if (pa_ptr->mode == HISSATSU_3DAN)
            n *= 2;

        success_hit = one_in_(n);
    } else if ((player_ptr->pclass == CLASS_NINJA) && ((pa_ptr->backstab || pa_ptr->surprise_attack) && !(r_ptr->flagsr & RFR_RES_ALL)))
        success_hit = true;
    else
        success_hit = test_hit_norm(player_ptr, chance, r_ptr->ac, pa_ptr->m_ptr->ml);

    if ((pa_ptr->mode == HISSATSU_MAJIN) && one_in_(2))
        success_hit = false;

    return success_hit;
}

/*!
 * @brief 直接攻撃の命中を処理するメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param chance 基本命中値
 * @return 当たればTRUE、外れればFALSE
 */
bool process_attack_hit(player_type *player_ptr, player_attack_type *pa_ptr, int chance)
{
    object_type *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    if (decide_attack_hit(player_ptr, pa_ptr, chance))
        return true;

    pa_ptr->backstab = false; /* Clumsy! */
    pa_ptr->surprise_attack = false; /* Clumsy! */

    if ((o_ptr->tval == ItemKindType::POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE) && one_in_(3)) {
        process_death_scythe_reflection(player_ptr, pa_ptr);
    } else {
        sound(SOUND_MISS);
        msg_format(_("ミス！ %sにかわされた。", "You miss %s."), pa_ptr->m_name);
    }

    return false;
}
