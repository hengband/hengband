﻿/*!
 * @brief フロアに影響のある魔法の処理
 * @date 2019/02/21
 * @author deskull
 */

#include "spell-kind/spells-floor.h"
#include "action/travel-execution.h"
#include "cmd-io/cmd-dump.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*
 * @brief 啓蒙/陽光召喚処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param ninja 忍者かどうか
 * @return なし
 */
void wiz_lite(player_type *caster_ptr, bool ninja)
{
    /* Memorize objects */
    for (OBJECT_IDX i = 1; i < caster_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];
        if (!object_is_valid(o_ptr))
            continue;
        if (object_is_held_monster(o_ptr))
            continue;
        o_ptr->marked |= OM_FOUND;
    }

    /* Scan all normal grids */
    for (POSITION y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++) {
        /* Scan all normal grids */
        for (POSITION x = 1; x < caster_ptr->current_floor_ptr->width - 1; x++) {
            grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

            /* Memorize terrain of the grid */
            g_ptr->info |= (CAVE_KNOWN);

            /* Feature code (applying "mimic" field) */
            FEAT_IDX feat = get_feat_mimic(g_ptr);
            feature_type *f_ptr;
            f_ptr = &f_info[feat];

            /* Process all non-walls */
            if (has_flag(f_ptr->flags, FF_WALL))
                continue;

            /* Scan all neighbors */
            for (OBJECT_IDX i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                g_ptr = &caster_ptr->current_floor_ptr->grid_array[yy][xx];

                /* Feature code (applying "mimic" field) */
                f_ptr = &f_info[get_feat_mimic(g_ptr)];

                /* Perma-lite the grid */
                if (!(d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !ninja) {
                    g_ptr->info |= (CAVE_GLOW);
                }

                /* Memorize normal features */
                if (has_flag(f_ptr->flags, FF_REMEMBER)) {
                    /* Memorize the grid */
                    g_ptr->info |= (CAVE_MARK);
                }

                /* Perma-lit grids (newly and previously) */
                else if (g_ptr->info & CAVE_GLOW) {
                    /* Normally, memorize floors (see above) */
                    if (view_perma_grids && !view_torch_grids) {
                        /* Memorize the grid */
                        g_ptr->info |= (CAVE_MARK);
                    }
                }
            }
        }
    }

    caster_ptr->update |= (PU_MONSTERS);
    caster_ptr->redraw |= (PR_MAP);
    caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

    if (caster_ptr->special_defense & NINJA_S_STEALTH) {
        if (caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info & CAVE_GLOW)
            set_superstealth(caster_ptr, FALSE);
    }
}

/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(player_type *caster_ptr)
{
    /* Forget every grid */
    for (POSITION y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x < caster_ptr->current_floor_ptr->width - 1; x++) {
            grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

            /* Process the grid */
            g_ptr->info &= ~(CAVE_MARK | CAVE_IN_DETECT | CAVE_KNOWN);
            g_ptr->info |= (CAVE_UNSAFE);
        }
    }

    /* Forget every grid on horizontal edge */
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        caster_ptr->current_floor_ptr->grid_array[0][x].info &= ~(CAVE_MARK);
        caster_ptr->current_floor_ptr->grid_array[caster_ptr->current_floor_ptr->height - 1][x].info &= ~(CAVE_MARK);
    }

    /* Forget every grid on vertical edge */
    for (POSITION y = 1; y < (caster_ptr->current_floor_ptr->height - 1); y++) {
        caster_ptr->current_floor_ptr->grid_array[y][0].info &= ~(CAVE_MARK);
        caster_ptr->current_floor_ptr->grid_array[y][caster_ptr->current_floor_ptr->width - 1].info &= ~(CAVE_MARK);
    }

    /* Forget all objects */
    for (OBJECT_IDX i = 1; i < caster_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

        if (!object_is_valid(o_ptr))
            continue;
        if (object_is_held_monster(o_ptr))
            continue;

        /* Forget the object */
        o_ptr->marked &= OM_TOUCHED;
    }

    /* Forget travel route when we have forgotten map */
    forget_travel_flow(caster_ptr->current_floor_ptr);

    caster_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
    caster_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    caster_ptr->update |= (PU_MONSTERS);
    caster_ptr->redraw |= (PR_MAP);
    caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
}

/*
 * Hack -- map the current panel (plus some) ala "magic mapping"
 */
void map_area(player_type *caster_ptr, POSITION range)
{
    if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
        range /= 3;

    /* Scan that area */
    for (POSITION y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x < caster_ptr->current_floor_ptr->width - 1; x++) {
            if (distance(caster_ptr->y, caster_ptr->x, y, x) > range)
                continue;

            grid_type *g_ptr;
            g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

            /* Memorize terrain of the grid */
            g_ptr->info |= (CAVE_KNOWN);

            /* Feature code (applying "mimic" field) */
            FEAT_IDX feat = get_feat_mimic(g_ptr);
            feature_type *f_ptr;
            f_ptr = &f_info[feat];

            /* All non-walls are "checked" */
            if (has_flag(f_ptr->flags, FF_WALL))
                continue;

            /* Memorize normal features */
            if (has_flag(f_ptr->flags, FF_REMEMBER)) {
                /* Memorize the object */
                g_ptr->info |= (CAVE_MARK);
            }

            /* Memorize known walls */
            for (int i = 0; i < 8; i++) {
                g_ptr = &caster_ptr->current_floor_ptr->grid_array[y + ddy_ddd[i]][x + ddx_ddd[i]];

                /* Feature code (applying "mimic" field) */
                feat = get_feat_mimic(g_ptr);
                f_ptr = &f_info[feat];

                /* Memorize walls (etc) */
                if (has_flag(f_ptr->flags, FF_REMEMBER)) {
                    /* Memorize the walls */
                    g_ptr->info |= (CAVE_MARK);
                }
            }
        }
    }

    caster_ptr->redraw |= (PR_MAP);
    caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
}

/*!
 * @brief *破壊*処理を行う / The spell of destruction
 * @param y1 破壊の中心Y座標
 * @param x1 破壊の中心X座標
 * @param r 破壊の半径
 * @param in_generate ダンジョンフロア生成中の処理ならばTRUE
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 * </pre>
 */
bool destroy_area(player_type *caster_ptr, POSITION y1, POSITION x1, POSITION r, bool in_generate)
{
    /* Prevent destruction of quest levels and town */
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if ((floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest)) || !floor_ptr->dun_level) {
        return FALSE;
    }

    /* Lose monster light */
    if (!in_generate)
        clear_mon_lite(floor_ptr);

    /* Big area of affect */
    bool flag = FALSE;
    for (POSITION y = (y1 - r); y <= (y1 + r); y++) {
        for (POSITION x = (x1 - r); x <= (x1 + r); x++) {
            if (!in_bounds(floor_ptr, y, x))
                continue;

            /* Extract the distance */
            int k = distance(y1, x1, y, x);

            /* Stay in the circle of death */
            if (k > r)
                continue;
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];

            /* Lose room and vault */
            g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

            /* Lose light and knowledge */
            g_ptr->info &= ~(CAVE_MARK | CAVE_GLOW | CAVE_KNOWN);

            if (!in_generate) /* Normal */
            {
                /* Lose unsafety */
                g_ptr->info &= ~(CAVE_UNSAFE);

                /* Hack -- Notice player affect */
                if (player_bold(caster_ptr, y, x)) {
                    /* Hurt the player later */
                    flag = TRUE;

                    /* Do not hurt this grid */
                    continue;
                }
            }

            /* Hack -- Skip the epicenter */
            if ((y == y1) && (x == x1))
                continue;

            if (g_ptr->m_idx) {
                monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
                monster_race *r_ptr = &r_info[m_ptr->r_idx];

                if (in_generate) /* In generation */
                {
                    /* Delete the monster (if any) */
                    delete_monster(caster_ptr, y, x);
                } else if (r_ptr->flags1 & RF1_QUESTOR) {
                    /* Heal the monster */
                    m_ptr->hp = m_ptr->maxhp;

                    /* Try to teleport away quest monsters */
                    if (!teleport_away(caster_ptr, g_ptr->m_idx, (r * 2) + 1, TELEPORT_DEC_VALOUR))
                        continue;
                } else {
                    if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
                        GAME_TEXT m_name[MAX_NLEN];

                        monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                        exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_DESTROY, m_name);
                    }

                    /* Delete the monster (if any) */
                    delete_monster(caster_ptr, y, x);
                }
            }

            /* During generation, destroyed artifacts are "preserved" */
            if (preserve_mode || in_generate) {
                OBJECT_IDX this_o_idx, next_o_idx = 0;

                /* Scan all objects in the grid */
                for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
                    object_type *o_ptr;
                    o_ptr = &floor_ptr->o_list[this_o_idx];
                    next_o_idx = o_ptr->next_o_idx;

                    /* Hack -- Preserve unknown artifacts */
                    if (object_is_fixed_artifact(o_ptr) && (!object_is_known(o_ptr) || in_generate)) {
                        /* Mega-Hack -- Preserve the artifact */
                        a_info[o_ptr->name1].cur_num = 0;

                        if (in_generate && cheat_peek) {
                            GAME_TEXT o_name[MAX_NLEN];
                            describe_flavor(caster_ptr, o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));
                            msg_format(_("伝説のアイテム (%s) は生成中に*破壊*された。", "Artifact (%s) was *destroyed* during generation."), o_name);
                        }
                    } else if (in_generate && cheat_peek && o_ptr->art_name) {
                        msg_print(
                            _("ランダム・アーティファクトの1つは生成中に*破壊*された。", "One of the random artifacts was *destroyed* during generation."));
                    }
                }
            }

            delete_all_items_from_floor(caster_ptr, y, x);

            /* Destroy "non-permanent" grids */
            if (cave_has_flag_grid(g_ptr, FF_PERMANENT))
                continue;

            /* Wall (or floor) type */
            int t = randint0(200);

            if (!in_generate) /* Normal */
            {
                if (t < 20) {
                    /* Create granite wall */
                    cave_set_feat(caster_ptr, y, x, feat_granite);
                } else if (t < 70) {
                    /* Create quartz vein */
                    cave_set_feat(caster_ptr, y, x, feat_quartz_vein);
                } else if (t < 100) {
                    /* Create magma vein */
                    cave_set_feat(caster_ptr, y, x, feat_magma_vein);
                } else {
                    /* Create floor */
                    cave_set_feat(caster_ptr, y, x, feat_ground_type[randint0(100)]);
                }

                continue;
            }

            if (t < 20) {
                /* Create granite wall */
                place_grid(caster_ptr, g_ptr, GB_EXTRA);
            } else if (t < 70) {
                /* Create quartz vein */
                g_ptr->feat = feat_quartz_vein;
            } else if (t < 100) {
                /* Create magma vein */
                g_ptr->feat = feat_magma_vein;
            } else {
                /* Create floor */
                place_grid(caster_ptr, g_ptr, GB_FLOOR);
            }

            /* Clear garbage of hidden trap or door */
            g_ptr->mimic = 0;
        }
    }

    if (in_generate)
        return TRUE;

    /* Process "re-glowing" */
    for (POSITION y = (y1 - r); y <= (y1 + r); y++) {
        for (POSITION x = (x1 - r); x <= (x1 + r); x++) {
            if (!in_bounds(floor_ptr, y, x))
                continue;

            /* Extract the distance */
            int k = distance(y1, x1, y, x);

            /* Stay in the circle of death */
            if (k > r)
                continue;
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];

            if (is_mirror_grid(g_ptr)) {
                g_ptr->info |= CAVE_GLOW;
                continue;
            }

            if ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
                continue;

            DIRECTION i;
            POSITION yy, xx;
            grid_type *cc_ptr;

            for (i = 0; i < 9; i++) {
                yy = y + ddy_ddd[i];
                xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx))
                    continue;
                cc_ptr = &floor_ptr->grid_array[yy][xx];
                if (has_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW)) {
                    g_ptr->info |= CAVE_GLOW;
                    break;
                }
            }
        }
    }

    /* Hack -- Affect player */
    if (flag) {
        msg_print(_("燃えるような閃光が発生した！", "There is a searing blast of light!"));

        /* Blind the player */
        if (!has_resist_blind(caster_ptr) && !has_resist_lite(caster_ptr)) {
            /* Become blind */
            (void)set_blind(caster_ptr, caster_ptr->blind + 10 + randint1(10));
        }
    }

    forget_flow(floor_ptr);

    /* Mega-Hack -- Forget the view and lite */
    caster_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
    caster_ptr->redraw |= (PR_MAP);
    caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

    if (caster_ptr->special_defense & NINJA_S_STEALTH) {
        if (floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info & CAVE_GLOW)
            set_superstealth(caster_ptr, FALSE);
    }

    return TRUE;
}
