/*
 * @brief 重鎧に耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-hard-armor.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
HardArmorEnchanter::HardArmorEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : ArmorEnchanter{ player_ptr, o_ptr, level, power }
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void HardArmorEnchanter::apply_magic()
{
    if (this->power > 1) {
        this->give_ego_index();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}
