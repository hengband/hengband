#include "specific-object/torch.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-ninja.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include <vector>

/*!
 * @brief 該当オブジェクトが残量アリの松明か否かを判定。
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @return 残量アリの松明ならtrue
 */
bool is_active_torch(ItemEntity *o_ptr)
{
    return (o_ptr->bi_key == BaseitemKey(ItemKindType::LITE, SV_LITE_TORCH)) && (o_ptr->fuel > 0);
}

/*!
 * @brief 投擲時たいまつに投げやすい/焼棄/アンデッドスレイの特別効果を返す。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 * @param flags 特別に追加するフラグを返す参照ポインタ
 */
void torch_flags(ItemEntity *o_ptr, TrFlags &flags)
{
    if (!is_active_torch(o_ptr)) {
        return;
    }

    flags.set(TR_BRAND_FIRE);
    flags.set(TR_KILL_UNDEAD);
    flags.set(TR_THROW);
}

/*!
 * @brief 投擲時命中したたいまつの寿命を縮める。
 * Torches have special abilities when they are flaming.
 * @param o_ptr 投擲するオブジェクトの構造体参照ポインタ
 */
void torch_lost_fuel(ItemEntity *o_ptr)
{
    if (!is_active_torch(o_ptr)) {
        return;
    }

    o_ptr->fuel -= FUEL_TORCH / 25;
    if (o_ptr->fuel < 0) {
        o_ptr->fuel = 0;
    }
}

/*!
 * @brief プレイヤーの光源半径を計算する / Extract and set the current "lite radius"
 * @details
 * SWD: Experimental modification: multiple light sources have additive effect.
 */
void update_lite_radius(PlayerType *player_ptr)
{
    player_ptr->cur_lite = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto *o_ptr = &player_ptr->inventory[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        const auto flags = o_ptr->get_flags();
        if (flags.has_not(TR_DARK_SOURCE)) {
            if (o_ptr->bi_key.tval() == ItemKindType::LITE) {
                const auto sval = o_ptr->bi_key.sval();
                if ((sval == SV_LITE_TORCH || sval == SV_LITE_LANTERN) && (o_ptr->fuel <= 0)) {
                    continue;
                }
            }
        }
        player_ptr->cur_lite += o_ptr->get_lite_radius();
    }

    if (player_ptr->current_floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS) && player_ptr->cur_lite > 1) {
        player_ptr->cur_lite = 1;
    }

    if (player_ptr->cur_lite <= 0 && player_ptr->lite) {
        player_ptr->cur_lite++;
    }

    if (player_ptr->cur_lite > 14) {
        player_ptr->cur_lite = 14;
    }

    if (player_ptr->cur_lite < 0) {
        player_ptr->cur_lite = 0;
    }

    if (player_ptr->old_lite == player_ptr->cur_lite) {
        return;
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    player_ptr->old_lite = player_ptr->cur_lite;
    if (player_ptr->cur_lite > 0) {
        set_superstealth(player_ptr, false);
    }
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
void update_lite(PlayerType *player_ptr)
{
    // 前回照らされていた座標たちを格納する配列。
    std::vector<Pos2D> points;

    POSITION p = player_ptr->cur_lite;
    auto &floor = *player_ptr->current_floor_ptr;

    // 前回照らされていた座標たちを記録。
    for (int i = 0; i < floor.lite_n; i++) {
        const POSITION y = floor.lite_y[i];
        const POSITION x = floor.lite_x[i];

        floor.grid_array[y][x].info &= ~(CAVE_LITE);
        floor.grid_array[y][x].info |= CAVE_TEMP;

        points.emplace_back(y, x);
    }

    floor.lite_n = 0;
    if (p >= 1) {
        cave_lite_hack(floor, player_ptr->y, player_ptr->x);
        cave_lite_hack(floor, player_ptr->y + 1, player_ptr->x);
        cave_lite_hack(floor, player_ptr->y - 1, player_ptr->x);
        cave_lite_hack(floor, player_ptr->y, player_ptr->x + 1);
        cave_lite_hack(floor, player_ptr->y, player_ptr->x - 1);
        cave_lite_hack(floor, player_ptr->y + 1, player_ptr->x + 1);
        cave_lite_hack(floor, player_ptr->y + 1, player_ptr->x - 1);
        cave_lite_hack(floor, player_ptr->y - 1, player_ptr->x + 1);
        cave_lite_hack(floor, player_ptr->y - 1, player_ptr->x - 1);
    }

    if (p >= 2) {
        if (cave_los_bold(floor, player_ptr->y + 1, player_ptr->x)) {
            cave_lite_hack(floor, player_ptr->y + 2, player_ptr->x);
            cave_lite_hack(floor, player_ptr->y + 2, player_ptr->x + 1);
            cave_lite_hack(floor, player_ptr->y + 2, player_ptr->x - 1);
        }

        if (cave_los_bold(floor, player_ptr->y - 1, player_ptr->x)) {
            cave_lite_hack(floor, player_ptr->y - 2, player_ptr->x);
            cave_lite_hack(floor, player_ptr->y - 2, player_ptr->x + 1);
            cave_lite_hack(floor, player_ptr->y - 2, player_ptr->x - 1);
        }

        if (cave_los_bold(floor, player_ptr->y, player_ptr->x + 1)) {
            cave_lite_hack(floor, player_ptr->y, player_ptr->x + 2);
            cave_lite_hack(floor, player_ptr->y + 1, player_ptr->x + 2);
            cave_lite_hack(floor, player_ptr->y - 1, player_ptr->x + 2);
        }

        if (cave_los_bold(floor, player_ptr->y, player_ptr->x - 1)) {
            cave_lite_hack(floor, player_ptr->y, player_ptr->x - 2);
            cave_lite_hack(floor, player_ptr->y + 1, player_ptr->x - 2);
            cave_lite_hack(floor, player_ptr->y - 1, player_ptr->x - 2);
        }
    }

    if (p >= 3) {
        int d;
        if (p > 14) {
            p = 14;
        }

        if (cave_los_bold(floor, player_ptr->y + 1, player_ptr->x + 1)) {
            cave_lite_hack(floor, player_ptr->y + 2, player_ptr->x + 2);
        }

        if (cave_los_bold(floor, player_ptr->y + 1, player_ptr->x - 1)) {
            cave_lite_hack(floor, player_ptr->y + 2, player_ptr->x - 2);
        }

        if (cave_los_bold(floor, player_ptr->y - 1, player_ptr->x + 1)) {
            cave_lite_hack(floor, player_ptr->y - 2, player_ptr->x + 2);
        }

        if (cave_los_bold(floor, player_ptr->y - 1, player_ptr->x - 1)) {
            cave_lite_hack(floor, player_ptr->y - 2, player_ptr->x - 2);
        }

        POSITION min_y = player_ptr->y - p;
        if (min_y < 0) {
            min_y = 0;
        }

        POSITION max_y = player_ptr->y + p;
        if (max_y > floor.height - 1) {
            max_y = floor.height - 1;
        }

        POSITION min_x = player_ptr->x - p;
        if (min_x < 0) {
            min_x = 0;
        }

        POSITION max_x = player_ptr->x + p;
        if (max_x > floor.width - 1) {
            max_x = floor.width - 1;
        }

        for (POSITION y = min_y; y <= max_y; y++) {
            for (POSITION x = min_x; x <= max_x; x++) {
                int dy = (player_ptr->y > y) ? (player_ptr->y - y) : (y - player_ptr->y);
                int dx = (player_ptr->x > x) ? (player_ptr->x - x) : (x - player_ptr->x);
                if ((dy <= 2) && (dx <= 2)) {
                    continue;
                }

                d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
                if (d > p) {
                    continue;
                }

                if (floor.grid_array[y][x].info & CAVE_VIEW) {
                    cave_lite_hack(floor, y, x);
                }
            }
        }
    }

    for (int i = 0; i < floor.lite_n; i++) {
        POSITION y = floor.lite_y[i];
        POSITION x = floor.lite_x[i];
        const auto &grid = floor.grid_array[y][x];
        if (grid.info & CAVE_TEMP) {
            continue;
        }

        cave_note_and_redraw_later(floor, y, x);
    }

    // 前回照らされていた座標たちのうち、状態が変わったものについて再描画フラグを立てる。
    for (const auto &[y, x] : points) {
        auto &grid = floor.grid_array[y][x];
        grid.info &= ~(CAVE_TEMP);
        if (grid.info & CAVE_LITE) {
            continue;
        }

        cave_redraw_later(floor, y, x);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::DELAY_VISIBILITY);
}
