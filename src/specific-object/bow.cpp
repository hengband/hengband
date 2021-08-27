#include "specific-object/bow.h"
#include "sv-definition/sv-bow-types.h"
#include "system/object-type-definition.h"

/*!
 * @brief 射撃武器に対応する矢/弾薬のベースアイテムIDを返す /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 対応する矢/弾薬のベースアイテムID
 */
tval_type bow_tval_ammo(object_type *o_ptr)
{
    switch (o_ptr->sval) {
    case SV_SLING: {
        return TV_SHOT;
    }

    case SV_SHORT_BOW:
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW: {
        return TV_ARROW;
    }

    case SV_LIGHT_XBOW:
    case SV_HEAVY_XBOW: {
        return TV_BOLT;
    }
    case SV_CRIMSON:
    case SV_HARP: {
        return TV_NO_AMMO;
    }
    }

    return TV_NONE;
}
