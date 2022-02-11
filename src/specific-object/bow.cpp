#include "specific-object/bow.h"
#include "object/tval-types.h"
#include "sv-definition/sv-bow-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 射撃武器に対応する矢/弾薬のベースアイテムIDを返す /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 対応する矢/弾薬のベースアイテムID
 */
ItemKindType bow_tval_ammo(ObjectType *o_ptr)
{
    switch (o_ptr->sval) {
    case SV_SLING:
        return ItemKindType::SHOT;
    case SV_SHORT_BOW:
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW:
        return ItemKindType::ARROW;
    case SV_LIGHT_XBOW:
    case SV_HEAVY_XBOW:
        return ItemKindType::BOLT;
    case SV_CRIMSON:
    case SV_HARP:
        return ItemKindType::NO_AMMO;
    }

    return ItemKindType::NONE;
}
