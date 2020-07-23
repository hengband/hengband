#include "specific-object/torch.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-ninja.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-lite-types.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 投擲時たいまつに投げやすい/焼棄/アンデッドスレイの特別効果を返す。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param flgs 特別に追加するフラグを返す参照ポインタ
 * @return なし
 */
void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs)
{
    if ((o_ptr->tval != TV_LITE) || (o_ptr->sval != SV_LITE_TORCH) || (o_ptr->xtra4 <= 0))
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
    if ((o_ptr->tval != TV_LITE) || (o_ptr->sval != SV_LITE_TORCH) || (o_ptr->xtra4 <= 0))
        return;

    *dd = 1;
    *ds = 6;
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

/*!
 * @brief プレイヤーの光源半径を計算する / Extract and set the current "lite radius"
 * @return なし
 * @details
 * SWD: Experimental modification: multiple light sources have additive effect.
 */
void calc_lite_radius(player_type *creature_ptr)
{
    creature_ptr->cur_lite = 0;
    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->name2 == EGO_LITE_SHINE)
            creature_ptr->cur_lite++;

        if (o_ptr->name2 != EGO_LITE_DARKNESS) {
            if (o_ptr->tval == TV_LITE) {
                if ((o_ptr->sval == SV_LITE_TORCH) && !(o_ptr->xtra4 > 0))
                    continue;

                if ((o_ptr->sval == SV_LITE_LANTERN) && !(o_ptr->xtra4 > 0))
                    continue;
            }
        }

        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_flags(creature_ptr, o_ptr, flgs);

        POSITION rad = 0;
        if (have_flag(flgs, TR_LITE_1) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 1;

        if (have_flag(flgs, TR_LITE_2) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 2;

        if (have_flag(flgs, TR_LITE_3) && o_ptr->name2 != EGO_LITE_DARKNESS)
            rad += 3;

        if (have_flag(flgs, TR_LITE_M1))
            rad -= 1;

        if (have_flag(flgs, TR_LITE_M2))
            rad -= 2;

        if (have_flag(flgs, TR_LITE_M3))
            rad -= 3;

        creature_ptr->cur_lite += rad;
    }

    if (d_info[creature_ptr->dungeon_idx].flags1 & DF1_DARKNESS && creature_ptr->cur_lite > 1)
        creature_ptr->cur_lite = 1;

    if (creature_ptr->cur_lite <= 0 && creature_ptr->lite)
        creature_ptr->cur_lite++;

    if (creature_ptr->cur_lite > 14)
        creature_ptr->cur_lite = 14;

    if (creature_ptr->cur_lite < 0)
        creature_ptr->cur_lite = 0;

    if (creature_ptr->old_lite == creature_ptr->cur_lite)
        return;

    creature_ptr->update |= PU_LITE | PU_MON_LITE | PU_MONSTERS;
    creature_ptr->old_lite = creature_ptr->cur_lite;

    if ((creature_ptr->cur_lite > 0) && (creature_ptr->special_defense & NINJA_S_STEALTH))
        set_superstealth(creature_ptr, FALSE);
}
