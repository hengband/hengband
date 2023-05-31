/*!
 * @brief グリッドの実装 / low level dungeon routines -BEN-
 * @date 2013/12/30
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * \n
 * Support for Adam Bolt's tileset, lighting and transparency effects\n
 * by Robert Ruehlmann (rr9@angband.org)\n
 * \n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "grid/grid.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-generator.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature-action-flags.h"
#include "grid/feature.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "io/screen-util.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object/item-tester-hooker.h"
#include "object/object-mark-types.h"
#include "player-info/class-info.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "room/rooms-builder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/point-2d.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <queue>

/*!
 * @brief 新規フロアに入りたてのプレイヤーをランダムな場所に配置する / Returns random co-ordinates for player/monster/object
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 配置に成功したらTRUEを返す
 */
bool new_player_spot(PlayerType *player_ptr)
{
    POSITION y = 0, x = 0;
    int max_attempts = 10000;

    grid_type *g_ptr;
    TerrainType *f_ptr;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    while (max_attempts--) {
        /* Pick a legal spot */
        y = (POSITION)rand_range(1, floor_ptr->height - 2);
        x = (POSITION)rand_range(1, floor_ptr->width - 2);

        g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

        /* Must be a "naked" floor grid */
        if (g_ptr->m_idx) {
            continue;
        }
        if (floor_ptr->is_in_dungeon()) {
            f_ptr = &terrains_info[g_ptr->feat];

            if (max_attempts > 5000) /* Rule 1 */
            {
                if (f_ptr->flags.has_not(TerrainCharacteristics::FLOOR)) {
                    continue;
                }
            } else /* Rule 2 */
            {
                if (f_ptr->flags.has_not(TerrainCharacteristics::MOVE)) {
                    continue;
                }
                if (f_ptr->flags.has(TerrainCharacteristics::HIT_TRAP)) {
                    continue;
                }
            }

            /* Refuse to start on anti-teleport grids in dungeon */
            if (f_ptr->flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
                continue;
            }
        }
        if (!player_can_enter(player_ptr, g_ptr->feat, 0)) {
            continue;
        }
        if (!in_bounds(floor_ptr, y, x)) {
            continue;
        }

        /* Refuse to start on anti-teleport grids */
        if (g_ptr->is_icky()) {
            continue;
        }

        break;
    }

    if (max_attempts < 1) { /* Should be -1, actually if we failed... */
        return false;
    }

    /* Save the new player grid */
    player_ptr->y = y;
    player_ptr->x = x;

    return true;
}

/*!
 * @brief マスに隠されたドアがあるかの判定を行う。 / Return TRUE if the given grid is a hidden closed door
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param g_ptr マス構造体の参照ポインタ
 * @return 隠されたドアがあるならTRUEを返す。
 */
bool is_hidden_door(PlayerType *player_ptr, grid_type *g_ptr)
{
    if ((g_ptr->mimic || g_ptr->cave_has_flag(TerrainCharacteristics::SECRET)) && is_closed_door(player_ptr, g_ptr->feat)) {
        return true;
    } else {
        return false;
    }
}

/*!
 * @brief 指定された座標のマスが現在照らされているかを返す。 / Check for "local" illumination
 * @param y y座標
 * @param x x座標
 * @return 指定された座標に照明がかかっているならTRUEを返す。。
 */
bool check_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const auto yy = (y < player_ptr->y) ? (y + 1) : (y > player_ptr->y) ? (y - 1)
                                                                        : y;
    const auto xx = (x < player_ptr->x) ? (x + 1) : (x > player_ptr->x) ? (x - 1)
                                                                        : x;
    const auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &grid_yyxx = floor_ptr->grid_array[yy][xx];
    const auto &grid_yxx = floor_ptr->grid_array[y][xx];
    const auto &grid_yyx = floor_ptr->grid_array[yy][x];
    auto is_illuminated = feat_supports_los(grid_yyxx.get_feat_mimic()) && (grid_yyxx.info & CAVE_GLOW);
    is_illuminated |= feat_supports_los(grid_yxx.get_feat_mimic()) && (grid_yxx.info & CAVE_GLOW);
    is_illuminated |= feat_supports_los(grid_yyx.get_feat_mimic()) && (grid_yyx.info & CAVE_GLOW);
    return is_illuminated;
}

/*!
 * @brief 対象座標のマスの照明状態を更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 更新したいマスのY座標
 * @param x 更新したいマスのX座標
 */
static void update_local_illumination_aux(PlayerType *player_ptr, int y, int x)
{
    if (!player_has_los_bold(player_ptr, y, x)) {
        return;
    }

    const auto &grid = player_ptr->current_floor_ptr->grid_array[y][x];
    if (grid.m_idx > 0) {
        update_monster(player_ptr, grid.m_idx, false);
    }

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
}

/*!
 * @brief 指定された座標の照明状態を更新する / Update "local" illumination
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 視界先y座標
 * @param x 視界先x座標
 */
void update_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x)
{
    int i;
    POSITION yy, xx;

    if (!in_bounds(player_ptr->current_floor_ptr, y, x)) {
        return;
    }

    if ((y != player_ptr->y) && (x != player_ptr->x)) {
        yy = (y < player_ptr->y) ? (y - 1) : (y + 1);
        xx = (x < player_ptr->x) ? (x - 1) : (x + 1);
        update_local_illumination_aux(player_ptr, yy, xx);
        update_local_illumination_aux(player_ptr, y, xx);
        update_local_illumination_aux(player_ptr, yy, x);
    } else if (x != player_ptr->x) /* y == player_ptr->y */
    {
        xx = (x < player_ptr->x) ? (x - 1) : (x + 1);
        for (i = -1; i <= 1; i++) {
            yy = y + i;
            update_local_illumination_aux(player_ptr, yy, xx);
        }
        yy = y - 1;
        update_local_illumination_aux(player_ptr, yy, x);
        yy = y + 1;
        update_local_illumination_aux(player_ptr, yy, x);
    } else if (y != player_ptr->y) /* x == player_ptr->x */
    {
        yy = (y < player_ptr->y) ? (y - 1) : (y + 1);
        for (i = -1; i <= 1; i++) {
            xx = x + i;
            update_local_illumination_aux(player_ptr, yy, xx);
        }
        xx = x - 1;
        update_local_illumination_aux(player_ptr, y, xx);
        xx = x + 1;
        update_local_illumination_aux(player_ptr, y, xx);
    } else /* Player's grid */
    {
        for (i = 0; i < 8; i++) {
            yy = y + ddy_cdd[i];
            xx = x + ddx_cdd[i];
            update_local_illumination_aux(player_ptr, yy, xx);
        }
    }
}

/*!
 * @brief 指定された座標をプレイヤー収められていない状態かどうか / Returns true if the player's grid is dark
 * @return 視覚に収められていないならTRUEを返す
 * @details player_can_see_bold()関数の返り値の否定を返している。
 */
bool no_lite(PlayerType *player_ptr)
{
    return !player_can_see_bold(player_ptr, player_ptr->y, player_ptr->x);
}

/*
 * Place an attr/char pair at the given map coordinate, if legal.
 */
void print_rel(PlayerType *player_ptr, char c, TERM_COLOR a, POSITION y, POSITION x)
{
    /* Only do "legal" locations */
    if (panel_contains(y, x)) {
        /* Hack -- fake monochrome */
        if (!use_graphics) {
            if (w_ptr->timewalk_m_idx) {
                a = TERM_DARK;
            } else if (is_invuln(player_ptr) || player_ptr->timewalk) {
                a = TERM_WHITE;
            } else if (player_ptr->wraith_form) {
                a = TERM_L_DARK;
            }
        }

        /* Draw the char using the attr */
        term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, 0, 0);
    }
}

void print_bolt_pict(PlayerType *player_ptr, POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ)
{
    const auto [a, c] = bolt_pict(y, x, ny, nx, typ);
    print_rel(player_ptr, c, a, ny, nx);
}

/*!
 * Memorize interesting viewable object/features in the given grid
 *
 * This function should only be called on "legal" grids.
 *
 * This function will memorize the object and/or feature in the given
 * grid, if they are (1) viewable and (2) interesting.  Note that all
 * objects are interesting, all terrain features except floors (and
 * invisible traps) are interesting, and floors (and invisible traps)
 * are interesting sometimes (depending on various options involving
 * the illumination of floor grids).
 *
 * The automatic memorization of all objects and non-floor terrain
 * features as soon as they are displayed allows incredible amounts
 * of optimization in various places, especially "map_info()".
 *
 * Note that the memorization of objects is completely separate from
 * the memorization of terrain features, preventing annoying floor
 * memorization when a detected object is picked up from a dark floor,
 * and object memorization when an object is dropped into a floor grid
 * which is memorized but out-of-sight.
 *
 * This function should be called every time the "memorization" of
 * a grid (or the object in a grid) is called into question, such
 * as when an object is created in a grid, when a terrain feature
 * "changes" from "floor" to "non-floor", when any grid becomes
 * "illuminated" or "viewable", and when a "floor" grid becomes
 * "torch-lit".
 *
 * Note the relatively efficient use of this function by the various
 * "update_view()" and "update_lite()" calls, to allow objects and
 * terrain features to be memorized (and drawn) whenever they become
 * viewable or illuminated in any way, but not when they "maintain"
 * or "lose" their previous viewability or illumination.
 *
 * Note the butchered "internal" version of "player_can_see_bold()",
 * optimized primarily for the most common cases, that is, for the
 * non-marked floor grids.
 */
void note_spot(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

    /* Blind players see nothing */
    if (player_ptr->effects()->blindness()->is_blind()) {
        return;
    }

    /* Analyze non-torch-lit grids */
    if (!(g_ptr->info & (CAVE_LITE | CAVE_MNLT))) {
        /* Require line of sight to the grid */
        if (!(g_ptr->info & (CAVE_VIEW))) {
            return;
        }

        /* Require "perma-lite" of the grid */
        if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
            /* Not Ninja */
            if (!player_ptr->see_nocto) {
                return;
            }
        }
    }

    /* Hack -- memorize objects */
    for (const auto this_o_idx : g_ptr->o_idx_list) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];

        /* Memorize objects */
        o_ptr->marked.set(OmType::FOUND);
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
    }

    /* Hack -- memorize grids */
    if (!g_ptr->is_mark()) {
        /* Feature code (applying "mimic" field) */
        auto *f_ptr = &terrains_info[g_ptr->get_feat_mimic()];

        /* Memorize some "boring" grids */
        if (f_ptr->flags.has_not(TerrainCharacteristics::REMEMBER)) {
            /* Option -- memorize all torch-lit floors */
            if (view_torch_grids && ((g_ptr->info & (CAVE_LITE | CAVE_MNLT)) || player_ptr->see_nocto)) {
                g_ptr->info |= (CAVE_MARK);
            }

            /* Option -- memorize all perma-lit floors */
            else if (view_perma_grids && ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)) {
                g_ptr->info |= (CAVE_MARK);
            }
        }

        /* Memorize normal grids */
        else if (f_ptr->flags.has(TerrainCharacteristics::LOS)) {
            g_ptr->info |= (CAVE_MARK);
        }

        /* Memorize torch-lit walls */
        else if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) {
            g_ptr->info |= (CAVE_MARK);
        }

        /* Memorize walls seen by noctovision of Ninja */
        else if (player_ptr->see_nocto) {
            g_ptr->info |= (CAVE_MARK);
        }

        /* Memorize certain non-torch-lit wall grids */
        else if (check_local_illumination(player_ptr, y, x)) {
            g_ptr->info |= (CAVE_MARK);
        }
    }

    /* Memorize terrain of the grid */
    g_ptr->info |= (CAVE_KNOWN);
}

/*
 * Redraw (on the screen) a given MAP location
 *
 * This function should only be called on "legal" grids
 */
void lite_spot(PlayerType *player_ptr, POSITION y, POSITION x)
{
    if (panel_contains(y, x) && in_bounds2(player_ptr->current_floor_ptr, y, x)) {
        TERM_COLOR a;
        char c;
        TERM_COLOR ta;
        char tc;

        map_info(player_ptr, y, x, &a, &c, &ta, &tc);
        if (!use_graphics) {
            if (w_ptr->timewalk_m_idx) {
                a = TERM_DARK;
            } else if (is_invuln(player_ptr) || player_ptr->timewalk) {
                a = TERM_WHITE;
            } else if (player_ptr->wraith_form) {
                a = TERM_L_DARK;
            }
        }

        term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);
        const auto flags = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }
}

/*
 * Some comments on the grid flags.  -BEN-
 *
 *
 * One of the major bottlenecks in previous versions of Angband was in
 * the calculation of "line of sight" from the player to various grids,
 * such as monsters.  This was such a nasty bottleneck that a lot of
 * silly things were done to reduce the dependancy on "line of sight",
 * for example, you could not "see" any grids in a lit room until you
 * actually entered the room, and there were all kinds of bizarre grid
 * flags to enable this behavior.  This is also why the "call light"
 * spells always lit an entire room.
 *
 * The code below provides functions to calculate the "field of view"
 * for the player, which, once calculated, provides extremely fast
 * calculation of "line of sight from the player", and to calculate
 * the "field of torch lite", which, again, once calculated, provides
 * extremely fast calculation of "which grids are lit by the player's
 * lite source".  In addition to marking grids as "GRID_VIEW" and/or
 * "GRID_LITE", as appropriate, these functions maintain an array for
 * each of these two flags, each array containing the locations of all
 * of the grids marked with the appropriate flag, which can be used to
 * very quickly scan through all of the grids in a given set.
 *
 * To allow more "semantically valid" field of view semantics, whenever
 * the field of view (or the set of torch lit grids) changes, all of the
 * grids in the field of view (or the set of torch lit grids) are "drawn"
 * so that changes in the world will become apparent as soon as possible.
 * This has been optimized so that only grids which actually "change" are
 * redrawn, using the "temp" array and the "GRID_TEMP" flag to keep track
 * of the grids which are entering or leaving the relevent set of grids.
 *
 * These new methods are so efficient that the old nasty code was removed.
 *
 * Note that there is no reason to "update" the "viewable space" unless
 * the player "moves", or walls/doors are created/destroyed, and there
 * is no reason to "update" the "torch lit grids" unless the field of
 * view changes, or the "light radius" changes.  This means that when
 * the player is resting, or digging, or doing anything that does not
 * involve movement or changing the state of the dungeon, there is no
 * need to update the "view" or the "lite" regions, which is nice.
 *
 * Note that the calls to the nasty "los()" function have been reduced
 * to a bare minimum by the use of the new "field of view" calculations.
 *
 * I wouldn't be surprised if slight modifications to the "update_view()"
 * function would allow us to determine "reverse line-of-sight" as well
 * as "normal line-of-sight", which would allow monsters to use a more
 * "correct" calculation to determine if they can "see" the player.  For
 * now, monsters simply "cheat" somewhat and assume that if the player
 * has "line of sight" to the monster, then the monster can "pretend"
 * that it has "line of sight" to the player.
 *
 *
 * The "update_lite()" function maintains the "CAVE_LITE" flag for each
 * grid and maintains an array of all "CAVE_LITE" grids.
 *
 * This set of grids is the complete set of all grids which are lit by
 * the players light source, which allows the "player_can_see_bold()"
 * function to work very quickly.
 *
 * Note that every "CAVE_LITE" grid is also a "CAVE_VIEW" grid, and in
 * fact, the player (unless blind) can always "see" all grids which are
 * marked as "CAVE_LITE", unless they are "off screen".
 *
 *
 * The "update_view()" function maintains the "CAVE_VIEW" flag for each
 * grid and maintains an array of all "CAVE_VIEW" grids.
 *
 * This set of grids is the complete set of all grids within line of sight
 * of the player, allowing the "player_has_los_bold()" macro to work very
 * quickly.
 *
 *
 * The current "update_view()" algorithm uses the "CAVE_XTRA" flag as a
 * temporary internal flag to mark those grids which are not only in view,
 * but which are also "easily" in line of sight of the player.  This flag
 * is always cleared when we are done.
 *
 *
 * The current "update_lite()" and "update_view()" algorithms use the
 * "CAVE_TEMP" flag, and the array of grids which are marked as "CAVE_TEMP",
 * to keep track of which grids were previously marked as "CAVE_LITE" or
 * "CAVE_VIEW", which allows us to optimize the "screen updates".
 *
 * The "CAVE_TEMP" flag, and the array of "CAVE_TEMP" grids, is also used
 * for various other purposes, such as spreading lite or darkness during
 * "lite_room()" / "unlite_room()", and for calculating monster flow.
 *
 *
 * Any grid can be marked as "CAVE_GLOW" which means that the grid itself is
 * in some way permanently lit.  However, for the player to "see" anything
 * in the grid, as determined by "player_can_see()", the player must not be
 * blind, the grid must be marked as "CAVE_VIEW", and, in addition, "wall"
 * grids, even if marked as "perma lit", are only illuminated if they touch
 * a grid which is not a wall and is marked both "CAVE_GLOW" and "CAVE_VIEW".
 *
 *
 * To simplify various things, a grid may be marked as "CAVE_MARK", meaning
 * that even if the player cannot "see" the grid, he "knows" the terrain in
 * that grid.  This is used to "remember" walls/doors/stairs/floors when they
 * are "seen" or "detected", and also to "memorize" floors, after "wiz_lite()",
 * or when one of the "memorize floor grids" options induces memorization.
 *
 * Objects are "memorized" in a different way, using a special "marked" flag
 * on the object itself, which is set when an object is observed or detected.
 *
 *
 * A grid may be marked as "CAVE_ROOM" which means that it is part of a "room",
 * and should be illuminated by "lite room" and "darkness" spells.
 *
 *
 * A grid may be marked as "CAVE_ICKY" which means it is part of a "vault",
 * and should be unavailable for "teleportation" destinations.
 *
 *
 * The "view_perma_grids" allows the player to "memorize" every perma-lit grid
 * which is observed, and the "view_torch_grids" allows the player to memorize
 * every torch-lit grid.  The player will always memorize important walls,
 * doors, stairs, and other terrain features, as well as any "detected" grids.
 *
 * Note that the new "update_view()" method allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone of
 * floor appearing as the player gets closer to the door.  Also, by not turning
 * on the "memorize perma-lit grids" option, the player will only "see" those
 * floor grids which are actually in line of sight.
 *
 * And my favorite "plus" is that you can now use a special option to draw the
 * "floors" in the "viewable region" brightly (actually, to draw the *other*
 * grids dimly), providing a "pretty" effect as the player runs around, and
 * to efficiently display the "torch lite" in a special color.
 *
 *
 * Some comments on the "update_view()" algorithm...
 *
 * The algorithm is very fast, since it spreads "obvious" grids very quickly,
 * and only has to call "los()" on the borderline cases.  The major axes/diags
 * even terminate early when they hit walls.  I need to find a quick way
 * to "terminate" the other scans.
 *
 * Note that in the worst case (a big empty area with say 5% scattered walls),
 * each of the 1500 or so nearby grids is checked once, most of them getting
 * an "instant" rating, and only a small portion requiring a call to "los()".
 *
 * The only time that the algorithm appears to be "noticeably" too slow is
 * when running, and this is usually only important in town, since the town
 * provides about the worst scenario possible, with large open regions and
 * a few scattered obstructions.  There is a special "efficiency" option to
 * allow the player to reduce his field of view in town, if needed.
 *
 * In the "best" case (say, a normal stretch of corridor), the algorithm
 * makes one check for each viewable grid, and makes no calls to "los()".
 * So running in corridors is very fast, and if a lot of monsters are
 * nearby, it is much faster than the old methods.
 *
 * Note that resting, most normal commands, and several forms of running,
 * plus all commands executed near large groups of monsters, are strictly
 * more efficient with "update_view()" that with the old "compute los() on
 * demand" method, primarily because once the "field of view" has been
 * calculated, it does not have to be recalculated until the player moves
 * (or a wall or door is created or destroyed).
 *
 * Note that we no longer have to do as many "los()" checks, since once the
 * "view" region has been built, very few things cause it to be "changed"
 * (player movement, and the opening/closing of doors, changes in wall status).
 * Note that door/wall changes are only relevant when the door/wall itself is
 * in the "view" region.
 *
 * The algorithm seems to only call "los()" from zero to ten times, usually
 * only when coming down a corridor into a room, or standing in a room, just
 * misaligned with a corridor.  So if, say, there are five "nearby" monsters,
 * we will be reducing the calls to "los()".
 *
 * I am thinking in terms of an algorithm that "walks" from the central point
 * out to the maximal "distance", at each point, determining the "view" code
 * (above).  For each grid not on a major axis or diagonal, the "view" code
 * depends on the "cave_los_bold()" and "view" of exactly two other grids
 * (the one along the nearest diagonal, and the one next to that one, see
 * "update_view_aux()"...).
 *
 * We "memorize" the viewable space array, so that at the cost of under 3000
 * bytes, we reduce the time taken by "forget_view()" to one assignment for
 * each grid actually in the "viewable space".  And for another 3000 bytes,
 * we prevent "erase + redraw" ineffiencies via the "seen" set.  These bytes
 * are also used by other routines, thus reducing the cost to almost nothing.
 *
 * A similar thing is done for "forget_lite()" in which case the savings are
 * much less, but save us from doing bizarre maintenance checking.
 *
 * In the worst "normal" case (in the middle of the town), the reachable space
 * actually reaches to more than half of the largest possible "circle" of view,
 * or about 800 grids, and in the worse case (in the middle of a dungeon level
 * where all the walls have been removed), the reachable space actually reaches
 * the theoretical maximum size of just under 1500 grids.
 *
 * Each grid G examines the "state" of two (?) other (adjacent) grids, G1 & G2.
 * If G1 is lite, G is lite.  Else if G2 is lite, G is half.  Else if G1 and G2
 * are both half, G is half.  Else G is dark.  It only takes 2 (or 4) bits to
 * "name" a grid, so (for MAX_RAD of 20) we could use 1600 bytes, and scan the
 * entire possible space (including initialization) in one step per grid.  If
 * we do the "clearing" as a separate step (and use an array of "view" grids),
 * then the clearing will take as many steps as grids that were viewed, and the
 * algorithm will be able to "stop" scanning at various points.
 * Oh, and outside of the "torch radius", only "lite" grids need to be scanned.
 */

/*
 * Hack - speed up the update_flow algorithm by only doing
 * it everytime the player moves out of LOS of the last
 * "way-point".
 */
static POSITION flow_x = 0;
static POSITION flow_y = 0;

/*
 * Hack -- fill in the "cost" field of every grid that the player
 * can "reach" with the number of steps needed to reach that grid.
 * This also yields the "distance" of the player from every grid.
 *
 * In addition, mark the "when" of the grids that can reach
 * the player with the incremented value of "flow_n".
 *
 * Hack -- use the "seen" array as a "circular queue".
 *
 * We do not need a priority queue because the cost from grid
 * to grid is always "one" and we process them in order.
 */
void update_flow(PlayerType *player_ptr)
{
    POSITION x, y;
    DIRECTION d;
    FloorType *f_ptr = player_ptr->current_floor_ptr;

    /* The last way-point is on the map */
    if (player_ptr->running && in_bounds(f_ptr, flow_y, flow_x)) {
        /* The way point is in sight - do not update.  (Speedup) */
        if (f_ptr->grid_array[flow_y][flow_x].info & CAVE_VIEW) {
            return;
        }
    }

    /* Erase all of the current flow information */
    for (y = 0; y < f_ptr->height; y++) {
        for (x = 0; x < f_ptr->width; x++) {
            memset(&f_ptr->grid_array[y][x].costs, 0, sizeof(f_ptr->grid_array[y][x].costs));
            memset(&f_ptr->grid_array[y][x].dists, 0, sizeof(f_ptr->grid_array[y][x].dists));
        }
    }

    /* Save player position */
    flow_y = player_ptr->y;
    flow_x = player_ptr->x;

    for (int i = 0; i < FLOW_MAX; i++) {
        // 幅優先探索用のキュー。
        std::queue<Pos2D> que;
        que.emplace(player_ptr->y, player_ptr->x);

        /* Now process the queue */
        while (!que.empty()) {
            /* Extract the next entry */
            const auto [ty, tx] = que.front();
            que.pop();

            /* Add the "children" */
            for (d = 0; d < 8; d++) {
                byte m = player_ptr->current_floor_ptr->grid_array[ty][tx].costs[i] + 1;
                byte n = player_ptr->current_floor_ptr->grid_array[ty][tx].dists[i] + 1;

                /* Child location */
                y = ty + ddy_ddd[d];
                x = tx + ddx_ddd[d];

                /* Ignore player's grid */
                if (player_bold(player_ptr, y, x)) {
                    continue;
                }

                auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

                if (is_closed_door(player_ptr, g_ptr->feat)) {
                    m += 3;
                }

                /* Ignore "pre-stamped" entries */
                if (g_ptr->dists[i] != 0 && g_ptr->dists[i] <= n && g_ptr->costs[i] <= m) {
                    continue;
                }

                /* Ignore "walls", "holes" and "rubble" */
                bool can_move = false;
                switch (i) {
                case FLOW_CAN_FLY:
                    can_move = g_ptr->cave_has_flag(TerrainCharacteristics::MOVE) || g_ptr->cave_has_flag(TerrainCharacteristics::CAN_FLY);
                    break;
                default:
                    can_move = g_ptr->cave_has_flag(TerrainCharacteristics::MOVE);
                    break;
                }

                if (!can_move && !is_closed_door(player_ptr, g_ptr->feat)) {
                    continue;
                }

                /* Save the flow cost */
                if (g_ptr->costs[i] == 0 || g_ptr->costs[i] > m) {
                    g_ptr->costs[i] = m;
                }
                if (g_ptr->dists[i] == 0 || g_ptr->dists[i] > n) {
                    g_ptr->dists[i] = n;
                }

                // 敵のプレイヤーに対する移動道のりの最大値(この値以上は処理を打ち切る).
                constexpr auto monster_flow_depth = 32;
                if (n == monster_flow_depth) {
                    continue;
                }

                /* Enqueue that entry */
                que.emplace(y, x);
            }
        }
    }
}

/*
 * Take a feature, determine what that feature becomes
 * through applying the given action.
 */
FEAT_IDX feat_state(FloorType *floor_ptr, FEAT_IDX feat, TerrainCharacteristics action)
{
    auto *f_ptr = &terrains_info[feat];
    int i;

    /* Get the new feature */
    for (i = 0; i < MAX_FEAT_STATES; i++) {
        if (f_ptr->state[i].action == action) {
            return conv_dungeon_feat(floor_ptr, f_ptr->state[i].result);
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::PERMANENT)) {
        return feat;
    }

    return (terrain_action_flags[enum2i(action)] & FAF_DESTROY) ? conv_dungeon_feat(floor_ptr, f_ptr->destroyed) : feat;
}

/*
 * Takes a location and action and changes the feature at that
 * location through applying the given action.
 */
void cave_alter_feat(PlayerType *player_ptr, POSITION y, POSITION x, TerrainCharacteristics action)
{
    /* Set old feature */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    FEAT_IDX oldfeat = floor_ptr->grid_array[y][x].feat;

    /* Get the new feat */
    FEAT_IDX newfeat = feat_state(player_ptr->current_floor_ptr, oldfeat, action);

    /* No change */
    if (newfeat == oldfeat) {
        return;
    }

    /* Set the new feature */
    cave_set_feat(player_ptr, y, x, newfeat);

    if (!(terrain_action_flags[enum2i(action)] & FAF_NO_DROP)) {
        TerrainType *old_f_ptr = &terrains_info[oldfeat];
        auto *f_ptr = &terrains_info[newfeat];
        bool found = false;

        /* Handle gold */
        if (old_f_ptr->flags.has(TerrainCharacteristics::HAS_GOLD) && f_ptr->flags.has_not(TerrainCharacteristics::HAS_GOLD)) {
            /* Place some gold */
            place_gold(player_ptr, y, x);
            found = true;
        }

        /* Handle item */
        if (old_f_ptr->flags.has(TerrainCharacteristics::HAS_ITEM) && f_ptr->flags.has_not(TerrainCharacteristics::HAS_ITEM) && (randint0(100) < (15 - floor_ptr->dun_level / 2))) {
            /* Place object */
            place_object(player_ptr, y, x, 0L);
            found = true;
        }

        if (found && w_ptr->character_dungeon && player_can_see_bold(player_ptr, y, x)) {
            msg_print(_("何かを発見した！", "You have found something!"));
        }
    }

    if (terrain_action_flags[enum2i(action)] & FAF_CRASH_GLASS) {
        TerrainType *old_f_ptr = &terrains_info[oldfeat];

        if (old_f_ptr->flags.has(TerrainCharacteristics::GLASS) && w_ptr->character_dungeon) {
            project(player_ptr, PROJECT_WHO_GLASS_SHARDS, 1, y, x, std::min(floor_ptr->dun_level, 100) / 4, AttributeType::SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }
    }
}

/*!
 * @brief 指定されたマスがモンスターのテレポート可能先かどうかを判定する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param y 移動先Y座標
 * @param x 移動先X座標
 * @param mode オプション
 * @return テレポート先として妥当ならばtrue
 */
bool cave_monster_teleportable_bold(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    auto *f_ptr = &terrains_info[g_ptr->feat];

    /* Require "teleportable" space */
    if (f_ptr->flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
        return false;
    }

    if (g_ptr->m_idx && (g_ptr->m_idx != m_idx)) {
        return false;
    }
    if (player_bold(player_ptr, y, x)) {
        return false;
    }

    /* Hack -- no teleport onto rune of protection */
    if (g_ptr->is_rune_protection()) {
        return false;
    }
    if (g_ptr->is_rune_explosion()) {
        return false;
    }

    if (!(mode & TELEPORT_PASSIVE)) {
        if (!monster_can_cross_terrain(player_ptr, g_ptr->feat, &monraces_info[m_ptr->r_idx], 0)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 指定されたマスにプレイヤーがテレポート可能かどうかを判定する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 移動先Y座標
 * @param x 移動先X座標
 * @param mode オプション
 * @return テレポート先として妥当ならばtrue
 */
bool cave_player_teleportable_bold(PlayerType *player_ptr, POSITION y, POSITION x, teleport_flags mode)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    auto *f_ptr = &terrains_info[g_ptr->feat];

    /* Require "teleportable" space */
    if (f_ptr->flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
        return false;
    }

    /* No magical teleporting into vaults and such */
    if (!(mode & TELEPORT_NONMAGICAL) && g_ptr->is_icky()) {
        return false;
    }

    if (g_ptr->m_idx && (g_ptr->m_idx != player_ptr->riding)) {
        return false;
    }

    /* don't teleport on a trap. */
    if (f_ptr->flags.has(TerrainCharacteristics::HIT_TRAP)) {
        return false;
    }

    if (!(mode & TELEPORT_PASSIVE)) {
        if (!player_can_enter(player_ptr, g_ptr->feat, 0)) {
            return false;
        }

        if (f_ptr->flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP })) {
            if (!player_ptr->levitation && !player_ptr->can_swim) {
                return false;
            }
        }

        if (f_ptr->flags.has(TerrainCharacteristics::LAVA) && !has_immune_fire(player_ptr) && !is_invuln(player_ptr)) {
            /* Always forbid deep lava */
            if (f_ptr->flags.has(TerrainCharacteristics::DEEP)) {
                return false;
            }

            /* Forbid shallow lava when the player don't have levitation */
            if (!player_ptr->levitation) {
                return false;
            }
        }
    }

    return true;
}

/*!
 * @brief 地形は開くものであって、かつ開かれているかを返す /
 * Attempt to open the given chest at the given location
 * @param feat 地形ID
 * @return 開いた地形である場合TRUEを返す /  Return TRUE if the given feature is an open door
 */
bool is_open(PlayerType *player_ptr, FEAT_IDX feat)
{
    return terrains_info[feat].flags.has(TerrainCharacteristics::CLOSE) && (feat != feat_state(player_ptr->current_floor_ptr, feat, TerrainCharacteristics::CLOSE));
}

/*!
 * @brief プレイヤーが地形踏破可能かを返す
 * @param feature 判定したい地形ID
 * @param mode 移動に関するオプションフラグ
 * @return 移動可能ならばTRUEを返す
 */
bool player_can_enter(PlayerType *player_ptr, FEAT_IDX feature, BIT_FLAGS16 mode)
{
    auto *f_ptr = &terrains_info[feature];

    if (player_ptr->riding) {
        return monster_can_cross_terrain(
            player_ptr, feature, &monraces_info[player_ptr->current_floor_ptr->m_list[player_ptr->riding].r_idx], mode | CEM_RIDING);
    }

    if (f_ptr->flags.has(TerrainCharacteristics::PATTERN)) {
        if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
            return false;
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::CAN_FLY) && player_ptr->levitation) {
        return true;
    }
    if (f_ptr->flags.has(TerrainCharacteristics::CAN_SWIM) && player_ptr->can_swim) {
        return true;
    }
    if (f_ptr->flags.has(TerrainCharacteristics::CAN_PASS) && has_pass_wall(player_ptr)) {
        return true;
    }

    if (f_ptr->flags.has_not(TerrainCharacteristics::MOVE)) {
        return false;
    }

    return true;
}

void place_grid(PlayerType *player_ptr, grid_type *g_ptr, grid_bold_type gb_type)
{
    switch (gb_type) {
    case GB_FLOOR: {
        g_ptr->feat = rand_choice(feat_ground_type);
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_FLOOR;
        break;
    }
    case GB_EXTRA: {
        g_ptr->feat = rand_choice(feat_wall_type);
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_EXTRA;
        break;
    }
    case GB_EXTRA_PERM: {
        g_ptr->feat = feat_permanent;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_EXTRA;
        break;
    }
    case GB_INNER: {
        g_ptr->feat = feat_wall_inner;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_INNER;
        break;
    }
    case GB_INNER_PERM: {
        g_ptr->feat = feat_permanent;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_INNER;
        break;
    }
    case GB_OUTER: {
        g_ptr->feat = feat_wall_outer;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_OUTER;
        break;
    }
    case GB_OUTER_NOPERM: {
        auto *f_ptr = &terrains_info[feat_wall_outer];
        if (permanent_wall(f_ptr)) {
            g_ptr->feat = (int16_t)feat_state(player_ptr->current_floor_ptr, feat_wall_outer, TerrainCharacteristics::UNPERM);
        } else {
            g_ptr->feat = feat_wall_outer;
        }

        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= (CAVE_OUTER | CAVE_VAULT);
        break;
    }
    case GB_SOLID: {
        g_ptr->feat = feat_wall_solid;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_SOLID;
        break;
    }
    case GB_SOLID_PERM: {
        g_ptr->feat = feat_permanent;
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_SOLID;
        break;
    }
    case GB_SOLID_NOPERM: {
        auto *f_ptr = &terrains_info[feat_wall_solid];
        if ((g_ptr->info & CAVE_VAULT) && permanent_wall(f_ptr)) {
            g_ptr->feat = (int16_t)feat_state(player_ptr->current_floor_ptr, feat_wall_solid, TerrainCharacteristics::UNPERM);
        } else {
            g_ptr->feat = feat_wall_solid;
        }
        g_ptr->info &= ~(CAVE_MASK);
        g_ptr->info |= CAVE_SOLID;
        break;
    }
    default:
        // 未知の値が渡されたら何もしない。
        return;
    }

    if (g_ptr->m_idx > 0) {
        delete_monster_idx(player_ptr, g_ptr->m_idx);
    }
}

/*!
 * モンスターにより照明が消されている地形か否かを判定する。 / Is this grid "darkened" by monster?
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @return 照明が消されている地形ならばTRUE
 */
bool darkened_grid(PlayerType *player_ptr, grid_type *g_ptr)
{
    return ((g_ptr->info & (CAVE_VIEW | CAVE_LITE | CAVE_MNLT | CAVE_MNDK)) == (CAVE_VIEW | CAVE_MNDK)) && !player_ptr->see_nocto;
}

void place_bold(PlayerType *player_ptr, POSITION y, POSITION x, grid_bold_type gb_type)
{
    grid_type *const g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    place_grid(player_ptr, g_ptr, gb_type);
}

void set_cave_feat(FloorType *floor_ptr, POSITION y, POSITION x, FEAT_IDX feature_idx)
{
    floor_ptr->grid_array[y][x].feat = feature_idx;
}

/*!
 * @brief プレイヤーの周辺9マスに該当する地形がいくつあるかを返す /
 * Attempt to open the given chest at the given location
 * @param y 該当する地形の中から1つのY座標を返す参照ポインタ
 * @param x 該当する地形の中から1つのX座標を返す参照ポインタ
 * @param test 地形条件を判定するための関数ポインタ
 * @param under TRUEならばプレイヤーの直下の座標も走査対象にする
 * @return 該当する地形の数
 * @details Return the number of features around (or under) the character.
 * Usually look for doors and floor traps.
 */
int count_dt(PlayerType *player_ptr, POSITION *y, POSITION *x, bool (*test)(PlayerType *, FEAT_IDX), bool under)
{
    int count = 0;
    for (DIRECTION d = 0; d < 9; d++) {
        grid_type *g_ptr;
        FEAT_IDX feat;
        if ((d == 8) && !under) {
            continue;
        }

        POSITION yy = player_ptr->y + ddy_ddd[d];
        POSITION xx = player_ptr->x + ddx_ddd[d];
        g_ptr = &player_ptr->current_floor_ptr->grid_array[yy][xx];
        if (!g_ptr->is_mark()) {
            continue;
        }

        feat = g_ptr->get_feat_mimic();
        if (!((*test)(player_ptr, feat))) {
            continue;
        }

        ++count;
        *y = yy;
        *x = xx;
    }

    return count;
}

/*!
 * @brief マス構造体のspecial要素を利用する地形かどうかを判定する.
 */
bool feat_uses_special(FEAT_IDX f_idx)
{
    return terrains_info[(f_idx)].flags.has(TerrainCharacteristics::SPECIAL);
}

/*
 * This function allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
void cave_lite_hack(FloorType *floor_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->is_lite()) {
        return;
    }

    g_ptr->info |= CAVE_LITE;
    floor_ptr->lite_y[floor_ptr->lite_n] = y;
    floor_ptr->lite_x[floor_ptr->lite_n++] = x;
}

/*
 * For delayed visual update
 */
void cave_redraw_later(FloorType *floor_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->is_redraw()) {
        return;
    }

    g_ptr->info |= CAVE_REDRAW;
    floor_ptr->redraw_y[floor_ptr->redraw_n] = y;
    floor_ptr->redraw_x[floor_ptr->redraw_n++] = x;
}

/*
 * For delayed visual update
 */
void cave_note_and_redraw_later(FloorType *floor_ptr, POSITION y, POSITION x)
{
    floor_ptr->grid_array[y][x].info |= CAVE_NOTE;
    cave_redraw_later(floor_ptr, y, x);
}

void cave_view_hack(FloorType *floor_ptr, POSITION y, POSITION x)
{
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->is_view()) {
        return;
    }

    g_ptr->info |= CAVE_VIEW;
    floor_ptr->view_y[floor_ptr->view_n] = y;
    floor_ptr->view_x[floor_ptr->view_n] = x;
    floor_ptr->view_n++;
}
