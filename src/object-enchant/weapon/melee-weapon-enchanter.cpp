/*!
 * @brief 剣・鈍器・戦斧武器に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/melee-weapon-enchanter.h"

MeleeWeaponEnchanter::MeleeWeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}
