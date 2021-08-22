#include "specific-object/torch.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-ninja.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include <vector>

/*!
 * @brief 投擲時たいまつに投げやすい/焼棄/アンデッドスレイの特別効果を返す。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param flgs 特別に追加するフラグを返す参照ポインタ
 */
void torch_flags(object_type *o_ptr, TrFlags &flgs)
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
 * @details
 * SWD: Experimental modification: multiple light sources have additive effect.
 */
void update_lite_radius(player_type *creature_ptr)
{
    creature_ptr->cur_lite = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[i];
        TrFlags flgs;
        object_flags(creature_ptr, o_ptr, flgs);

        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->name2 == EGO_LITE_SHINE)
            creature_ptr->cur_lite++;

        if (!has_flag(flgs, TR_DARK_SOURCE)) {
            if (o_ptr->tval == TV_LITE) {
                if ((o_ptr->sval == SV_LITE_TORCH) && !(o_ptr->xtra4 > 0))
                    continue;

                if ((o_ptr->sval == SV_LITE_LANTERN) && !(o_ptr->xtra4 > 0))
                    continue;
            }
        }

        POSITION rad = 0;
        if (has_flag(flgs, TR_LITE_1) && !has_flag(flgs, TR_DARK_SOURCE))
            rad += 1;

        if (has_flag(flgs, TR_LITE_2) && !has_flag(flgs, TR_DARK_SOURCE))
            rad += 2;

        if (has_flag(flgs, TR_LITE_3) && !has_flag(flgs, TR_DARK_SOURCE))
            rad += 3;

        if (has_flag(flgs, TR_LITE_M1))
            rad -= 1;

        if (has_flag(flgs, TR_LITE_M2))
            rad -= 2;

        if (has_flag(flgs, TR_LITE_M3))
            rad -= 3;

        creature_ptr->cur_lite += rad;
    }

    if (d_info[creature_ptr->dungeon_idx].flags.has(DF::DARKNESS) && creature_ptr->cur_lite > 1)
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
        set_superstealth(creature_ptr, false);
}

/*
 * Update the set of grids "illuminated" by the player's lite.
 *
 * This routine needs to use the results of "update_view()"
 *
 * Note that "blindness" does NOT affect "torch lite".  Be careful!
 *
 * We optimize most lites (all non-artifact lites) by using "obvious"
 * facts about the results of "small" lite radius, and we attempt to
 * list the "nearby" grids before the more "distant" ones in the
 * array of torch-lit grids.
 *
 * We assume that "radius zero" lite is in fact no lite at all.
 *
 *     Torch     Lantern     Artifacts
 *     (etc)
 *                              ***
 *                 ***         *****
 *      ***       *****       *******
 *      *@*       **@**       ***@***
 *      ***       *****       *******
 *                 ***         *****
 *                              ***
 */
void update_lite(player_type *subject_ptr)
{
    // 前回照らされていた座標たちを格納する配列。
    std::vector<Pos2D> points;

    POSITION p = subject_ptr->cur_lite;
    floor_type *const floor_ptr = subject_ptr->current_floor_ptr;

    // 前回照らされていた座標たちを記録。
    for (int i = 0; i < floor_ptr->lite_n; i++) {
        const POSITION y = floor_ptr->lite_y[i];
        const POSITION x = floor_ptr->lite_x[i];

        floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);
        floor_ptr->grid_array[y][x].info |= CAVE_TEMP;

        points.emplace_back(y, x);
    }

    floor_ptr->lite_n = 0;
    if (p >= 1) {
        cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x);
        cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x);
        cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x);
        cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x + 1);
        cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x - 1);
        cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 1);
        cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 1);
        cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 1);
        cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 1);
    }

    if (p >= 2) {
        if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x)) {
            cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x);
            cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x + 1);
            cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x - 1);
        }

        if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x)) {
            cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x);
            cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x + 1);
            cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x - 1);
        }

        if (cave_los_bold(floor_ptr, subject_ptr->y, subject_ptr->x + 1)) {
            cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x + 2);
            cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 2);
            cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 2);
        }

        if (cave_los_bold(floor_ptr, subject_ptr->y, subject_ptr->x - 1)) {
            cave_lite_hack(floor_ptr, subject_ptr->y, subject_ptr->x - 2);
            cave_lite_hack(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 2);
            cave_lite_hack(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 2);
        }
    }

    if (p >= 3) {
        int d;
        if (p > 14)
            p = 14;

        if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x + 1))
            cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x + 2);

        if (cave_los_bold(floor_ptr, subject_ptr->y + 1, subject_ptr->x - 1))
            cave_lite_hack(floor_ptr, subject_ptr->y + 2, subject_ptr->x - 2);

        if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x + 1))
            cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x + 2);

        if (cave_los_bold(floor_ptr, subject_ptr->y - 1, subject_ptr->x - 1))
            cave_lite_hack(floor_ptr, subject_ptr->y - 2, subject_ptr->x - 2);

        POSITION min_y = subject_ptr->y - p;
        if (min_y < 0)
            min_y = 0;

        POSITION max_y = subject_ptr->y + p;
        if (max_y > floor_ptr->height - 1)
            max_y = floor_ptr->height - 1;

        POSITION min_x = subject_ptr->x - p;
        if (min_x < 0)
            min_x = 0;

        POSITION max_x = subject_ptr->x + p;
        if (max_x > floor_ptr->width - 1)
            max_x = floor_ptr->width - 1;

        for (POSITION y = min_y; y <= max_y; y++) {
            for (POSITION x = min_x; x <= max_x; x++) {
                int dy = (subject_ptr->y > y) ? (subject_ptr->y - y) : (y - subject_ptr->y);
                int dx = (subject_ptr->x > x) ? (subject_ptr->x - x) : (x - subject_ptr->x);
                if ((dy <= 2) && (dx <= 2))
                    continue;

                d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
                if (d > p)
                    continue;

                if (floor_ptr->grid_array[y][x].info & CAVE_VIEW)
                    cave_lite_hack(floor_ptr, y, x);
            }
        }
    }

    for (int i = 0; i < floor_ptr->lite_n; i++) {
        POSITION y = floor_ptr->lite_y[i];
        POSITION x = floor_ptr->lite_x[i];
        grid_type *g_ptr = &floor_ptr->grid_array[y][x];
        if (g_ptr->info & CAVE_TEMP)
            continue;

        cave_note_and_redraw_later(floor_ptr, y, x);
    }

    // 前回照らされていた座標たちのうち、状態が変わったものについて再描画フラグを立てる。
    for (const auto &[y, x] : points) {
        grid_type *g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_TEMP);
        if (g_ptr->info & CAVE_LITE)
            continue;

        cave_redraw_later(floor_ptr, y, x);
    }

    subject_ptr->update |= PU_DELAY_VIS;
}
