/*!
 * @brief 剣・鈍器・戦斧に耐性等の追加効果を付与する処理
 * @date 2022/03/11
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-sword.h"

/*!
 * @brief 剣・鈍器・戦斧強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
SwordEnchanter::SwordEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}
