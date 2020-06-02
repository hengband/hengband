#include "object/torch.h"
#include "sv-definition/sv-lite-types.h"
#include "object-enchant/tr-types.h"

/*!
 * @brief 投擲時たいまつに投げやすい/焼棄/アンデッドスレイの特別効果を返す。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param flgs 特別に追加するフラグを返す参照ポインタ
 * @return なし
 */
void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs)
{
    if ((o_ptr->tval != TV_LITE) || (o_ptr->sval != SV_LITE_TORCH))
        return;
    if (o_ptr->xtra4 <= 0)
        return;
    add_flag(flgs, TR_BRAND_FIRE);
    add_flag(flgs, TR_KILL_UNDEAD);
    add_flag(flgs, TR_THROW);
}

/*!
 * @brief 投擲時たいまつにダイスを与える。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param dd 特別なダイス数を返す参照ポインタ
 * @param ds 特別なダイス面数を返す参照ポインタ
 * @return なし
 */
void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds)
{
    if ((o_ptr->tval != TV_LITE) || (o_ptr->sval != SV_LITE_TORCH))
        return;
    if (o_ptr->xtra4 <= 0)
        return;
    (*dd) = 1;
    (*ds) = 6;
}

/*!
 * @brief 投擲時命中したたいまつの寿命を縮める。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @return なし
 */
void torch_lost_fuel(object_type *o_ptr)
{
    if ((o_ptr->tval != TV_LITE) || (o_ptr->sval != SV_LITE_TORCH))
        return;
    o_ptr->xtra4 -= (FUEL_TORCH / 25);
    if (o_ptr->xtra4 < 0)
        o_ptr->xtra4 = 0;
}
