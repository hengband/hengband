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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
ArmorEnchanter::ArmorEnchanter(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , player_ptr(player_ptr)
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void ArmorEnchanter::apply_magic()
{
    if (this->power == 0) {
        return;
    }

    switch (this->o_ptr->tval) {
    case ItemKindType::DRAG_ARMOR:
        if (one_in_(50) || (this->power > 2)) {
            become_random_artifact(this->player_ptr, this->o_ptr, false);
        }

        break;
    case ItemKindType::HARD_ARMOR:
        if (this->power > 1) {
            this->give_ego_index();
            return;
        }

        if (this->power < -1) {
            this->give_cursed();
        }

        return;
    case ItemKindType::SOFT_ARMOR: {
        if (this->o_ptr->sval == SV_KUROSHOUZOKU) {
            this->o_ptr->pval = randint1(4);
        }

        if (this->power > 1) {
            this->give_high_ego_index();
            if (this->is_high_ego_generated) {
                return;
            }

            this->give_ego_index();
            return;
        }
        
        if (this->power < -1) {
            this->give_cursed();
        }

        return;
    }
    default:
        return;
    }
}

/*
 * @details power > 2はデバッグ専用.
 */
void ArmorEnchanter::give_ego_index()
{
    if (one_in_(20) || (this->power > 2)) {
        become_random_artifact(this->player_ptr, this->o_ptr, false);
        return;
    }

    while (true) {
        auto valid = true;
        this->o_ptr->name2 = get_random_ego(INVEN_BODY, true);
        switch (this->o_ptr->name2) {
        case EGO_DWARVEN:
            if (this->o_ptr->tval != ItemKindType::HARD_ARMOR) {
                valid = false;
            }

            break;
        case EGO_DRUID:
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

/*
 * @brief ベースアイテムがローブの時、確率で永続か宵闇のローブを生成する.
 * @return 生成条件を満たしたらtrue、満たさなかったらfalse
 * @details 永続：12%、宵闇：3%
 */
void ArmorEnchanter::give_high_ego_index()
{
    if ((this->o_ptr->sval != SV_ROBE) || (randint0(100) >= 15)) {
        return;
    }

    this->is_high_ego_generated = true;
    auto ego_robe = one_in_(5);
    this->o_ptr->name2 = ego_robe ? EGO_TWILIGHT : EGO_PERMANENCE;
    if (!ego_robe) {
        return;
    }

    this->o_ptr->k_idx = lookup_kind(ItemKindType::SOFT_ARMOR, SV_TWILIGHT_ROBE);
    this->o_ptr->sval = SV_TWILIGHT_ROBE;
    this->o_ptr->ac = 0;
    this->o_ptr->to_a = 0;
    return;
}

void ArmorEnchanter::give_cursed()
{
    this->o_ptr->name2 = get_random_ego(INVEN_BODY, false);
    switch (this->o_ptr->name2) {
    case EGO_A_DEMON:
    case EGO_A_MORGUL:
        return;
    default:
        msg_print(_("エラー：適した呪い鎧エゴがみつかりませんでした.", "Error:  suitable cursed armor ego not found."));
        return;
    }
}
