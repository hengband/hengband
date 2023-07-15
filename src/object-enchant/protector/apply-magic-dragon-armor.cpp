/*
 * @brief ドラゴン・スケイルメイルに耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-dragon-armor.h"
#include "artifact/random-art-generator.h"
#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
DragonArmorEnchanter::DragonArmorEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void DragonArmorEnchanter::apply_magic()
{
    if ((this->power > 2) || one_in_(50)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
    }
}
