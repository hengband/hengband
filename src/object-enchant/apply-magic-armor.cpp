/*
 * @brief 鎧類に耐性等の追加効果を付与する処理
 * @date 2021/08/01
 * @author Hourier
 */

#include "object-enchant/apply-magic-armor.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-armor-types.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*
 * @brief コンストラクタ
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
ArmorEnchanter::ArmorEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , owner_ptr(owner_ptr)
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void ArmorEnchanter::apply_magic()
{
    switch (this->o_ptr->tval) {
    case TV_DRAG_ARMOR:
        if (one_in_(50) || (this->power > 2)) {
            become_random_artifact(this->owner_ptr, this->o_ptr, false);
        }

        break;
    case TV_HARD_ARMOR:
        if (this->power < -1) {
            this->give_cursed();
        }

        break;
    case TV_SOFT_ARMOR: {
        if (this->power > 1) {
            this->try_generate_twilight_robe();
            break;
        }
        
        if (this->power < -1) {
            this->give_cursed();
        }

        break;
    }
    default:
        break;
    }
}

/*
 * @details power > 2はデバッグ専用.
 */
void ArmorEnchanter::give_ego_index()
{
    if (one_in_(20) || (this->power > 2)) {
        become_random_artifact(this->owner_ptr, this->o_ptr, false);
        return;
    }

    while (true) {
        auto valid = true;
        this->o_ptr->name2 = get_random_ego(INVEN_BODY, true);
        switch (this->o_ptr->name2) {
        case EGO_DWARVEN:
            if (this->o_ptr->tval != TV_HARD_ARMOR) {
                valid = false;
            }

            break;
        case EGO_DRUID:
            if (this->o_ptr->tval != TV_SOFT_ARMOR) {
                valid = false;
            }

            break;
        default:
            break;
        }

        if (valid)
            break;
    }
}

void ArmorEnchanter::give_cursed()
{
    while (true) {
        this->o_ptr->name2 = get_random_ego(INVEN_BODY, false);
        switch (this->o_ptr->name2) {
        case EGO_A_DEMON:
        case EGO_A_MORGUL:
            return;
        default:
            msg_print(_("エラー：適した呪い鎧エゴがみつかりませんでした.", "Error:Suitable cursed armor ego not found."));
            return;
        }
    }
}

/*
 * @brief ベースアイテムがローブの時、15%の確率で宵闇のローブを生成する.
 */
void ArmorEnchanter::try_generate_twilight_robe()
{
    if ((this->o_ptr->sval != SV_ROBE) || (randint0(100) >= 15)) {
        return;
    }

    if (!one_in_(5)) {
        this->o_ptr->name2 = EGO_PERMANENCE;
        return;
    }

    this->o_ptr->name2 = EGO_TWILIGHT;
    this->o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_TWILIGHT_ROBE);
    this->o_ptr->sval = SV_TWILIGHT_ROBE;
    this->o_ptr->ac = 0;
    this->o_ptr->to_a = 0;
}
