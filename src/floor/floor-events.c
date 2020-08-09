#include "floor/floor-events.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * ダンジョンの雰囲気を計算するための非線形基準値 / Dungeon rating is no longer linear
 */
static int rating_boost(int delta) { return delta * delta + 50 * delta; }

static bool mon_invis;
static POSITION mon_fy, mon_fx;

void day_break(player_type *subject_ptr)
{
    msg_print(_("夜が明けた。", "The sun has risen."));
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    if (!subject_ptr->wild_mode) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                grid_type *g_ptr = &floor_ptr->grid_array[y][x];
                g_ptr->info |= (CAVE_GLOW);
                if (view_perma_grids)
                    g_ptr->info |= (CAVE_MARK);

                note_spot(subject_ptr, y, x);
            }
        }
    }

    subject_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
    subject_ptr->redraw |= (PR_MAP);
    subject_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    if (((subject_ptr->special_defense & NINJA_S_STEALTH) != 0) && ((floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_GLOW) != 0))
        set_superstealth(subject_ptr, FALSE);
}

void night_falls(player_type *subject_ptr)
{
    msg_print(_("日が沈んだ。", "The sun has fallen."));
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    if (!subject_ptr->wild_mode) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                grid_type *g_ptr = &floor_ptr->grid_array[y][x];
                feature_type *f_ptr = &f_info[get_feat_mimic(g_ptr)];
                if (is_mirror_grid(g_ptr) || have_flag(f_ptr->flags, FF_QUEST_ENTER) || have_flag(f_ptr->flags, FF_ENTRANCE))
                    continue;

                g_ptr->info &= ~(CAVE_GLOW);
                if (!have_flag(f_ptr->flags, FF_REMEMBER)) {
                    g_ptr->info &= ~(CAVE_MARK);
                    note_spot(subject_ptr, y, x);
                }
            }

            glow_deep_lava_and_bldg(subject_ptr);
        }
    }

    subject_ptr->update |= PU_MONSTERS | PU_MON_LITE;
    subject_ptr->redraw |= PR_MAP;
    subject_ptr->window |= PW_OVERHEAD | PW_DUNGEON;

    if (((subject_ptr->special_defense & NINJA_S_STEALTH) != 0) && ((floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_GLOW) != 0))
        set_superstealth(subject_ptr, FALSE);
}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr)
{
    MONSTER_NUMBER number_mon = 0;
    for (POSITION x = 0; x < floor_ptr->width; ++x) {
        for (POSITION y = 0; y < floor_ptr->height; ++y) {
            MONSTER_IDX m_idx = floor_ptr->grid_array[y][x].m_idx;
            if (m_idx > 0 && is_hostile(&floor_ptr->m_list[m_idx]))
                ++number_mon;
        }
    }

    return number_mon;
}

/*!
 * @brief ダンジョンの雰囲気を算出する。
 * / Examine all monsters and unidentified objects, and get the feeling of current dungeon floor
 * @return 算出されたダンジョンの雰囲気ランク
 */
static byte get_dungeon_feeling(player_type *subject_ptr)
{
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level)
        return 0;

    const int base = 10;
    int rating = 0;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        monster_race *r_ptr;
        int delta = 0;
        if (!monster_is_valid(m_ptr) || is_pet(m_ptr))
            continue;

        r_ptr = &r_info[m_ptr->r_idx];
        if (r_ptr->flags1 & (RF1_UNIQUE)) {
            if (r_ptr->level + 10 > floor_ptr->dun_level)
                delta += (r_ptr->level + 10 - floor_ptr->dun_level) * 2 * base;
        } else if (r_ptr->level > floor_ptr->dun_level)
            delta += (r_ptr->level - floor_ptr->dun_level) * base;

        if (r_ptr->flags1 & RF1_FRIENDS) {
            if (5 <= get_monster_crowd_number(floor_ptr, i))
                delta += 1;
        } else if (2 <= get_monster_crowd_number(floor_ptr, i))
            delta += 1;

        rating += rating_boost(delta);
    }

    for (MONSTER_IDX i = 1; i < floor_ptr->o_max; i++) {
        object_type *o_ptr = &floor_ptr->o_list[i];
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        int delta = 0;
        if (!object_is_valid(o_ptr) || (object_is_known(o_ptr) && ((o_ptr->marked & OM_TOUCHED) != 0)) || ((o_ptr->ident & IDENT_SENSE) != 0))
            continue;

        if (object_is_ego(o_ptr)) {
            ego_item_type *e_ptr = &e_info[o_ptr->name2];
            delta += e_ptr->rating * base;
        }

        if (object_is_artifact(o_ptr)) {
            PRICE cost = object_value_real(subject_ptr, o_ptr);
            delta += 10 * base;
            if (cost > 10000L)
                delta += 10 * base;

            if (cost > 50000L)
                delta += 10 * base;

            if (cost > 100000L)
                delta += 10 * base;

            if (!preserve_mode)
                return 1;
        }

        if (o_ptr->tval == TV_DRAG_ARMOR)
            delta += 30 * base;

        if (o_ptr->tval == TV_SHIELD && o_ptr->sval == SV_DRAGON_SHIELD)
            delta += 5 * base;

        if (o_ptr->tval == TV_GLOVES && o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)
            delta += 5 * base;

        if (o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)
            delta += 5 * base;

        if (o_ptr->tval == TV_HELM && o_ptr->sval == SV_DRAGON_HELM)
            delta += 5 * base;

        if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED && !object_is_cursed(o_ptr))
            delta += 25 * base;

        if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_LORDLY && !object_is_cursed(o_ptr))
            delta += 15 * base;

        if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_THE_MAGI && !object_is_cursed(o_ptr))
            delta += 15 * base;

        if (!object_is_cursed(o_ptr) && !object_is_broken(o_ptr) && k_ptr->level > floor_ptr->dun_level)
            delta += (k_ptr->level - floor_ptr->dun_level) * base;

        rating += rating_boost(delta);
    }

    if (rating > rating_boost(1000))
        return 2;

    if (rating > rating_boost(800))
        return 3;

    if (rating > rating_boost(600))
        return 4;

    if (rating > rating_boost(400))
        return 5;

    if (rating > rating_boost(300))
        return 6;

    if (rating > rating_boost(200))
        return 7;

    if (rating > rating_boost(100))
        return 8;

    if (rating > rating_boost(0))
        return 9;

    return 10;
}

/*!
 * @brief ダンジョンの雰囲気を更新し、変化があった場合メッセージを表示する
 * / Update dungeon feeling, and announce it if changed
 * @return なし
 */
void update_dungeon_feeling(player_type *subject_ptr)
{
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level)
        return;

    if (subject_ptr->phase_out)
        return;

    int delay = MAX(10, 150 - subject_ptr->skill_fos) * (150 - floor_ptr->dun_level) * TURNS_PER_TICK / 100;
    if (current_world_ptr->game_turn < subject_ptr->feeling_turn + delay && !cheat_xtra)
        return;

    int quest_num = quest_number(subject_ptr, floor_ptr->dun_level);
    if (quest_num
        && (is_fixed_quest_idx(quest_num) && !((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) || !(quest[quest_num].flags & QUEST_FLAG_PRESET))))
        return;

    byte new_feeling = get_dungeon_feeling(subject_ptr);
    subject_ptr->feeling_turn = current_world_ptr->game_turn;
    if (subject_ptr->feeling == new_feeling)
        return;

    subject_ptr->feeling = new_feeling;
    do_cmd_feeling(subject_ptr);
    select_floor_music(subject_ptr);
    subject_ptr->redraw |= (PR_DEPTH);
    if (disturb_minor)
        disturb(subject_ptr, FALSE, FALSE);
}

/*
 * Glow deep lava and building entrances in the floor
 */
void glow_deep_lava_and_bldg(player_type *subject_ptr)
{
    if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
        return;

    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            if (!have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_GLOW))
                continue;

            for (DIRECTION i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx))
                    continue;

                floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
            }
        }
    }

    subject_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    subject_ptr->redraw |= (PR_MAP);
}

/*
 * Actually erase the entire "lite" array, redrawing every grid
 */
void forget_lite(floor_type *floor_ptr)
{
    if (!floor_ptr->lite_n)
        return;

    for (int i = 0; i < floor_ptr->lite_n; i++) {
        POSITION y = floor_ptr->lite_y[i];
        POSITION x = floor_ptr->lite_x[i];
        floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);
    }

    floor_ptr->lite_n = 0;
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
    POSITION p = subject_ptr->cur_lite;
    grid_type *g_ptr;
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->lite_n; i++) {
        POSITION y = floor_ptr->lite_y[i];
        POSITION x = floor_ptr->lite_x[i];
        floor_ptr->grid_array[y][x].info &= ~(CAVE_LITE);
        floor_ptr->grid_array[y][x].info |= (CAVE_TEMP);
        tmp_pos.y[tmp_pos.n] = y;
        tmp_pos.x[tmp_pos.n] = x;
        tmp_pos.n++;
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
        g_ptr = &floor_ptr->grid_array[y][x];
        if (g_ptr->info & (CAVE_TEMP))
            continue;

        cave_note_and_redraw_later(floor_ptr, g_ptr, y, x);
    }

    for (int i = 0; i < tmp_pos.n; i++) {
        POSITION y = tmp_pos.y[i];
        POSITION x = tmp_pos.x[i];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_TEMP);
        if (g_ptr->info & (CAVE_LITE))
            continue;

        cave_redraw_later(floor_ptr, g_ptr, y, x);
    }

    tmp_pos.n = 0;
    subject_ptr->update |= (PU_DELAY_VIS);
}

/*
 * Clear the viewable space
 */
void forget_view(floor_type *floor_ptr)
{
    if (!floor_ptr->view_n)
        return;

    for (int i = 0; i < floor_ptr->view_n; i++) {
        POSITION y = floor_ptr->view_y[i];
        POSITION x = floor_ptr->view_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
    }

    floor_ptr->view_n = 0;
}

/*
 * Helper function for "update_view()" below
 *
 * We are checking the "viewability" of grid (y,x) by the player.
 *
 * This function assumes that (y,x) is legal (i.e. on the map).
 *
 * Grid (y1,x1) is on the "diagonal" between (subject_ptr->y,subject_ptr->x) and (y,x)
 * Grid (y2,x2) is "adjacent", also between (subject_ptr->y,subject_ptr->x) and (y,x).
 *
 * Note that we are using the "CAVE_XTRA" field for marking grids as
 * "easily viewable".  This bit is cleared at the end of "update_view()".
 *
 * This function adds (y,x) to the "viewable set" if necessary.
 *
 * This function now returns "TRUE" if vision is "blocked" by grid (y,x).
 */
static bool update_view_aux(player_type *subject_ptr, POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    grid_type *g1_c_ptr;
    grid_type *g2_c_ptr;
    g1_c_ptr = &floor_ptr->grid_array[y1][x1];
    g2_c_ptr = &floor_ptr->grid_array[y2][x2];
    bool f1 = (cave_los_grid(g1_c_ptr));
    bool f2 = (cave_los_grid(g2_c_ptr));
    if (!f1 && !f2)
        return TRUE;

    bool v1 = (f1 && (g1_c_ptr->info & (CAVE_VIEW)));
    bool v2 = (f2 && (g2_c_ptr->info & (CAVE_VIEW)));
    if (!v1 && !v2)
        return TRUE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    bool wall = (!cave_los_grid(g_ptr));
    bool z1 = (v1 && (g1_c_ptr->info & (CAVE_XTRA)));
    bool z2 = (v2 && (g2_c_ptr->info & (CAVE_XTRA)));
    if (z1 && z2) {
        g_ptr->info |= (CAVE_XTRA);
        cave_view_hack(floor_ptr, g_ptr, y, x);
        return wall;
    }

    if (z1) {
        cave_view_hack(floor_ptr, g_ptr, y, x);
        return wall;
    }

    if (v1 && v2) {
        cave_view_hack(floor_ptr, g_ptr, y, x);
        return wall;
    }

    if (wall) {
        cave_view_hack(floor_ptr, g_ptr, y, x);
        return wall;
    }

    if (los(subject_ptr, subject_ptr->y, subject_ptr->x, y, x)) {
        cave_view_hack(floor_ptr, g_ptr, y, x);
        return wall;
    }

    return TRUE;
}

/*
 * Calculate the viewable space
 *
 *  1: Process the player
 *  1a: The player is always (easily) viewable
 *  2: Process the diagonals
 *  2a: The diagonals are (easily) viewable up to the first wall
 *  2b: But never go more than 2/3 of the "full" distance
 *  3: Process the main axes
 *  3a: The main axes are (easily) viewable up to the first wall
 *  3b: But never go more than the "full" distance
 *  4: Process sequential "strips" in each of the eight octants
 *  4a: Each strip runs along the previous strip
 *  4b: The main axes are "previous" to the first strip
 *  4c: Process both "sides" of each "direction" of each strip
 *  4c1: Each side aborts as soon as possible
 *  4c2: Each side tells the next strip how far it has to check
 */
void update_view(player_type *subject_ptr)
{
    int n, m, d, k, z;
    POSITION y, x;

    int se, sw, ne, nw, es, en, ws, wn;

    int full, over;

    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    POSITION y_max = floor_ptr->height - 1;
    POSITION x_max = floor_ptr->width - 1;

    grid_type *g_ptr;
    if (view_reduce_view && !floor_ptr->dun_level) {
        full = MAX_SIGHT / 2;
        over = MAX_SIGHT * 3 / 4;
    } else {
        full = MAX_SIGHT;
        over = MAX_SIGHT * 3 / 2;
    }

    for (n = 0; n < floor_ptr->view_n; n++) {
        y = floor_ptr->view_y[n];
        x = floor_ptr->view_x[n];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
        g_ptr->info |= CAVE_TEMP;
        tmp_pos.y[tmp_pos.n] = y;
        tmp_pos.x[tmp_pos.n] = x;
        tmp_pos.n++;
    }

    floor_ptr->view_n = 0;
    y = subject_ptr->y;
    x = subject_ptr->x;
    g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->info |= CAVE_XTRA;
    cave_view_hack(floor_ptr, g_ptr, y, x);

    z = full * 2 / 3;
    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y + d, x + d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y + d, x - d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y - d, x + d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y - d, x - d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y + d, x);
        if (!cave_los_grid(g_ptr))
            break;
    }

    se = sw = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y - d, x);
        if (!cave_los_grid(g_ptr))
            break;
    }

    ne = nw = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y, x + d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    es = en = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, g_ptr, y, x - d);
        if (!cave_los_grid(g_ptr))
            break;
    }

    ws = wn = d;
    for (n = 1; n <= over / 2; n++) {
        POSITION ypn, ymn, xpn, xmn;
        z = over - n - n;
        if (z > full - n)
            z = full - n;

        while ((z + n + (n >> 1)) > full)
            z--;

        ypn = y + n;
        ymn = y - n;
        xpn = x + n;
        xmn = x - n;
        if (ypn < y_max) {
            m = MIN(z, y_max - ypn);
            if ((xpn <= x_max) && (n < se)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ypn + d, xpn, ypn + d - 1, xpn - 1, ypn + d - 1, xpn)) {
                        if (n + d >= se)
                            break;
                    } else
                        k = n + d;
                }

                se = k + 1;
            }

            if ((xmn >= 0) && (n < sw)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ypn + d, xmn, ypn + d - 1, xmn + 1, ypn + d - 1, xmn)) {
                        if (n + d >= sw)
                            break;
                    } else
                        k = n + d;
                }

                sw = k + 1;
            }
        }

        if (ymn > 0) {
            m = MIN(z, ymn);
            if ((xpn <= x_max) && (n < ne)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ymn - d, xpn, ymn - d + 1, xpn - 1, ymn - d + 1, xpn)) {
                        if (n + d >= ne)
                            break;
                    } else
                        k = n + d;
                }

                ne = k + 1;
            }

            if ((xmn >= 0) && (n < nw)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ymn - d, xmn, ymn - d + 1, xmn + 1, ymn - d + 1, xmn)) {
                        if (n + d >= nw)
                            break;
                    } else
                        k = n + d;
                }

                nw = k + 1;
            }
        }

        if (xpn < x_max) {
            m = MIN(z, x_max - xpn);
            if ((ypn <= x_max) && (n < es)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ypn, xpn + d, ypn - 1, xpn + d - 1, ypn, xpn + d - 1)) {
                        if (n + d >= es)
                            break;
                    } else
                        k = n + d;
                }

                es = k + 1;
            }

            if ((ymn >= 0) && (n < en)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ymn, xpn + d, ymn + 1, xpn + d - 1, ymn, xpn + d - 1)) {
                        if (n + d >= en)
                            break;
                    } else
                        k = n + d;
                }

                en = k + 1;
            }
        }

        if (xmn > 0) {
            m = MIN(z, xmn);
            if ((ypn <= y_max) && (n < ws)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ypn, xmn - d, ypn - 1, xmn - d + 1, ypn, xmn - d + 1)) {
                        if (n + d >= ws)
                            break;
                    } else
                        k = n + d;
                }

                ws = k + 1;
            }

            if ((ymn >= 0) && (n < wn)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(subject_ptr, ymn, xmn - d, ymn + 1, xmn - d + 1, ymn, xmn - d + 1)) {
                        if (n + d >= wn)
                            break;
                    } else
                        k = n + d;
                }

                wn = k + 1;
            }
        }
    }

    for (n = 0; n < floor_ptr->view_n; n++) {
        y = floor_ptr->view_y[n];
        x = floor_ptr->view_x[n];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_XTRA);
        if (g_ptr->info & CAVE_TEMP)
            continue;

        cave_note_and_redraw_later(floor_ptr, g_ptr, y, x);
    }

    for (n = 0; n < tmp_pos.n; n++) {
        y = tmp_pos.y[n];
        x = tmp_pos.x[n];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_TEMP);
        if (g_ptr->info & CAVE_VIEW)
            continue;

        cave_redraw_later(floor_ptr, g_ptr, y, x);
    }

    tmp_pos.n = 0;
    subject_ptr->update |= (PU_DELAY_VIS);
}

/*!
 * @brief モンスターによる光量状態更新 / Add a square to the changes array
 * @param subject_ptr 主観となるクリーチャーの参照ポインタ
 * @param y Y座標
 * @param x X座標
 */
static void mon_lite_hack(player_type *subject_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    int dpf, d;
    POSITION midpoint;
    g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW)
        return;

    if (!cave_los_grid(g_ptr)) {
        if (((y < subject_ptr->y) && (y > mon_fy)) || ((y > subject_ptr->y) && (y < mon_fy))) {
            dpf = subject_ptr->y - mon_fy;
            d = y - mon_fy;
            midpoint = mon_fx + ((subject_ptr->x - mon_fx) * ABS(d)) / ABS(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1))
                    return;
            } else if (x > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1))
                    return;
            } else if (mon_invis)
                return;
        }

        if (((x < subject_ptr->x) && (x > mon_fx)) || ((x > subject_ptr->x) && (x < mon_fx))) {
            dpf = subject_ptr->x - mon_fx;
            d = x - mon_fx;
            midpoint = mon_fy + ((subject_ptr->y - mon_fy) * ABS(d)) / ABS(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x))
                    return;
            } else if (y > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x))
                    return;
            } else if (mon_invis)
                return;
        }
    }

    if (!(g_ptr->info & CAVE_MNDK)) {
        tmp_pos.x[tmp_pos.n] = x;
        tmp_pos.y[tmp_pos.n] = y;
        tmp_pos.n++;
    } else {
        g_ptr->info &= ~(CAVE_MNDK);
    }

    g_ptr->info |= CAVE_MNLT;
}

/*
 * Add a square to the changes array
 */
static void mon_dark_hack(player_type *subject_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    int midpoint, dpf, d;
    g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_LITE | CAVE_MNLT | CAVE_MNDK | CAVE_VIEW)) != CAVE_VIEW)
        return;

    if (!cave_los_grid(g_ptr) && !cave_have_flag_grid(g_ptr, FF_PROJECT)) {
        if (((y < subject_ptr->y) && (y > mon_fy)) || ((y > subject_ptr->y) && (y < mon_fy))) {
            dpf = subject_ptr->y - mon_fy;
            d = y - mon_fy;
            midpoint = mon_fx + ((subject_ptr->x - mon_fx) * ABS(d)) / ABS(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y, x + 1, FF_PROJECT))
                    return;
            } else if (x > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y, x - 1, FF_PROJECT))
                    return;
            } else if (mon_invis)
                return;
        }

        if (((x < subject_ptr->x) && (x > mon_fx)) || ((x > subject_ptr->x) && (x < mon_fx))) {
            dpf = subject_ptr->x - mon_fx;
            d = x - mon_fx;
            midpoint = mon_fy + ((subject_ptr->y - mon_fy) * ABS(d)) / ABS(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y + 1, x, FF_PROJECT))
                    return;
            } else if (y > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x) && !cave_have_flag_bold(subject_ptr->current_floor_ptr, y - 1, x, FF_PROJECT))
                    return;
            } else if (mon_invis)
                return;
        }
    }

    tmp_pos.x[tmp_pos.n] = x;
    tmp_pos.y[tmp_pos.n] = y;
    tmp_pos.n++;
    g_ptr->info |= CAVE_MNDK;
}

/*
 * Update squares illuminated or darkened by monsters.
 *
 * Hack - use the CAVE_ROOM flag (renamed to be CAVE_MNLT) to
 * denote squares illuminated by monsters.
 *
 * The CAVE_TEMP and CAVE_XTRA flag are used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 */
void update_mon_lite(player_type *subject_ptr)
{
    void (*add_mon_lite)(player_type *, POSITION, POSITION);
    int dis_lim = ((d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto) ? (MAX_SIGHT / 2 + 1) : (MAX_SIGHT + 3);
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];
        g_ptr->info |= (g_ptr->info & CAVE_MNLT) ? CAVE_TEMP : CAVE_XTRA;
        g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    tmp_pos.n = 0;
    if (!current_world_ptr->timewalk_m_idx) {
        monster_type *m_ptr;
        monster_race *r_ptr;
        for (int i = 1; i < floor_ptr->m_max; i++) {
            m_ptr = &floor_ptr->m_list[i];
            r_ptr = &r_info[m_ptr->r_idx];
            if (!monster_is_valid(m_ptr) || (m_ptr->cdis > dis_lim))
                continue;

            int rad = 0;
            if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_SELF_LITE_1))
                rad++;

            if (r_ptr->flags7 & (RF7_HAS_LITE_2 | RF7_SELF_LITE_2))
                rad += 2;

            if (r_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_SELF_DARK_1))
                rad--;

            if (r_ptr->flags7 & (RF7_HAS_DARK_2 | RF7_SELF_DARK_2))
                rad -= 2;

            if (!rad)
                continue;

            int f_flag;
            if (rad > 0) {
                if (!(r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2))
                    && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && is_daytime()) || subject_ptr->phase_out))
                    continue;

                if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
                    rad = 1;

                add_mon_lite = mon_lite_hack;
                f_flag = FF_LOS;
            } else {
                if (!(r_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2)) && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && !is_daytime())))
                    continue;

                add_mon_lite = mon_dark_hack;
                f_flag = FF_PROJECT;
                rad = -rad;
            }

            mon_fx = m_ptr->fx;
            mon_fy = m_ptr->fy;
            mon_invis = !(floor_ptr->grid_array[mon_fy][mon_fx].info & CAVE_VIEW);
            add_mon_lite(subject_ptr, mon_fy, mon_fx);
            add_mon_lite(subject_ptr, mon_fy + 1, mon_fx);
            add_mon_lite(subject_ptr, mon_fy - 1, mon_fx);
            add_mon_lite(subject_ptr, mon_fy, mon_fx + 1);
            add_mon_lite(subject_ptr, mon_fy, mon_fx - 1);
            add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 1);
            add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 1);
            add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 1);
            add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 1);
            if (rad < 2)
                continue;

            grid_type *g_ptr;
            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx, f_flag)) {
                add_mon_lite(subject_ptr, mon_fy + 2, mon_fx + 1);
                add_mon_lite(subject_ptr, mon_fy + 2, mon_fx);
                add_mon_lite(subject_ptr, mon_fy + 2, mon_fx - 1);
                g_ptr = &floor_ptr->grid_array[mon_fy + 2][mon_fx];
                if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, mon_fy + 3, mon_fx + 1);
                    add_mon_lite(subject_ptr, mon_fy + 3, mon_fx);
                    add_mon_lite(subject_ptr, mon_fy + 3, mon_fx - 1);
                }
            }

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx, f_flag)) {
                add_mon_lite(subject_ptr, mon_fy - 2, mon_fx + 1);
                add_mon_lite(subject_ptr, mon_fy - 2, mon_fx);
                add_mon_lite(subject_ptr, mon_fy - 2, mon_fx - 1);
                g_ptr = &floor_ptr->grid_array[mon_fy - 2][mon_fx];
                if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, mon_fy - 3, mon_fx + 1);
                    add_mon_lite(subject_ptr, mon_fy - 3, mon_fx);
                    add_mon_lite(subject_ptr, mon_fy - 3, mon_fx - 1);
                }
            }

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy, mon_fx + 1, f_flag)) {
                add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 2);
                add_mon_lite(subject_ptr, mon_fy, mon_fx + 2);
                add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 2);
                g_ptr = &floor_ptr->grid_array[mon_fy][mon_fx + 2];
                if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, mon_fy + 1, mon_fx + 3);
                    add_mon_lite(subject_ptr, mon_fy, mon_fx + 3);
                    add_mon_lite(subject_ptr, mon_fy - 1, mon_fx + 3);
                }
            }

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy, mon_fx - 1, f_flag)) {
                add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 2);
                add_mon_lite(subject_ptr, mon_fy, mon_fx - 2);
                add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 2);
                g_ptr = &floor_ptr->grid_array[mon_fy][mon_fx - 2];
                if ((rad == 3) && cave_have_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, mon_fy + 1, mon_fx - 3);
                    add_mon_lite(subject_ptr, mon_fy, mon_fx - 3);
                    add_mon_lite(subject_ptr, mon_fy - 1, mon_fx - 3);
                }
            }

            if (rad != 3)
                continue;

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx + 1, f_flag))
                add_mon_lite(subject_ptr, mon_fy + 2, mon_fx + 2);

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy + 1, mon_fx - 1, f_flag))
                add_mon_lite(subject_ptr, mon_fy + 2, mon_fx - 2);

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx + 1, f_flag))
                add_mon_lite(subject_ptr, mon_fy - 2, mon_fx + 2);

            if (cave_have_flag_bold(subject_ptr->current_floor_ptr, mon_fy - 1, mon_fx - 1, f_flag))
                add_mon_lite(subject_ptr, mon_fy - 2, mon_fx - 2);
        }
    }

    s16b end_temp = tmp_pos.n;
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        POSITION fx = floor_ptr->mon_lite_x[i];
        POSITION fy = floor_ptr->mon_lite_y[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_TEMP) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW)
                cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW)
            cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);

        tmp_pos.x[tmp_pos.n] = fx;
        tmp_pos.y[tmp_pos.n] = fy;
        tmp_pos.n++;
    }

    floor_ptr->mon_lite_n = 0;
    for (int i = 0; i < end_temp; i++) {
        POSITION fx = tmp_pos.x[i];
        POSITION fy = tmp_pos.y[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_MNLT) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW)
                cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW)
            cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);

        floor_ptr->mon_lite_x[floor_ptr->mon_lite_n] = fx;
        floor_ptr->mon_lite_y[floor_ptr->mon_lite_n] = fy;
        floor_ptr->mon_lite_n++;
    }

    for (int i = end_temp; i < tmp_pos.n; i++)
        floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]].info &= ~(CAVE_TEMP | CAVE_XTRA);

    tmp_pos.n = 0;
    subject_ptr->update |= (PU_DELAY_VIS);
    subject_ptr->monlite = (floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_MNLT) ? TRUE : FALSE;
    if (!(subject_ptr->special_defense & NINJA_S_STEALTH)) {
        subject_ptr->old_monlite = subject_ptr->monlite;
        return;
    }

    if (subject_ptr->old_monlite == subject_ptr->monlite) {
        subject_ptr->old_monlite = subject_ptr->monlite;
        return;
    }

    if (subject_ptr->monlite)
        msg_print(_("影の覆いが薄れた気がする。", "Your mantle of shadow becomes thin."));
    else
        msg_print(_("影の覆いが濃くなった！", "Your mantle of shadow is restored to its original darkness."));
    
    subject_ptr->old_monlite = subject_ptr->monlite;
}

void clear_mon_lite(floor_type *floor_ptr)
{
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];
        g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    floor_ptr->mon_lite_n = 0;
}
