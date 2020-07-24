/*!
 * @brief 雑多なその他の処理2 / effects of various "objects"
 * @date 2014/02/06
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "target/targeting.h"
#include "action/travel-execution.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/object-scanner.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/command-repeater.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/object-curse.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "spell/spells-summon.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-monster-status.h"
#include "window/main-window-util.h"
#include "world/world.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

bool show_gold_on_floor = FALSE;

/*!
 * @brief コンソール上におけるマップ表示の左上位置を返す /
 * Calculates current boundaries Called below and from "do_cmd_locate()".
 * @return なし
 */
void panel_bounds_center(void)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    panel_row_max = panel_row_min + hgt - 1;
    panel_row_prt = panel_row_min - 1;
    panel_col_max = panel_col_min + wid - 1;
    panel_col_prt = panel_col_min - 13;
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(player_type *creature_ptr, POSITION y, POSITION x)
{
    POSITION dy = 0, dx = 0;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    if (y < panel_row_min)
        dy = -1;

    if (y > panel_row_max)
        dy = 1;

    if (x < panel_col_min)
        dx = -1;

    if (x > panel_col_max)
        dx = 1;

    if (!dy && !dx)
        return FALSE;

    return change_panel(creature_ptr, dy, dx);
}

/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(player_type *creature_ptr)
{
    POSITION y = creature_ptr->y;
    POSITION x = creature_ptr->x;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    int max_prow_min = creature_ptr->current_floor_ptr->height - hgt;
    int max_pcol_min = creature_ptr->current_floor_ptr->width - wid;
    if (max_prow_min < 0)
        max_prow_min = 0;
    if (max_pcol_min < 0)
        max_pcol_min = 0;

    int prow_min;
    int pcol_min;
    if (center_player && (center_running || !creature_ptr->running)) {
        prow_min = y - hgt / 2;
        if (prow_min < 0)
            prow_min = 0;
        else if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        pcol_min = x - wid / 2;
        if (pcol_min < 0)
            pcol_min = 0;
        else if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;
    } else {
        prow_min = panel_row_min;
        pcol_min = panel_col_min;
        if (y > panel_row_max - 2)
            while (y > prow_min + hgt - 1 - 2)
                prow_min += (hgt / 2);

        if (y < panel_row_min + 2)
            while (y < prow_min + 2)
                prow_min -= (hgt / 2);

        if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        if (prow_min < 0)
            prow_min = 0;

        if (x > panel_col_max - 4)
            while (x > pcol_min + wid - 1 - 4)
                pcol_min += (wid / 2);

        if (x < panel_col_min + 4)
            while (x < pcol_min + 4)
                pcol_min -= (wid / 2);

        if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;

        if (pcol_min < 0)
            pcol_min = 0;
    }

    if ((prow_min == panel_row_min) && (pcol_min == panel_col_min))
        return;

    panel_row_min = prow_min;
    panel_col_min = pcol_min;
    if (disturb_panel && !center_player)
        disturb(creature_ptr, FALSE, FALSE);

    panel_bounds_center();
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}

/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(player_type *creature_ptr, MONSTER_IDX m_idx)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    if (!monster_is_valid(m_ptr))
        return FALSE;

    if (creature_ptr->image)
        return FALSE;

    if (!m_ptr->ml)
        return FALSE;

    if (creature_ptr->riding && (creature_ptr->riding == m_idx))
        return TRUE;

    if (!projectable(creature_ptr, creature_ptr->y, creature_ptr->x, m_ptr->fy, m_ptr->fx))
        return FALSE;

    return TRUE;
}

/* Targetting variables */
MONSTER_IDX target_who;
POSITION target_col;
POSITION target_row;

/*
 * Update (if necessary) and verify (if possible) the target.
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(player_type *creature_ptr)
{
    if (target_who < 0)
        return TRUE;

    if (target_who <= 0)
        return FALSE;

    if (!target_able(creature_ptr, target_who))
        return FALSE;

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[target_who];
    target_row = m_ptr->fy;
    target_col = m_ptr->fx;
    return TRUE;
}

/*
 * Determine if a given location is "interesting"
 */
static bool target_set_accept(player_type *creature_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x)))
        return FALSE;

    if (player_bold(creature_ptr, y, x))
        return TRUE;

    if (creature_ptr->image)
        return FALSE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->m_idx) {
        monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (m_ptr->ml)
            return TRUE;
    }

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (o_ptr->marked & OM_FOUND)
            return TRUE;
    }

    if (g_ptr->info & (CAVE_MARK)) {
        if (g_ptr->info & CAVE_OBJECT)
            return TRUE;

        if (have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_NOTICE))
            return TRUE;
    }

    return FALSE;
}

/*
 * Prepare the "temp" array for "target_set"
 *
 * Return the number of target_able monsters in the set.
 */
static void target_set_prepare(player_type *creature_ptr, BIT_FLAGS mode)
{
    POSITION min_hgt, max_hgt, min_wid, max_wid;
    if (mode & TARGET_KILL) {
        min_hgt = MAX((creature_ptr->y - get_max_range(creature_ptr)), 0);
        max_hgt = MIN((creature_ptr->y + get_max_range(creature_ptr)), creature_ptr->current_floor_ptr->height - 1);
        min_wid = MAX((creature_ptr->x - get_max_range(creature_ptr)), 0);
        max_wid = MIN((creature_ptr->x + get_max_range(creature_ptr)), creature_ptr->current_floor_ptr->width - 1);
    } else {
        min_hgt = panel_row_min;
        max_hgt = panel_row_max;
        min_wid = panel_col_min;
        max_wid = panel_col_max;
    }

    tmp_pos.n = 0;
    for (POSITION y = min_hgt; y <= max_hgt; y++) {
        for (POSITION x = min_wid; x <= max_wid; x++) {
            grid_type *g_ptr;
            if (!target_set_accept(creature_ptr, y, x))
                continue;

            g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
            if ((mode & (TARGET_KILL)) && !target_able(creature_ptr, g_ptr->m_idx))
                continue;

            if ((mode & (TARGET_KILL)) && !target_pet && is_pet(&creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx]))
                continue;

            tmp_pos.x[tmp_pos.n] = x;
            tmp_pos.y[tmp_pos.n] = y;
            tmp_pos.n++;
        }
    }

    if (mode & (TARGET_KILL)) {
        ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
    } else {
        ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_importance, ang_sort_swap_distance);
    }

    if (creature_ptr->riding == 0 || !target_pet || (tmp_pos.n <= 1) || !(mode & (TARGET_KILL)))
        return;

    POSITION tmp = tmp_pos.y[0];
    tmp_pos.y[0] = tmp_pos.y[1];
    tmp_pos.y[1] = tmp;
    tmp = tmp_pos.x[0];
    tmp_pos.x[0] = tmp_pos.x[1];
    tmp_pos.x[1] = tmp;
}

void target_set_prepare_look(player_type *creature_ptr) { target_set_prepare(creature_ptr, TARGET_LOOK); }

/*
 * Evaluate number of kill needed to gain level
 */
static void evaluate_monster_exp(player_type *creature_ptr, char *buf, monster_type *m_ptr)
{
    monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
    if ((creature_ptr->lev >= PY_MAX_LEVEL) || (creature_ptr->prace == RACE_ANDROID)) {
        sprintf(buf, "**");
        return;
    }

    if (!ap_r_ptr->r_tkills || (m_ptr->mflag2 & MFLAG2_KAGE)) {
        if (!current_world_ptr->wizard) {
            sprintf(buf, "??");
            return;
        }
    }

    s32b exp_mon = ap_r_ptr->mexp * ap_r_ptr->level;
    u32b exp_mon_frac = 0;
    s64b_div(&exp_mon, &exp_mon_frac, 0, (creature_ptr->max_plv + 2));

    s32b exp_adv = player_exp[creature_ptr->lev - 1] * creature_ptr->expfact;
    u32b exp_adv_frac = 0;
    s64b_div(&exp_adv, &exp_adv_frac, 0, 100);

    s64b_sub(&exp_adv, &exp_adv_frac, creature_ptr->exp, creature_ptr->exp_frac);

    s64b_add(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);
    s64b_sub(&exp_adv, &exp_adv_frac, 0, 1);

    s64b_div(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);

    u32b num = MIN(999, exp_adv_frac);
    sprintf(buf, "%03ld", (long int)num);
}

/*
 * todo xとlで処理を分ける？
 * @brief xまたはlで指定したグリッドにあるアイテムやモンスターの説明を記述する
 * @param subject_ptr プレーヤーへの参照ポインタ
 * @param y 指定グリッドのY座標
 * @param x 指定グリッドのX座標
 * @param mode x (KILL)かl (LOOK)
 * @param info 記述用文字列
 * @return 入力キー
 */
static char examine_grid(player_type *subject_ptr, POSITION y, POSITION x, target_type mode, concptr info)
{
    OBJECT_IDX next_o_idx = 0;
    concptr s1 = "";
    concptr s2 = "";
    concptr s3 = "";
    concptr x_info = "";
    bool boring = TRUE;
    FEAT_IDX feat;
    feature_type *f_ptr;
    char query = '\001';
    char out_val[MAX_NLEN + 80];
    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = 0;
    if (easy_floor) {
        floor_num = scan_floor_items(subject_ptr, floor_list, y, x, 0x02, 0);
        if (floor_num)
            x_info = _("x物 ", "x,");
    }

    if (player_bold(subject_ptr, y, x)) {
#ifdef JP
        s1 = "あなたは";
        s2 = "の上";
        s3 = "にいる";
#else
        s1 = "You are ";
        s2 = "on ";
#endif
    } else {
        s1 = _("ターゲット:", "Target:");
    }

    if (subject_ptr->image) {
        concptr name = _("何か奇妙な物", "something strange");
#ifdef JP
        sprintf(out_val, "%s%s%s%s [%s]", s1, name, s2, s3, info);
#else
        sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif
        prt(out_val, 0, 0);
        move_cursor_relative(y, x);
        query = inkey();
        if ((query != '\r') && (query != '\n'))
            return query;

        return 0;
    }

    grid_type *g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];
    if (g_ptr->m_idx && subject_ptr->current_floor_ptr->m_list[g_ptr->m_idx].ml) {
        monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
        GAME_TEXT m_name[MAX_NLEN];
        bool recall = FALSE;
        boring = FALSE;
        monster_race_track(subject_ptr, m_ptr->ap_r_idx);
        health_track(subject_ptr, g_ptr->m_idx);
        handle_stuff(subject_ptr);
        while (TRUE) {
            char acount[10];
            if (recall) {
                screen_save();
                screen_roff(subject_ptr, m_ptr->ap_r_idx, 0);
                term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), x_info, info));
                query = inkey();
                screen_load();
                if (query != 'r')
                    break;

                recall = FALSE;
                continue;
            }

            evaluate_monster_exp(subject_ptr, acount, m_ptr);
#ifdef JP
            sprintf(out_val, "[%s]%s%s(%s)%s%s [r思 %s%s]", acount, s1, m_name, look_mon_desc(m_ptr, 0x01), s2, s3, x_info, info);
#else
            sprintf(out_val, "[%s]%s%s%s%s(%s) [r, %s%s]", acount, s1, s2, s3, m_name, look_mon_desc(m_ptr, 0x01), x_info, info);
#endif
            prt(out_val, 0, 0);
            move_cursor_relative(y, x);
            query = inkey();
            if (query != 'r')
                break;

            recall = TRUE;
        }

        if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x'))
            return query;

        if ((query == ' ') && !(mode & TARGET_LOOK))
            return query;

        s1 = _("それは", "It is ");
        if (ap_r_ptr->flags1 & (RF1_FEMALE))
            s1 = _("彼女は", "She is ");
        else if (ap_r_ptr->flags1 & (RF1_MALE))
            s1 = _("彼は", "He is ");

#ifdef JP
        s2 = "を";
        s3 = "持っている";
#else
        s2 = "carrying ";
#endif

        for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
            GAME_TEXT o_name[MAX_NLEN];
            object_type *o_ptr;
            o_ptr = &subject_ptr->current_floor_ptr->o_list[this_o_idx];
            next_o_idx = o_ptr->next_o_idx;
            describe_flavor(subject_ptr, o_name, o_ptr, 0);
#ifdef JP
            sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
            sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif
            prt(out_val, 0, 0);
            move_cursor_relative(y, x);
            query = inkey();
            if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x'))
                return query;

            if ((query == ' ') && !(mode & TARGET_LOOK))
                return query;

            s2 = _("をまた", "also carrying ");
        }

#ifdef JP
        s2 = "の上";
        s3 = "にいる";
#else
        s2 = "on ";
#endif
    }

    if (floor_num) {
        int min_width = 0;
        while (TRUE) {
            if (floor_num == 1) {
                GAME_TEXT o_name[MAX_NLEN];
                object_type *o_ptr;
                o_ptr = &subject_ptr->current_floor_ptr->o_list[floor_list[0]];
                describe_flavor(subject_ptr, o_name, o_ptr, 0);
#ifdef JP
                sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
                sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif
                prt(out_val, 0, 0);
                move_cursor_relative(y, x);
                query = inkey();
                return query;
            }

            if (boring) {
#ifdef JP
                sprintf(out_val, "%s %d個のアイテム%s%s ['x'で一覧, %s]", s1, (int)floor_num, s2, s3, info);
#else
                sprintf(out_val, "%s%s%sa pile of %d items [x,%s]", s1, s2, s3, (int)floor_num, info);
#endif
                prt(out_val, 0, 0);
                move_cursor_relative(y, x);
                query = inkey();
                if (query != 'x' && query != ' ')
                    return query;
            }

            while (TRUE) {
                int i;
                OBJECT_IDX o_idx;
                screen_save();
                show_gold_on_floor = TRUE;
                (void)show_floor_items(subject_ptr, 0, y, x, &min_width, 0);
                show_gold_on_floor = FALSE;
#ifdef JP
                sprintf(out_val, "%s %d個のアイテム%s%s [Enterで次へ, %s]", s1, (int)floor_num, s2, s3, info);
#else
                sprintf(out_val, "%s%s%sa pile of %d items [Enter,%s]", s1, s2, s3, (int)floor_num, info);
#endif
                prt(out_val, 0, 0);
                query = inkey();
                screen_load();
                if (query != '\n' && query != '\r')
                    return query;

                o_idx = g_ptr->o_idx;
                if (!(o_idx && subject_ptr->current_floor_ptr->o_list[o_idx].next_o_idx))
                    continue;

                excise_object_idx(subject_ptr->current_floor_ptr, o_idx);
                i = g_ptr->o_idx;
                while (subject_ptr->current_floor_ptr->o_list[i].next_o_idx)
                    i = subject_ptr->current_floor_ptr->o_list[i].next_o_idx;

                subject_ptr->current_floor_ptr->o_list[i].next_o_idx = o_idx;
            }
        }
    }

    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &subject_ptr->current_floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (o_ptr->marked & OM_FOUND) {
            GAME_TEXT o_name[MAX_NLEN];
            boring = FALSE;
            describe_flavor(subject_ptr, o_name, o_ptr, 0);
#ifdef JP
            sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
            sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif
            prt(out_val, 0, 0);
            move_cursor_relative(y, x);
            query = inkey();
            if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x'))
                return query;

            if ((query == ' ') && !(mode & TARGET_LOOK))
                return query;

            s1 = _("それは", "It is ");
            if (o_ptr->number != 1)
                s1 = _("それらは", "They are ");

#ifdef JP
            s2 = "の上";
            s3 = "に見える";
#else
            s2 = "on ";
#endif
        }
    }

    feat = get_feat_mimic(g_ptr);
    if (!(g_ptr->info & CAVE_MARK) && !player_can_see_bold(subject_ptr, y, x))
        feat = feat_none;

    f_ptr = &f_info[feat];
    if (!boring && !have_flag(f_ptr->flags, FF_REMEMBER)) {
        if ((query != '\r') && (query != '\n'))
            return query;

        return 0;
    }

    concptr name;
    if (have_flag(f_ptr->flags, FF_QUEST_ENTER)) {
        IDX old_quest = subject_ptr->current_floor_ptr->inside_quest;
        for (int j = 0; j < 10; j++)
            quest_text[j][0] = '\0';

        quest_text_line = 0;
        subject_ptr->current_floor_ptr->inside_quest = g_ptr->special;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(subject_ptr, "q_info.txt", 0, 0, 0, 0);
        name = format(_("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)"), quest[g_ptr->special].name, quest[g_ptr->special].level);
        subject_ptr->current_floor_ptr->inside_quest = old_quest;
    } else if (have_flag(f_ptr->flags, FF_BLDG) && !subject_ptr->current_floor_ptr->inside_arena) {
        name = building[f_ptr->subtype].name;
    } else if (have_flag(f_ptr->flags, FF_ENTRANCE)) {
        name = format(_("%s(%d階相当)", "%s(level %d)"), d_text + d_info[g_ptr->special].text, d_info[g_ptr->special].mindepth);
    } else if (have_flag(f_ptr->flags, FF_TOWN)) {
        name = town_info[g_ptr->special].name;
    } else if (subject_ptr->wild_mode && (feat == feat_floor)) {
        name = _("道", "road");
    } else {
        name = f_name + f_ptr->name;
    }

    if (*s2
        && ((!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY))
            || (!have_flag(f_ptr->flags, FF_LOS) && !have_flag(f_ptr->flags, FF_TREE)) || have_flag(f_ptr->flags, FF_TOWN))) {
        s2 = _("の中", "in ");
    }

    if (have_flag(f_ptr->flags, FF_STORE) || have_flag(f_ptr->flags, FF_QUEST_ENTER)
        || (have_flag(f_ptr->flags, FF_BLDG) && !subject_ptr->current_floor_ptr->inside_arena) || have_flag(f_ptr->flags, FF_ENTRANCE))
        s2 = _("の入口", "");
#ifdef JP
#else
    else if (have_flag(f_ptr->flags, FF_FLOOR) || have_flag(f_ptr->flags, FF_TOWN) || have_flag(f_ptr->flags, FF_SHALLOW) || have_flag(f_ptr->flags, FF_DEEP))
        s3 = "";
    else
        s3 = (is_a_vowel(name[0])) ? "an " : "a ";
#endif

    if (current_world_ptr->wizard) {
        char f_idx_str[32];
        if (g_ptr->mimic)
            sprintf(f_idx_str, "%d/%d", g_ptr->feat, g_ptr->mimic);
        else
            sprintf(f_idx_str, "%d", g_ptr->feat);

#ifdef JP
        sprintf(out_val, "%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", s1, name, s2, s3, info, (unsigned int)g_ptr->info, f_idx_str, g_ptr->dist, g_ptr->cost,
            g_ptr->when, (int)y, (int)x, travel.cost[y][x]);
#else
        sprintf(out_val, "%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", s1, s2, s3, name, info, g_ptr->info, f_idx_str, g_ptr->dist, g_ptr->cost, g_ptr->when, (int)y,
            (int)x);
#endif
    } else
#ifdef JP
        sprintf(out_val, "%s%s%s%s[%s]", s1, name, s2, s3, info);
#else
        sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif

    prt(out_val, 0, 0);
    move_cursor_relative(y, x);
    query = inkey();
    if ((query != '\r') && (query != '\n') && (query != ' '))
        return query;

    return 0;
}

/*
 * Help "select" a location (see below)
 */
static POSITION_IDX target_pick(POSITION y1, POSITION x1, POSITION dy, POSITION dx)
{
    POSITION_IDX b_i = -1, b_v = 9999;
    for (POSITION_IDX i = 0; i < tmp_pos.n; i++) {
        POSITION x2 = tmp_pos.x[i];
        POSITION y2 = tmp_pos.y[i];
        POSITION x3 = (x2 - x1);
        POSITION y3 = (y2 - y1);
        if (dx && (x3 * dx <= 0))
            continue;

        if (dy && (y3 * dy <= 0))
            continue;

        POSITION x4 = ABS(x3);
        POSITION y4 = ABS(y3);
        if (dy && !dx && (x4 > y4))
            continue;

        if (dx && !dy && (y4 > x4))
            continue;

        POSITION_IDX v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));
        if ((b_i >= 0) && (v >= b_v))
            continue;

        b_i = i;
        b_v = v;
    }

    return b_i;
}

/*
 * Handle "target" and "look".
 */
bool target_set(player_type *creature_ptr, target_type mode)
{
    int i, d, t, bd;
    POSITION y = creature_ptr->y;
    POSITION x = creature_ptr->x;
    bool done = FALSE;
    bool flag = TRUE;
    char query;
    char info[80];
    grid_type *g_ptr;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    target_who = 0;
    const char same_key = rogue_like_commands ? 'x' : 'l';
    target_set_prepare(creature_ptr, mode);
    int m = 0;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    while (!done) {
        if (flag && tmp_pos.n) {
            y = tmp_pos.y[m];
            x = tmp_pos.x[m];
            change_panel_xy(creature_ptr, y, x);
            if (!(mode & TARGET_LOOK))
                print_path(creature_ptr, y, x);

            g_ptr = &floor_ptr->grid_array[y][x];
            if (target_able(creature_ptr, g_ptr->m_idx))
                strcpy(info, _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>"));
            else
                strcpy(info, _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>"));

            if (cheat_sight) {
                char cheatinfo[30];
                sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, y, x),
                    projectable(creature_ptr, creature_ptr->y, creature_ptr->x, y, x));
                strcat(info, cheatinfo);
            }

            while (TRUE) {
                query = examine_grid(creature_ptr, y, x, mode, info);
                if (query)
                    break;
            }

            d = 0;
            if (use_menu) {
                if (query == '\r')
                    query = 't';
            }

            switch (query) {
            case ESCAPE:
            case 'q': {
                done = TRUE;
                break;
            }
            case 't':
            case '.':
            case '5':
            case '0': {
                if (!target_able(creature_ptr, g_ptr->m_idx)) {
                    bell();
                    break;
                }

                health_track(creature_ptr, g_ptr->m_idx);
                target_who = g_ptr->m_idx;
                target_row = y;
                target_col = x;
                done = TRUE;
                break;
            }
            case ' ':
            case '*':
            case '+': {
                if (++m == tmp_pos.n) {
                    m = 0;
                    if (!expand_list)
                        done = TRUE;
                }

                break;
            }
            case '-': {
                if (m-- == 0) {
                    m = tmp_pos.n - 1;
                    if (!expand_list)
                        done = TRUE;
                }

                break;
            }
            case 'p': {
                verify_panel(creature_ptr);
                creature_ptr->update |= (PU_MONSTERS);
                creature_ptr->redraw |= (PR_MAP);
                creature_ptr->window |= (PW_OVERHEAD);
                handle_stuff(creature_ptr);
                target_set_prepare(creature_ptr, mode);
                y = creature_ptr->y;
                x = creature_ptr->x;
            }
                /* Fall through */
            case 'o':
                flag = FALSE;
                break;
            case 'm':
                break;
            default: {
                if (query == same_key) {
                    if (++m == tmp_pos.n) {
                        m = 0;
                        if (!expand_list)
                            done = TRUE;
                    }
                } else {
                    d = get_keymap_dir(query);
                    if (!d)
                        bell();

                    break;
                }
            }
            }

            if (d) {
                POSITION y2 = panel_row_min;
                POSITION x2 = panel_col_min;
                i = target_pick(tmp_pos.y[m], tmp_pos.x[m], ddy[d], ddx[d]);
                while (flag && (i < 0)) {
                    if (change_panel(creature_ptr, ddy[d], ddx[d])) {
                        int v = tmp_pos.y[m];
                        int u = tmp_pos.x[m];
                        target_set_prepare(creature_ptr, mode);
                        flag = TRUE;
                        i = target_pick(v, u, ddy[d], ddx[d]);
                        if (i >= 0)
                            m = i;

                        continue;
                    }

                    POSITION dx = ddx[d];
                    POSITION dy = ddy[d];
                    panel_row_min = y2;
                    panel_col_min = x2;
                    panel_bounds_center();
                    creature_ptr->update |= (PU_MONSTERS);
                    creature_ptr->redraw |= (PR_MAP);
                    creature_ptr->window |= (PW_OVERHEAD);
                    handle_stuff(creature_ptr);
                    target_set_prepare(creature_ptr, mode);
                    flag = FALSE;
                    x += dx;
                    y += dy;
                    if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0)))
                        dx = 0;

                    if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0)))
                        dy = 0;

                    if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min)) {
                        if (change_panel(creature_ptr, dy, dx))
                            target_set_prepare(creature_ptr, mode);
                    }

                    if (x >= floor_ptr->width - 1)
                        x = floor_ptr->width - 2;
                    else if (x <= 0)
                        x = 1;

                    if (y >= floor_ptr->height - 1)
                        y = floor_ptr->height - 2;
                    else if (y <= 0)
                        y = 1;
                }

                m = i;
            }

            continue;
        }

        bool move_fast = FALSE;
        if (!(mode & TARGET_LOOK))
            print_path(creature_ptr, y, x);

        g_ptr = &floor_ptr->grid_array[y][x];
        strcpy(info, _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>"));
        if (cheat_sight) {
            char cheatinfo[100];
            sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d, SPECIAL:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, y, x),
                projectable(creature_ptr, creature_ptr->y, creature_ptr->x, y, x), g_ptr->special);
            strcat(info, cheatinfo);
        }

        /* Describe and Prompt (enable "TARGET_LOOK") */
        while ((query = examine_grid(creature_ptr, y, x, mode | TARGET_LOOK, info)) == 0)
            ;

        d = 0;
        if (use_menu && (query == '\r'))
            query = 't';

        switch (query) {
        case ESCAPE:
        case 'q':
            done = TRUE;
            break;
        case 't':
        case '.':
        case '5':
        case '0':
            target_who = -1;
            target_row = y;
            target_col = x;
            done = TRUE;
            break;
        case 'p':
            verify_panel(creature_ptr);
            creature_ptr->update |= (PU_MONSTERS);
            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->window |= (PW_OVERHEAD);
            handle_stuff(creature_ptr);
            target_set_prepare(creature_ptr, mode);
            y = creature_ptr->y;
            x = creature_ptr->x;
        case 'o':
            break;
        case ' ':
        case '*':
        case '+':
        case '-':
        case 'm': {
            flag = TRUE;
            m = 0;
            bd = 999;
            for (i = 0; i < tmp_pos.n; i++) {
                t = distance(y, x, tmp_pos.y[i], tmp_pos.x[i]);
                if (t < bd) {
                    m = i;
                    bd = t;
                }
            }

            if (bd == 999)
                flag = FALSE;

            break;
        }
        default: {
            d = get_keymap_dir(query);
            if (isupper(query))
                move_fast = TRUE;

            if (!d)
                bell();
            break;
        }
        }

        if (d) {
            POSITION dx = ddx[d];
            POSITION dy = ddy[d];
            if (move_fast) {
                int mag = MIN(wid / 2, hgt / 2);
                x += dx * mag;
                y += dy * mag;
            } else {
                x += dx;
                y += dy;
            }

            if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0)))
                dx = 0;

            if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0)))
                dy = 0;

            if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min)) {
                if (change_panel(creature_ptr, dy, dx))
                    target_set_prepare(creature_ptr, mode);
            }

            if (x >= floor_ptr->width - 1)
                x = floor_ptr->width - 2;
            else if (x <= 0)
                x = 1;

            if (y >= floor_ptr->height - 1)
                y = floor_ptr->height - 2;
            else if (y <= 0)
                y = 1;
        }
    }

    tmp_pos.n = 0;
    prt("", 0, 0);
    verify_panel(creature_ptr);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD);
    handle_stuff(creature_ptr);
    return target_who != 0;
}

/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 */
bool get_aim_dir(player_type *creature_ptr, DIRECTION *dp)
{
    DIRECTION dir = command_dir;
    if (use_old_target && target_okay(creature_ptr))
        dir = 5;

    COMMAND_CODE code;
    if (repeat_pull(&code))
        if (!(code == 5 && !target_okay(creature_ptr)))
            dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    char command;
    while (!dir) {
        concptr p;
        if (!target_okay(creature_ptr))
            p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
        else
            p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");

        if (!get_com(p, &command, TRUE))
            break;

        if (use_menu && (command == '\r'))
            command = 't';

        switch (command) {
        case 'T':
        case 't':
        case '.':
        case '5':
        case '0':
            dir = 5;
            break;
        case '*':
        case ' ':
        case '\r':
            if (target_set(creature_ptr, TARGET_KILL))
                dir = 5;

            break;
        default:
            dir = get_keymap_dir(command);
            break;
        }

        if ((dir == 5) && !target_okay(creature_ptr))
            dir = 0;

        if (!dir)
            bell();
    }

    if (!dir) {
        project_length = 0;
        return FALSE;
    }

    command_dir = dir;
    if (creature_ptr->confused)
        dir = ddd[randint0(8)];

    if (command_dir != dir)
        msg_print(_("あなたは混乱している。", "You are confused."));

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}

bool get_direction(player_type *creature_ptr, DIRECTION *dp, bool allow_under, bool with_steed)
{
    DIRECTION dir = command_dir;
    COMMAND_CODE code;
    if (repeat_pull(&code))
        dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    concptr prompt = allow_under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ")
                                 : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");

    while (!dir) {
        char ch;
        if (!get_com(prompt, &ch, TRUE))
            break;

        if ((allow_under) && ((ch == '5') || (ch == '-') || (ch == '.'))) {
            dir = 5;
            continue;
        }

        dir = get_keymap_dir(ch);
        if (!dir)
            bell();
    }

    if ((dir == 5) && (!allow_under))
        dir = 0;

    if (!dir)
        return FALSE;

    command_dir = dir;
    if (creature_ptr->confused) {
        if (randint0(100) < 75) {
            dir = ddd[randint0(8)];
        }
    } else if (creature_ptr->riding && with_steed) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (monster_confused_remaining(m_ptr)) {
            if (randint0(100) < 75)
                dir = ddd[randint0(8)];
        } else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
            dir = ddd[randint0(8)];
        else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
            dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        if (creature_ptr->confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            GAME_TEXT m_name[MAX_NLEN];
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];

            monster_desc(creature_ptr, m_name, m_ptr, 0);
            if (monster_confused_remaining(m_ptr)) {
                msg_format(_("%sは混乱している。", "%^s is confused."), m_name);
            } else {
                msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
            }
        }
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}

/*
 * @brief 進行方向を指定する(騎乗対象の混乱の影響を受ける) / Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.  Note that,
 * for example, it is no longer possible to "disarm" or "open" chests
 * in the same grid as the player.
 *
 * Direction "5" is illegal and will (cleanly) abort the command.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
bool get_rep_dir(player_type *creature_ptr, DIRECTION *dp, bool under)
{
    DIRECTION dir = command_dir;
    COMMAND_CODE code;
    if (repeat_pull(&code))
        dir = (DIRECTION)code;

    *dp = (DIRECTION)code;
    concptr prompt
        = under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ") : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (!dir) {
        char ch;
        if (!get_com(prompt, &ch, TRUE))
            break;

        if ((under) && ((ch == '5') || (ch == '-') || (ch == '.'))) {
            dir = 5;
            continue;
        }

        dir = get_keymap_dir(ch);
        if (!dir)
            bell();
    }

    if ((dir == 5) && (!under))
        dir = 0;

    if (!dir)
        return FALSE;

    command_dir = dir;
    if (creature_ptr->confused) {
        if (randint0(100) < 75)
            dir = ddd[randint0(8)];
    } else if (creature_ptr->riding) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (monster_confused_remaining(m_ptr)) {
            if (randint0(100) < 75)
                dir = ddd[randint0(8)];
        } else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
            dir = ddd[randint0(8)];
        else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
            dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        if (creature_ptr->confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            GAME_TEXT m_name[MAX_NLEN];
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
            monster_desc(creature_ptr, m_name, m_ptr, 0);
            if (monster_confused_remaining(m_ptr))
                msg_format(_("%sは混乱している。", "%^s is confused."), m_name);
            else
                msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
        }
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return TRUE;
}

/*
 * XAngband: determine if a given location is "interesting"
 * based on target_set_accept function.
 */
static bool tgt_pt_accept(player_type *creature_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x)))
        return FALSE;

    if ((y == creature_ptr->y) && (x == creature_ptr->x))
        return TRUE;

    if (creature_ptr->image)
        return FALSE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (!(g_ptr->info & (CAVE_MARK)))
        return FALSE;

    if (cave_have_flag_grid(g_ptr, FF_LESS) || cave_have_flag_grid(g_ptr, FF_MORE) || cave_have_flag_grid(g_ptr, FF_QUEST_ENTER)
        || cave_have_flag_grid(g_ptr, FF_QUEST_EXIT))
        return TRUE;

    return FALSE;
}

/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(player_type *creature_ptr)
{
    tmp_pos.n = 0;
    if (!expand_list)
        return;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (POSITION y = 1; y < floor_ptr->height; y++) {
        for (POSITION x = 1; x < floor_ptr->width; x++) {
            if (!tgt_pt_accept(creature_ptr, y, x))
                continue;

            tmp_pos.x[tmp_pos.n] = x;
            tmp_pos.y[tmp_pos.n] = y;
            tmp_pos.n++;
        }
    }

    ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
}

/*
 * old -- from PsiAngband.
 */
bool tgt_pt(player_type *creature_ptr, POSITION *x_ptr, POSITION *y_ptr)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);

    POSITION x = creature_ptr->x;
    POSITION y = creature_ptr->y;
    if (expand_list)
        tgt_pt_prepare(creature_ptr);

    msg_print(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."));
    msg_flag = FALSE;

    char ch = 0;
    int n = 0;
    bool success = FALSE;
    while ((ch != ESCAPE) && !success) {
        bool move_fast = FALSE;
        move_cursor_relative(y, x);
        ch = inkey();
        switch (ch) {
        case ESCAPE:
            break;
        case ' ':
        case 't':
        case '.':
        case '5':
        case '0':
            if (player_bold(creature_ptr, y, x))
                ch = 0;
            else
                success = TRUE;

            break;
        case '>':
        case '<': {
            if (!expand_list || !tmp_pos.n)
                break;

            int dx, dy;
            int cx = (panel_col_min + panel_col_max) / 2;
            int cy = (panel_row_min + panel_row_max) / 2;
            n++;
            for (; n < tmp_pos.n; ++n) {
                grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[tmp_pos.y[n]][tmp_pos.x[n]];
                if (cave_have_flag_grid(g_ptr, FF_STAIRS) && cave_have_flag_grid(g_ptr, ch == '>' ? FF_MORE : FF_LESS))
                    break;
            }

            if (n == tmp_pos.n) {
                n = 0;
                y = creature_ptr->y;
                x = creature_ptr->x;
                verify_panel(creature_ptr);
                creature_ptr->update |= PU_MONSTERS;
                creature_ptr->redraw |= PR_MAP;
                creature_ptr->window |= PW_OVERHEAD;
                handle_stuff(creature_ptr);
            } else {
                y = tmp_pos.y[n];
                x = tmp_pos.x[n];
                dy = 2 * (y - cy) / hgt;
                dx = 2 * (x - cx) / wid;
                if (dy || dx)
                    change_panel(creature_ptr, dy, dx);
            }

            break;
        }
        default: {
            int d = get_keymap_dir(ch);
            if (isupper(ch))
                move_fast = TRUE;

            if (d == 0)
                break;

            int dx = ddx[d];
            int dy = ddy[d];
            if (move_fast) {
                int mag = MIN(wid / 2, hgt / 2);
                x += dx * mag;
                y += dy * mag;
            } else {
                x += dx;
                y += dy;
            }

            if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0)))
                dx = 0;

            if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0)))
                dy = 0;

            if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min))
                change_panel(creature_ptr, dy, dx);

            if (x >= creature_ptr->current_floor_ptr->width - 1)
                x = creature_ptr->current_floor_ptr->width - 2;
            else if (x <= 0)
                x = 1;

            if (y >= creature_ptr->current_floor_ptr->height - 1)
                y = creature_ptr->current_floor_ptr->height - 2;
            else if (y <= 0)
                y = 1;

            break;
        }
        }
    }

    prt("", 0, 0);
    verify_panel(creature_ptr);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD);
    handle_stuff(creature_ptr);
    *x_ptr = x;
    *y_ptr = y;
    return success;
}

/*!
 * @briefプレイヤーの攻撃射程(マス) / Maximum range (spells, etc)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 射程
 */
int get_max_range(player_type *creature_ptr) { return creature_ptr->phase_out ? 36 : 18; }
