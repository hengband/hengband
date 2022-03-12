/*
 * @brief 鎧類に耐性等の追加効果を付与する基底処理
 * @date 2022/03/12
 * @author Hourier
 * @details 重鎧と軽鎧に適用. ドラゴン・スケイルメイルは対象外.
 */

#include "object-enchant/protector/apply-magic-armor.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-kind-hook.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
ArmorEnchanter::ArmorEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*
 * @brief 重鎧/軽鎧共通のエゴ付与処理
 * @details power > 2はデバッグ専用.
 */
void ArmorEnchanter::give_ego_index()
{
    if ((this->power > 2) || one_in_(20)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    while (true) {
        auto valid = true;
        this->o_ptr->ego_idx = get_random_ego(INVEN_BODY, true);
        switch (this->o_ptr->ego_idx) {
        case EgoType::DWARVEN:
            if (this->o_ptr->tval != ItemKindType::HARD_ARMOR) {
                valid = false;
            }

            break;
        case EgoType::DRUID:
            if (this->o_ptr->tval != ItemKindType::SOFT_ARMOR) {
                valid = false;
            }

            break;
        default:
            break;
        }

        if (valid) {
            break;
        }
    }
}

/*!
 * @brief 重鎧/軽鎧共通の呪い付与処理
 */
void ArmorEnchanter::give_cursed()
{
    this->o_ptr->ego_idx = get_random_ego(INVEN_BODY, false);
    switch (this->o_ptr->ego_idx) {
    case EgoType::A_DEMON:
    case EgoType::A_MORGUL:
        return;
    default:
        msg_print(_("エラー：適した呪い鎧エゴがみつかりませんでした.", "Error:  suitable cursed armor ego not found."));
        return;
    }
}
